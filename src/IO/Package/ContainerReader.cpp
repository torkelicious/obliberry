#include <iostream>
#include <fstream>

#include "Container.h"
#include <lz4.h>
#include <ranges>

namespace IO {
    bool ContainerReader::open(const std::filesystem::path &file) {
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

        for (size_t i = 0; i < m_toc.size(); ++i) {
            std::string_view name(m_string_table.data() + m_toc[i].name_offset, m_toc[i].name_length);
            m_path_to_index[name] = i;
        }

        const auto blob_size = std::filesystem::file_size(file) - header.blob_data_offset;
        m_blob_data.resize(blob_size);
        f.seekg(header.blob_data_offset);
        f.read(m_blob_data.data(), static_cast<std::streamsize>(blob_size));

        return true;
    }


    std::optional<std::string> ContainerReader::read(const std::string &canonical_path) const {
        const auto it = m_path_to_index.find(canonical_path);
        if (it == m_path_to_index.end())
            return std::nullopt;
        const auto &entry = m_toc[it->second];

        if (entry.data_offset + entry.compressed_size > m_blob_data.size()) {
            return std::nullopt; // bounds check
        }

        std::string raw(m_blob_data.data() + entry.data_offset, entry.compressed_size);

        if (entry.flags == Package::EntryFlags::None) {
            return raw;
        }

        std::string decompressed(entry.uncompressed_size, '\0');
        const int result = LZ4_decompress_safe(raw.data(), decompressed.data(), static_cast<int>(entry.compressed_size), static_cast<int>(entry.uncompressed_size));

        if (result < 0 || static_cast<uint64_t>(result) != entry.uncompressed_size) {
            return std::nullopt;
        }

        return decompressed;
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
