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
        glm::ivec2 Size;       // Quad size (padded for SDF, actual for bitmap)
        glm::ivec2 LayoutSize; // Unpadded size for text layout / bounding box
        glm::ivec2 Bearing;    // Offset from baseline to left/top edge
        GLuint Advance;        // Horizontal offset to next glyph (pixels)
        glm::vec2 UVOffset;    // Bottom-left UV of glyph in atlas
        glm::vec2 UVSize;      // Glyph size as UV fraction of atlas
    };

    class Font {
    public:
        explicit Font(std::string filepath, unsigned int fontSize = 12, bool useSDF = false, unsigned int sdfSpread = 8);
        ~Font();

        Font(const Font &) = delete;
        Font &operator=(const Font &) = delete;
        Font(Font &&) = delete;
        Font &operator=(Font &&) = delete;

        void LoadCPU();

        void InitGL() const;

        [[nodiscard]] bool IsValid() const { return m_Valid; }
        [[nodiscard]] const Glyph &GetGlyph(char c) const;
        [[nodiscard]] std::shared_ptr<Rendering::Texture> GetAtlasTexture() const { return m_AtlasTexture; }
        [[nodiscard]] unsigned int GetFontSize() const { return m_FontSize; }
        [[nodiscard]] bool IsSDF() const { return m_IsSDF; }
        [[nodiscard]] unsigned int GetSDFSpread() const { return m_SDFSpread; }
        [[nodiscard]] const std::string &GetPath() const { return m_FilePath; }

    private:
        void BuildSDFAtlas(const std::string &filepath, unsigned int fontSize, unsigned int spread);
        void BuildBitmapAtlas(const std::string &filepath, unsigned int fontSize);

        FT_Library m_FTLibrary = nullptr;
        FT_Face m_Face = nullptr;
        std::string m_FilePath;
        bool m_Valid = false;
        unsigned int m_FontSize = 0;
        bool m_IsSDF = false;
        unsigned int m_SDFSpread = 0;
        std::map<char, Glyph> m_Glyphs;
        std::shared_ptr<Rendering::Texture> m_AtlasTexture;
    };

} // namespace UI
