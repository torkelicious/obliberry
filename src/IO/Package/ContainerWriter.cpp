#include "Container.h"
#include <fstream>
#include <lz4.h>

namespace IO {
    void ContainerWriter::add_script(const std::string &canonical_path, const std::string &source) {
        Pending p;
        p.path = canonical_path;
        p.uncompressed_size = source.size();
        p.type = Package::EntryType::ScriptSource;

        const int bound = LZ4_compressBound(static_cast<int>(source.size()));
        std::vector<uint8_t> compressed(bound);

        const int compressed_size = LZ4_compress_default(source.data(), reinterpret_cast<char *>(compressed.data()),
                                                         static_cast<int>(source.size()), bound);

        if (compressed_size <= 0) {
            p.data.assign(source.begin(), source.end());
            p.flags = Package::EntryFlags::None;
        } else {
            compressed.resize(compressed_size);
            p.data = std::move(compressed);
            p.flags = Package::EntryFlags::Compressed;
        }
        m_entries.push_back(std::move(p));
    }

    void ContainerWriter::add_compiled_script(const std::string &canonical_path, std::vector<uint8_t> serialized_ast,
                                              const bool compress) {
        add_raw_data(canonical_path, std::move(serialized_ast), Package::EntryType::SerializedAST, compress);
    }

    void ContainerWriter::add_binary_json(const std::string &canonical_path, std::vector<uint8_t> binary_json,
                                          const bool compress) {
        add_raw_data(canonical_path, std::move(binary_json), Package::EntryType::BinaryJSON, compress);
    }

    void ContainerWriter::add_raw_data(const std::string &canonical_path, std::vector<uint8_t> data,
                                       const Package::EntryType type, const bool compress) {
        Pending p;
        p.path = canonical_path;
        p.uncompressed_size = data.size();
        p.type = type;

        if (!compress) {
            p.data = std::move(data);
            p.flags = Package::EntryFlags::None;
        } else {
            const int bound = LZ4_compressBound(static_cast<int>(data.size()));
            std::vector<uint8_t> comp_data(bound);

            const int compressed_size = LZ4_compress_default(reinterpret_cast<const char *>(data.data()),
                                                             reinterpret_cast<char *>(comp_data.data()),
                                                             static_cast<int>(data.size()), bound);

            if (compressed_size <= 0) {
                p.data = std::move(data);
                p.flags = Package::EntryFlags::None;
            } else {
                comp_data.resize(compressed_size);
                p.data = std::move(comp_data);
                p.flags = Package::EntryFlags::Compressed;
            }
        }
        m_entries.push_back(std::move(p));
    }

    void ContainerWriter::write(const std::filesystem::path &out_file) const {
        Package::FileHeader header;
        header.entry_count = static_cast<uint32_t>(m_entries.size());

        std::vector<char> string_table;
        std::vector<uint32_t> name_offsets;
        std::vector<uint32_t> name_lengths;

        for (const auto &e : m_entries) {
            name_offsets.push_back(static_cast<uint32_t>(string_table.size()));
            name_lengths.push_back(static_cast<uint32_t>(e.path.size()));
            string_table.insert(string_table.end(), e.path.begin(), e.path.end());
        }

        // lay out blob data w. per-entry offsets
        std::vector<uint64_t> data_offsets;
        uint64_t running_offset = 0;
        for (const auto &e : m_entries) {
            data_offsets.push_back(running_offset);
            running_offset += e.data.size();
        }

        // build TOC entries
        std::vector<Package::TocEntry> toc;
        for (size_t i = 0; i < m_entries.size(); ++i) {
            Package::TocEntry t{};
            t.name_offset = name_offsets[i];
            t.name_length = name_lengths[i];
            t.data_offset = data_offsets[i];
            t.compressed_size = m_entries[i].data.size();
            t.uncompressed_size = m_entries[i].uncompressed_size;
            t.type = m_entries[i].type;
            t.flags = m_entries[i].flags;
            toc.push_back(t);
        }

        // compute section offsets in the final file
        uint64_t offset = sizeof(Package::FileHeader);
        header.toc_offset = offset;
        offset += toc.size() * sizeof(Package::TocEntry);

        header.string_table_offset = offset;
        header.string_table_size = string_table.size();
        offset += string_table.size();

        header.blob_data_offset = offset;

        // write it all out in order
        std::ofstream out(out_file, std::ios::binary);
        out.write(reinterpret_cast<const char *>(&header), sizeof(header));
        out.write(reinterpret_cast<const char *>(toc.data()), toc.size() * sizeof(Package::TocEntry));
        out.write(string_table.data(), string_table.size());
        for (const auto &e : m_entries) {
            out.write(reinterpret_cast<const char *>(e.data.data()), e.data.size());
        }
    }
} // namespace IO
