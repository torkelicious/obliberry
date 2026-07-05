#include "Container.h"
#include <lz4.h>

namespace IO {
    bool ContainerReader::open(const std::filesystem::path &file) {
        m_file.open(file, std::ios::binary);
        if (!m_file.is_open()) return false;

        Package::FileHeader header;
        m_file.read(reinterpret_cast<char *>(&header), sizeof(header));
        if (!m_file || std::string_view(header.magic, 4) != "OBPK") return false;
        if (header.version != 1) return false;

        m_toc.resize(header.entry_count);
        m_file.seekg(header.toc_offset);
        m_file.read(reinterpret_cast<char *>(m_toc.data()), header.entry_count * sizeof(Package::TocEntry));

        m_string_table.resize(header.string_table_size);
        m_file.seekg(header.string_table_offset);
        m_file.read(m_string_table.data(), header.string_table_size);

        for (size_t i = 0; i < m_toc.size(); ++i) {
            std::string_view name(m_string_table.data() + m_toc[i].name_offset, m_toc[i].name_length);
            m_path_to_index[name] = i;
        }
        m_blob_data_offset = header.blob_data_offset;
        return true;
    }


    std::optional<std::string> ContainerReader::read(const std::string &canonical_path) const {
        const auto it = m_path_to_index.find(canonical_path);
        if (it == m_path_to_index.end()) return std::nullopt;
        const auto &entry = m_toc[it->second];
        m_file.seekg(m_blob_data_offset + entry.data_offset);
        std::string raw(entry.compressed_size, '\0');
        m_file.read(raw.data(), entry.compressed_size);
        if (!m_file) return std::nullopt; // short read / IO failure

        if (entry.flags == Package::EntryFlags::None) {
            return raw; // wasn't compressed, return back as-is
        }

        std::string decompressed(entry.uncompressed_size, '\0');
        int result = LZ4_decompress_safe(
            raw.data(), decompressed.data(),
            static_cast<int>(entry.compressed_size),
            static_cast<int>(entry.uncompressed_size));

        if (result < 0 || static_cast<uint64_t>(result) != entry.uncompressed_size) {
            return std::nullopt; // corrupt or mismatched data
        }

        return decompressed;
    }
}
