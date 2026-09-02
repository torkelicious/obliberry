#pragma once
#include "Rendering/PostProcessing/PostProcessing.h"

#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Rendering::PostProcessing::Parser {
    enum class UniformType {
        Float,
        Int,
        Bool, // stored as int (0/1)

        Vec2,
        Vec3,
        Vec4,

        /*
        // future?
        Sampler2D,
        Mat2,
        Mat3,
        Mat4,

        SamplerCube,

        FloatArray,
        Vec2Array,
        Vec3Array,
        Vec4Array,
        */
    };

    struct UniformInfo {
        std::string name;
        UniformType type;
    };

    inline UniformValue UniformTypeEnumToValue(const UniformType type) {
        switch (type) {
            case UniformType::Float:
                return float();
            case UniformType::Int:
            case UniformType::Bool:
                return int();
            case UniformType::Vec2:
                return glm::vec2();
            case UniformType::Vec3:
                return glm::vec3();
            case UniformType::Vec4:
                return glm::vec4();
        }
        return float();
    }

    inline std::optional<UniformType> ParseUniformType(const std::string_view type) {
        if (type == "float")
            return UniformType::Float;
        if (type == "int")
            return UniformType::Int;
        if (type == "bool")
            return UniformType::Bool;
        if (type == "vec2")
            return UniformType::Vec2;
        if (type == "vec3")
            return UniformType::Vec3;
        if (type == "vec4")
            return UniformType::Vec4;
        return std::nullopt;
    }

    inline std::string StripComments(const std::string_view src) {
        std::string result;
        result.reserve(src.size());

        bool lineCmt = false;
        bool blockCmt = false;

        for (size_t i = 0; i < src.size(); ++i) {
            const char c = src[i];
            const char next = (i + 1 < src.size()) ? src[i + 1] : '\0';

            if (lineCmt) {
                if (c == '\n') {
                    lineCmt = false;
                    result += '\n';
                }
                continue;
            }

            if (blockCmt) {
                if (c == '*' && next == '/') {
                    blockCmt = false;
                    ++i; // skip the '/'
                }
                continue;
            }

            if (c == '/' && next == '/') {
                lineCmt = true;
                ++i; // skip the second '/'
                continue;
            }

            if (c == '/' && next == '*') {
                blockCmt = true;
                ++i; // skip the '*'
                continue;
            }

            result += c;
        }

        return result;
    }

    inline std::vector<std::string_view> Tokenize(std::string_view src) {
        std::vector<std::string_view> tokens;
        size_t i = 0;
        while (i < src.size()) {
            while (i < src.size() && !std::isalpha(static_cast<unsigned char>(src[i])) && src[i] != '_') {
                ++i;
            }
            if (i >= src.size()) {
                break;
            }
            const size_t start = i++;
            while (i < src.size() && (std::isalnum(static_cast<unsigned char>(src[i])) || src[i] == '_')) {
                ++i;
            }
            tokens.push_back(src.substr(start, i - start));
        }

        return tokens;
    }

    inline bool IsReservedUniform(const std::string_view name) { return name == kUniformResolution || name == kUniformTexelSize || name == kUniformTime || name == kUniformScene || name == "u_Texture"; }

    // an incredibly basic uniform parser,
    // does NOT support arrays, custom layouts, etc.
    // and will only work for types defined in UniformType enum above.
    inline std::vector<UniformInfo> ParseUniforms(const std::string_view src) {
        const std::string stripped = StripComments(src);
        const auto tokens = Tokenize(stripped);
        std::vector<UniformInfo> uniforms;

        for (size_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i] != "uniform") {
                continue;
            }

            // uniform < type > < name >
            if (i + 2 >= tokens.size()) {
                break;
            }

            const auto type = ParseUniformType(tokens[i + 1]);
            if (!type) {
                continue;
            }

            if (IsReservedUniform(tokens[i + 2])) {
                i += 2;
                continue;
            }

            std::string name(tokens[i + 2]);
            uniforms.push_back({.name = std::move(name), .type = *type});
            i += 2;
        }
        return uniforms;
    }

} // namespace Rendering::PostProcessing::Parser
