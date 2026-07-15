#include <iostream>
#include <fstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "Container.h"
#include <lz4.h>
#include <ranges>

namespace IO {
    ContainerReader::~ContainerReader() {
        if (m_mapped_region && m_mapped_region != MAP_FAILED) {
            munmap(m_mapped_region, m_mapped_size);
        }
        if (m_mapped_fd >= 0) {
            close(m_mapped_fd);
        }
    }

    ContainerReader::ContainerReader(ContainerReader &&other) noexcept
        : m_toc(std::move(other.m_toc)), m_string_table(std::move(other.m_string_table)), m_blob_data(other.m_blob_data), m_blob_size(other.m_blob_size), m_mapped_region(other.m_mapped_region),
          m_mapped_size(other.m_mapped_size), m_mapped_fd(other.m_mapped_fd), m_path_to_index(std::move(other.m_path_to_index)) {
        other.m_blob_data = nullptr;
        other.m_blob_size = 0;
        other.m_mapped_region = nullptr;
        other.m_mapped_size = 0;
        other.m_mapped_fd = -1;
    }

    ContainerReader &ContainerReader::operator=(ContainerReader &&other) noexcept {
        if (this != &other) {
            if (m_mapped_region && m_mapped_region != MAP_FAILED)
                munmap(m_mapped_region, m_mapped_size);
            if (m_mapped_fd >= 0)
                close(m_mapped_fd);

            m_toc = std::move(other.m_toc);
            m_string_table = std::move(other.m_string_table);
            m_blob_data = other.m_blob_data;
            m_blob_size = other.m_blob_size;
            m_mapped_region = other.m_mapped_region;
            m_mapped_size = other.m_mapped_size;
            m_mapped_fd = other.m_mapped_fd;
            m_path_to_index = std::move(other.m_path_to_index);

            other.m_blob_data = nullptr;
            other.m_blob_size = 0;
            other.m_mapped_region = nullptr;
            other.m_mapped_size = 0;
            other.m_mapped_fd = -1;
        }
        return *this;
    }

    bool ContainerReader::open(const std::filesystem::path &file) {
        // clean up any previous mapping
        if (m_mapped_region && m_mapped_region != MAP_FAILED) {
            munmap(m_mapped_region, m_mapped_size);
            m_mapped_region = nullptr;
        }
        if (m_mapped_fd >= 0) {
            close(m_mapped_fd);
            m_mapped_fd = -1;
        }

        std::ifstream f(file, std::ios::binary);
        if (!f.is_open())
            return false;

        Package::FileHeader header;
        f.read(reinterpret_cast<char *>(&header), sizeof(header));
        if (!f || std::string_view(header.magic, 4) != "OBPK")
            return false;
        if (header.version != 1)
            return false;

        m_toc.resize(header.entry_count);
        f.seekg(header.toc_offset);
        f.read(reinterpret_cast<char *>(m_toc.data()), header.entry_count * sizeof(Package::TocEntry));

        m_string_table.resize(header.string_table_size);
        f.seekg(header.string_table_offset);
        f.read(m_string_table.data(), header.string_table_size);

        f.close();

        for (size_t i = 0; i < m_toc.size(); ++i) {
            std::string_view name(m_string_table.data() + m_toc[i].name_offset, m_toc[i].name_length);
            m_path_to_index[name] = i;
        }

        // mmap the blob region
        const auto file_size = std::filesystem::file_size(file);
        m_mapped_fd = ::open(file.c_str(), O_RDONLY);
        if (m_mapped_fd < 0)
            return false;

        m_mapped_size = file_size;
        m_mapped_region = mmap(nullptr, m_mapped_size, PROT_READ, MAP_PRIVATE, m_mapped_fd, 0);
        if (m_mapped_region == MAP_FAILED) {
            close(m_mapped_fd);
            m_mapped_fd = -1;
            m_mapped_region = nullptr;
            return false;
        }

        m_blob_data = static_cast<const char *>(m_mapped_region) + header.blob_data_offset;
        m_blob_size = file_size - header.blob_data_offset;

        madvise(m_mapped_region, m_mapped_size, MADV_SEQUENTIAL);

        return true;
    }


    std::optional<std::string> ContainerReader::read(const std::string &canonical_path) const {
        const auto it = m_path_to_index.find(canonical_path);
        if (it == m_path_to_index.end())
            return std::nullopt;
        const auto &entry = m_toc[it->second];

        if (entry.data_offset + entry.compressed_size > m_blob_size) {
            return std::nullopt;
        }

        const char *src = m_blob_data + entry.data_offset;

        if (entry.flags == Package::EntryFlags::None) {
            // Uncompressed
            return std::string(src, entry.compressed_size);
        }

        // Compressed
        std::string decompressed(entry.uncompressed_size, '\0');

        if (const int result = LZ4_decompress_safe(src, decompressed.data(), static_cast<int>(entry.compressed_size), static_cast<int>(entry.uncompressed_size));
            result < 0 || static_cast<uint64_t>(result) != entry.uncompressed_size) {
            return std::nullopt;
        }

        return decompressed;
    }

    std::optional<std::string_view> ContainerReader::read_view(const std::string &canonical_path) const {
        const auto it = m_path_to_index.find(canonical_path);
        if (it == m_path_to_index.end())
            return std::nullopt;
        const auto &entry = m_toc[it->second];

        if (entry.data_offset + entry.compressed_size > m_blob_size) {
            return std::nullopt;
        }

        // only zero copy for uncompressed entries...
        if (entry.flags != Package::EntryFlags::None) {
            return std::nullopt;
        }

        return std::string_view(m_blob_data + entry.data_offset, entry.compressed_size);
    }

    void ContainerReader::print_entries() const {
        std::cout << "Package Contents:\n";
        for (const auto &name : m_path_to_index | std::views::keys) {
            std::cout << "  - " << name << "\n";
        }
    }

    std::vector<std::string> ContainerReader::get_entry_paths() const {
        std::vector<std::string> paths;
        paths.reserve(m_path_to_index.size());
        for (const auto &name : m_path_to_index | std::views::keys) {
            paths.emplace_back(name);
        }
        return paths;
    }
} // namespace IO
