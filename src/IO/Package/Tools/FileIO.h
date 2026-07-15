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
        return std::vector<uint8_t>(std::istreambuf_iterator(file), std::istreambuf_iterator<char>());
    }

    inline std::string read_file_string(const std::filesystem::path &filepath) {
        auto data = read_file_binary(filepath);
        return std::string(data.begin(), data.end());
    }
} // namespace IO::Package::Tools
