#include "Font.h"
#include "Logger/LoggerService.h"
#include "Rendering/Texture.h"

#include <cstring>
#include <vector>

#pragma push_macro("LOG_WHO")
#define LOG_WHO "Font"

namespace UI {

    // Maybe resolve with VFS idk

    Font::Font(const std::string &filepath, unsigned int fontSize) : m_FontSize(fontSize) {
        if (FT_Init_FreeType(&m_FTLibrary)) {
            LOG_ERROR(LOG_WHO, "Could not init FreeType");
            return;
        }

        if (FT_New_Face(m_FTLibrary, filepath.c_str(), 0, &m_Face)) {
            LOG_ERROR(LOG_WHO, "Failed to load font: " + filepath);
            FT_Done_FreeType(m_FTLibrary);
            m_FTLibrary = nullptr;
            return;
        }

        FT_Set_Pixel_Sizes(m_Face, 0, fontSize);

        // temp storage
        struct BitmapData {
            std::vector<unsigned char> pixels;
            int width;
            int rows;
        };
        std::map<char, BitmapData> bitmaps;

        constexpr int MAX_ATLAS_WIDTH = 1024;
        int x = 0;
        int y = 0;
        int rowHeight = 0;

        for (unsigned char c = 0; c < 128; c++) {
            if (FT_Load_Char(m_Face, c, FT_LOAD_RENDER)) {
                LOG_WARN(LOG_WHO, std::string("Failed to load glyph: ") + static_cast<char>(c));
                continue;
            }

            const FT_GlyphSlot glyph = m_Face->glyph;
            const int w = static_cast<int>(glyph->bitmap.width);
            const int h = static_cast<int>(glyph->bitmap.rows);

            Glyph g{};
            g.Bearing = glm::ivec2(glyph->bitmap_left, glyph->bitmap_top);
            g.Advance = static_cast<GLuint>(glyph->advance.x >> 6);

            if (w == 0 || h == 0) {
                g.Size = glm::ivec2(0);
                g.UVOffset = glm::vec2(0.0f);
                g.UVSize = glm::vec2(0.0f);
                m_Glyphs[static_cast<char>(c)] = g;
                continue;
            }

            // wrap
            if (x + w > MAX_ATLAS_WIDTH) {
                x = 0;
                y += rowHeight;
                rowHeight = 0;
            }

            // Copy bitmap data
            BitmapData bmp;
            bmp.width = w;
            bmp.rows = h;
            bmp.pixels.resize(static_cast<size_t>(w * h));
            if (glyph->bitmap.buffer) {
                std::memcpy(bmp.pixels.data(), glyph->bitmap.buffer, bmp.pixels.size());
            }
            bitmaps[static_cast<char>(c)] = std::move(bmp);

            // pixel coordinates
            g.Size = glm::ivec2(w, h);
            g.UVOffset = glm::vec2(static_cast<float>(x), static_cast<float>(y));
            g.UVSize = glm::vec2(static_cast<float>(w), static_cast<float>(h));
            m_Glyphs[static_cast<char>(c)] = g;

            if (h > rowHeight)
                rowHeight = h;
            x += w;
        }

        // atlas dimensions
        const int atlasWidth = MAX_ATLAS_WIDTH;
        const int atlasHeight = y + rowHeight;

        if (atlasHeight == 0 || m_Glyphs.empty()) {
            LOG_ERROR(LOG_WHO, "No glyphs loaded from font: " + filepath);
            FT_Done_Face(m_Face);
            m_Face = nullptr;
            FT_Done_FreeType(m_FTLibrary);
            m_FTLibrary = nullptr;
            return;
        }

        // RGBA atlas buffer
        std::vector<unsigned char> atlasPixels(static_cast<size_t>(atlasWidth * atlasHeight * 4), 0);

        for (auto &[c, g] : m_Glyphs) {
            auto it = bitmaps.find(c);
            if (it == bitmaps.end())
                continue;

            const int bmpX = static_cast<int>(g.UVOffset.x);
            const int bmpY = static_cast<int>(g.UVOffset.y);
            const int w = it->second.width;
            const int h = it->second.rows;

            for (int row = 0; row < h; row++) {
                for (int col = 0; col < w; col++) {
                    // Flip
                    const unsigned char alpha = it->second.pixels[static_cast<size_t>((h - 1 - row) * w + col)];
                    const size_t idx = static_cast<size_t>((bmpY + row) * atlasWidth + (bmpX + col)) * 4;
                    atlasPixels[idx + 0] = 255;   // R
                    atlasPixels[idx + 1] = 255;   // G
                    atlasPixels[idx + 2] = 255;   // B
                    atlasPixels[idx + 3] = alpha; // A
                }
            }
        }

        // normalize UV
        const float invAtlasW = 1.0f / static_cast<float>(atlasWidth);
        const float invAtlasH = 1.0f / static_cast<float>(atlasHeight);
        for (auto &[c, g] : m_Glyphs) {
            g.UVOffset.x *= invAtlasW;
            g.UVOffset.y *= invAtlasH;
            g.UVSize.x *= invAtlasW;
            g.UVSize.y *= invAtlasH;
        }

        // Create atlas texture
        m_AtlasTexture = std::make_shared<Rendering::Texture>(atlasWidth, atlasHeight, atlasPixels.data(), GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

        // free FreeType resources
        FT_Done_Face(m_Face);
        m_Face = nullptr;
        FT_Done_FreeType(m_FTLibrary);
        m_FTLibrary = nullptr;

        m_Valid = true;
        LOG_INFO(LOG_WHO, "Font loaded: " + filepath + " (" + std::to_string(m_Glyphs.size()) + " glyphs, " + std::to_string(atlasWidth) + "x" + std::to_string(atlasHeight) + " atlas)");
    }

    Font::~Font() {
        // catches failed or partial construction.
        if (m_Face) {
            FT_Done_Face(m_Face);
            m_Face = nullptr;
        }
        if (m_FTLibrary) {
            FT_Done_FreeType(m_FTLibrary);
            m_FTLibrary = nullptr;
        }
    }

    // GPU upload
    void Font::InitGL() const {
        if (m_AtlasTexture) {
            m_AtlasTexture->InitGL();
        }
    }

    const Glyph &Font::GetGlyph(const char c) const {
        if (const auto it = m_Glyphs.find(c); it != m_Glyphs.end()) {
            return it->second;
        }
        // Fallback
        if (const auto it = m_Glyphs.find(' '); it != m_Glyphs.end()) {
            return it->second;
        }
        return m_Glyphs.begin()->second;
    }

} // namespace UI

#pragma pop_macro("LOG_WHO")
