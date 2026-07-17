#pragma once
#include <map>
#include <memory>
#include <string>
#include <freetype/freetype.h>
#include <glad/glad.h>
#include <glm/vec2.hpp>

namespace Rendering {
    class Texture;
}

namespace UI {

    struct Glyph {
        glm::ivec2 Size;    // Width and height of glyph in pixels
        glm::ivec2 Bearing; // Offset from baseline to left/top edge
        GLuint Advance;     // Horizontal offset to next glyph (pixels)
        glm::vec2 UVOffset; // Bottom-left UV of glyph in atlas
        glm::vec2 UVSize;   // Glyph size as UV fraction of atlas
    };

    class Font {
    public:
        Font(const std::string &filepath, unsigned int fontSize);
        ~Font();

        Font(const Font &) = delete;
        Font &operator=(const Font &) = delete;
        Font(Font &&) = delete;
        Font &operator=(Font &&) = delete;

        void InitGL() const;

        [[nodiscard]] bool IsValid() const { return m_Valid; }
        [[nodiscard]] const Glyph &GetGlyph(char c) const;
        [[nodiscard]] std::shared_ptr<Rendering::Texture> GetAtlasTexture() const { return m_AtlasTexture; }
        [[nodiscard]] unsigned int GetFontSize() const { return m_FontSize; }

    private:
        FT_Library m_FTLibrary = nullptr;
        FT_Face m_Face = nullptr;
        bool m_Valid = false;
        unsigned int m_FontSize = 0;
        std::map<char, Glyph> m_Glyphs;
        std::shared_ptr<Rendering::Texture> m_AtlasTexture;
    };

} // namespace UI
