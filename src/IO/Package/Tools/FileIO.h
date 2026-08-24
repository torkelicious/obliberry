#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
#include <stdexcept>

namespace IO::Package::Tools {
    inline std::vector<uint8_t> read_file_binary(const std::filesystem::path &filepath) {
        std::ifstream file(filepath, std::ios::in | std::ios::binary);
        if (!file)
            throw std::runtime_error("Could not open file: " + filepath.string());

        file.seekg(0, std::ios::end);
        const auto size = static_cast<std::streamoff>(file.tellg());
        if (size < 0)
            throw std::runtime_error("Could not determine size of file: " + filepath.string());
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> data(static_cast<size_t>(size));
        if (size > 0)
            file.read(reinterpret_cast<char *>(data.data()), size);
        if (!file && !file.eof())
            throw std::runtime_error("Read error in file: " + filepath.string());
        return data;
    }

    inline std::string read_file_string(const std::filesystem::path &filepath) {
        std::ifstream file(filepath, std::ios::in | std::ios::binary);
        if (!file)
            throw std::runtime_error("Could not open file: " + filepath.string());

        file.seekg(0, std::ios::end);
        const auto size = static_cast<std::streamoff>(file.tellg());
        if (size < 0)
            throw std::runtime_error("Could not determine size of file: " + filepath.string());
        file.seekg(0, std::ios::beg);

        std::string str(static_cast<size_t>(size), '\0');
        if (size > 0)
            file.read(str.data(), size);
        if (!file && !file.eof())
            throw std::runtime_error("Read error in file: " + filepath.string());
        return str;
    }
} // namespace IO::Package::Tools
