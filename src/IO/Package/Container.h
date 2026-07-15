#pragma once
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// obliberry asset packaging FF
namespace IO {
    namespace Package {
#ifdef None
#undef None
#endif
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

        enum class EntryType : uint8_t { ScriptSource = 0, SerializedAST = 1, BinaryJSON = 2, RawBinary = 3, Media = 4, ShaderSource = 5 };

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
    } // namespace Package


    class ContainerWriter {
    public:
        void add_script(const std::string &canonical_path, const std::string &source);

        void add_compiled_script(const std::string &canonical_path, std::vector<uint8_t> serialized_ast, bool compress = true);

        void add_binary_json(const std::string &canonical_path, std::vector<uint8_t> binary_json, bool compress = true);

        // media, maps, shaders, etc.
        void add_raw_data(const std::string &canonical_path, std::vector<uint8_t> data, Package::EntryType type, bool compress = true);

        void write(const std::filesystem::path &out_file) const;

    private:
        struct Pending {
            std::string path;
            std::vector<uint8_t> data;
            uint64_t uncompressed_size;
            Package::EntryFlags flags;
            Package::EntryType type;
        };

        std::vector<Pending> m_entries;
    };

    class ContainerReader {
    public:
        ContainerReader() = default;
        ~ContainerReader();
        ContainerReader(const ContainerReader &) = delete;
        ContainerReader &operator=(const ContainerReader &) = delete;
        ContainerReader(ContainerReader &&other) noexcept;
        ContainerReader &operator=(ContainerReader &&other) noexcept;

        bool open(const std::filesystem::path &file);

        // returns owned string (decompresses if needed)
        std::optional<std::string> read(const std::string &canonical_path) const;

        // returns string_view into mapped blob for uncompressed entries
        // returns nullopt for compressed entries or missing paths
        std::optional<std::string_view> read_view(const std::string &canonical_path) const;

        void print_entries() const;

        [[nodiscard]] std::vector<std::string> get_entry_paths() const;

    private:
        std::vector<Package::TocEntry> m_toc;
        std::vector<char> m_string_table;
        const char *m_blob_data = nullptr;
        size_t m_blob_size = 0;
        void *m_mapped_region = nullptr;
        size_t m_mapped_size = 0;
        int m_mapped_fd = -1;
        std::unordered_map<std::string_view, size_t> m_path_to_index;
    };
} // namespace IO
