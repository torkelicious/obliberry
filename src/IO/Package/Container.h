#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// obliberry asset packaging FF
namespace IO {
    namespace Package {
#pragma pack(push, 1)
        struct FileHeader {
            char magic[4] = {'O', 'B', 'P', 'K'};
            uint16_t version = 1;
            uint16_t flags = 0;
            uint32_t entry_count = 0;
            uint64_t toc_offset = 0;
            uint64_t string_table_offset = 0;
            uint64_t string_table_size = 0;
            uint64_t blob_data_offset = 0;
        };

        enum class EntryType : uint8_t { ScriptSource = 0 };

        enum class EntryFlags : uint8_t { None = 0, Compressed = 1 << 0 };

        struct TocEntry {
            uint32_t name_offset;
            uint32_t name_length;
            uint64_t data_offset;
            uint64_t compressed_size;
            uint64_t uncompressed_size;
            EntryType type;
            EntryFlags flags;
            uint8_t _pad[6] = {};
        };
#pragma pack(pop)

        static_assert(sizeof(TocEntry) == 40);
    }


    class ContainerWriter {
    public:
        void add_script(const std::string &canonical_path, const std::string &source);

        void write(const std::filesystem::path &out_file);

    private:
        struct Pending {
            std::string path;
            std::vector<uint8_t> data;
            uint64_t uncompressed_size;
            Package::EntryFlags flags;
        };

        std::vector<Pending> m_entries;
    };

    class ContainerReader {
    public:
        bool open(const std::filesystem::path &file);

        std::optional<std::string> read(const std::string &canonical_path) const;

    private:
        uint64_t m_blob_data_offset = 0;
        std::vector<Package::TocEntry> m_toc;
        std::vector<char> m_string_table;
        std::unordered_map<std::string_view, size_t> m_path_to_index;
        mutable std::ifstream m_file; // open for on-demand reads
    };
}
