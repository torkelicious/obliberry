#include "Font.h"
#include "Logger/LoggerService.h"
#include "Rendering/Texture.h"
#include "IO/VFS/VFS.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <ranges>
#include <utility>
#include <vector>

#pragma push_macro("LOG_WHO")
#define LOG_WHO "Font"

namespace UI {

    // 8SSEDT (felzenszwalb & huttenlocher)
    static void ForwardDT(std::vector<float> &dist, const int w, const int h) {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                const int i = y * w + x;
                if (y > 0 && x > 0) {
                    if (const float d = dist[(y - 1) * w + (x - 1)] + 2.0f; d < dist[i])
                        dist[i] = d;
                }
                if (y > 0) {
                    if (const float d = dist[(y - 1) * w + x] + 1.0f; d < dist[i])
                        dist[i] = d;
                }
                if (y > 0 && x < w - 1) {
                    if (const float d = dist[(y - 1) * w + (x + 1)] + 2.0f; d < dist[i])
                        dist[i] = d;
                }
                if (x > 0) {
                    if (const float d = dist[y * w + (x - 1)] + 1.0f; d < dist[i])
                        dist[i] = d;
                }
            }
        }
    }

    static void BackwardDT(std::vector<float> &dist, const int w, const int h) {
        for (int y = h - 1; y >= 0; y--) {
            for (int x = w - 1; x >= 0; x--) {
                const int i = y * w + x;
                if (y < h - 1 && x < w - 1) {
                    if (const float d = dist[(y + 1) * w + (x + 1)] + 2.0f; d < dist[i])
                        dist[i] = d;
                }
                if (y < h - 1) {
                    if (const float d = dist[(y + 1) * w + x] + 1.0f; d < dist[i])
                        dist[i] = d;
                }
                if (y < h - 1 && x > 0) {
                    if (const float d = dist[(y + 1) * w + (x - 1)] + 2.0f; d < dist[i])
                        dist[i] = d;
                }
                if (x < w - 1) {
                    if (const float d = dist[y * w + (x + 1)] + 1.0f; d < dist[i])
                        dist[i] = d;
                }
            }
        }
    }

    static void ComputeSDF(const unsigned char *bitmap, const int w, const int h, const int spread, std::vector<unsigned char> &out) {
        constexpr float INF = 1e20f;
        const int n = w * h;

        // distance to nearest inside pixel (0 if inside)
        std::vector<float> dInside(n);
        // distance to nearest outside pixel (0 if outside)
        std::vector<float> dOutside(n);
        std::vector<bool> inside(n);

        for (int i = 0; i < n; i++) {
            inside[i] = bitmap[i] > 127;
            dInside[i] = inside[i] ? 0.0f : INF;
            dOutside[i] = inside[i] ? INF : 0.0f;
        }

        ForwardDT(dInside, w, h);
        BackwardDT(dInside, w, h);
        ForwardDT(dOutside, w, h);
        BackwardDT(dOutside, w, h);

        out.resize(n);
        for (int i = 0; i < n; i++) {
            float d;
            if (inside[i]) {
                d = std::min(std::sqrt(dOutside[i]), static_cast<float>(spread));
            } else {
                d = -std::min(std::sqrt(dInside[i]), static_cast<float>(spread));
            }
            const float sdf = d / static_cast<float>(spread) * 0.5f + 0.5f;
            out[i] = static_cast<unsigned char>(std::clamp(sdf, 0.0f, 1.0f) * 255.0f);
        }
    }

    Font::Font(std::string filepath, const unsigned int fontSize, const bool useSDF, const unsigned int sdfSpread) : m_FilePath(std::move(filepath)), m_FontSize(fontSize), m_IsSDF(useSDF), m_SDFSpread(sdfSpread) {}

    void Font::LoadCPU() {
        if (m_Valid)
            return;

        std::lock_guard ftLock(FreeType::mutex());

        bool faceLoaded = false;
        if (auto data = IO::VFS::ReadVirtual(m_FilePath)) {
            m_FontData.assign(data->begin(), data->end());
            if (!m_FontData.empty()) {
                faceLoaded = FT_New_Memory_Face(FTLibrary, m_FontData.data(), static_cast<FT_Long>(m_FontData.size()), 0, &m_Face) == 0;
                if (!faceLoaded) {
                    LOG_ERROR(LOG_WHO, "Failed to load font from memory: " + m_FilePath);
                }
            }
        }

        if (!faceLoaded) {
            const std::string resolvedPath = IO::VFS::Resolve(m_FilePath).string();
            if (resolvedPath.empty()) {
                LOG_ERROR(LOG_WHO, "Could not resolve font path: " + m_FilePath);
                return;
            }

            if (FT_New_Face(FTLibrary, resolvedPath.c_str(), 0, &m_Face)) {
                LOG_ERROR(LOG_WHO, "Failed to load font: " + m_FilePath + " (resolved: " + resolvedPath + ")");
                return;
            }
        }

        if (m_IsSDF) {
            BuildSDFAtlas(m_FilePath, m_FontSize, m_SDFSpread);
        } else {
            BuildBitmapAtlas(m_FilePath, m_FontSize);
        }
    }

    void Font::BuildBitmapAtlas(const std::string &filepath, unsigned int fontSize) {
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
                g.LayoutSize = glm::ivec2(0);
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
            g.LayoutSize = g.Size;
            g.UVOffset = glm::vec2(static_cast<float>(x), static_cast<float>(y));
            g.UVSize = glm::vec2(static_cast<float>(w), static_cast<float>(h));
            m_Glyphs[static_cast<char>(c)] = g;

            if (h > rowHeight)
                rowHeight = h;
            x += w;
        }

        // atlas dimensions
        constexpr int atlasWidth = MAX_ATLAS_WIDTH;
        const int atlasHeight = y + rowHeight;

        if (atlasHeight == 0 || m_Glyphs.empty()) {
            LOG_ERROR(LOG_WHO, "No glyphs loaded from font: " + filepath);
            FT_Done_Face(m_Face);
            m_Face = nullptr;
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
                    atlasPixels[idx + 0] = 255;
                    atlasPixels[idx + 1] = 255;
                    atlasPixels[idx + 2] = 255;
                    atlasPixels[idx + 3] = alpha;
                }
            }
        }

        // normalize UV
        constexpr float invAtlasW = 1.0f / static_cast<float>(atlasWidth);
        const float invAtlasH = 1.0f / static_cast<float>(atlasHeight);
        for (auto &g : m_Glyphs | std::views::values) {
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
        m_FontData.clear();
        m_FontData.shrink_to_fit();
        m_Valid = true;
        LOG_INFO(LOG_WHO, "Font loaded: " + filepath + " (" + std::to_string(m_Glyphs.size()) + " glyphs, " + std::to_string(atlasWidth) + "x" + std::to_string(atlasHeight) + " atlas)");
    }

    void Font::BuildSDFAtlas(const std::string &filepath, unsigned int fontSize, unsigned int spread) {
        constexpr int OVERSAMPLE = 3;
        const unsigned int renderSize = fontSize * OVERSAMPLE;

        FT_Set_Pixel_Sizes(m_Face, 0, renderSize);

        struct BitmapData {
            std::vector<unsigned char> pixels;
            int width;
            int rows;
            unsigned int advance;
        };
        std::map<char, BitmapData> bitmaps;

        constexpr int MAX_ATLAS_WIDTH = 2048;
        int x = 0;
        int y = 0;
        int rowHeight = 0;

        for (unsigned char c = 0; c < 128; c++) {
            if (FT_Load_Char(m_Face, c, FT_LOAD_RENDER)) {
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
                g.LayoutSize = glm::ivec2(0);
                g.UVOffset = glm::vec2(0.0f);
                g.UVSize = glm::vec2(0.0f);
                m_Glyphs[static_cast<char>(c)] = g;
                continue;
            }

            const int pad = static_cast<int>(spread);
            const int paddedW = w + pad * 2;
            const int paddedH = h + pad * 2;

            if (x + paddedW > MAX_ATLAS_WIDTH) {
                x = 0;
                y += rowHeight;
                rowHeight = 0;
            }

            BitmapData bmp;
            bmp.width = w;
            bmp.rows = h;
            bmp.advance = static_cast<unsigned int>(glyph->advance.x >> 6);
            bmp.pixels.resize(static_cast<size_t>(w * h));
            if (glyph->bitmap.buffer) {
                std::memcpy(bmp.pixels.data(), glyph->bitmap.buffer, bmp.pixels.size());
            }
            bitmaps[static_cast<char>(c)] = std::move(bmp);

            g.Size = glm::ivec2(paddedW, paddedH);
            g.LayoutSize = glm::ivec2(w, h);
            g.UVOffset = glm::vec2(static_cast<float>(x), static_cast<float>(y));
            g.UVSize = glm::vec2(static_cast<float>(paddedW), static_cast<float>(paddedH));
            m_Glyphs[static_cast<char>(c)] = g;

            if (paddedH > rowHeight)
                rowHeight = paddedH;
            x += paddedW;
        }

        constexpr int atlasWidth = MAX_ATLAS_WIDTH;
        const int atlasHeight = y + rowHeight;

        if (atlasHeight == 0 || m_Glyphs.empty()) {
            LOG_ERROR(LOG_WHO, "No glyphs loaded from font: " + filepath);
            FT_Done_Face(m_Face);
            m_Face = nullptr;
            return;
        }

        // SDF atlas
        std::vector<unsigned char> atlasPixels(static_cast<size_t>(atlasWidth * atlasHeight * 4), 0);

        const int pad = static_cast<int>(spread);

        for (auto &[c, g] : m_Glyphs) {
            auto it = bitmaps.find(c);
            if (it == bitmaps.end())
                continue;

            const int bmpX = static_cast<int>(g.UVOffset.x);
            const int bmpY = static_cast<int>(g.UVOffset.y);
            const int gw = it->second.width;
            const int gh = it->second.rows;
            const int paddedW = g.Size.x;
            const int paddedH = g.Size.y;

            std::vector<unsigned char> paddedBitmap(static_cast<size_t>(paddedW * paddedH), 0);
            for (int row = 0; row < gh; row++) {
                for (int col = 0; col < gw; col++) {
                    const unsigned char alpha = it->second.pixels[static_cast<size_t>((gh - 1 - row) * gw + col)];
                    paddedBitmap[static_cast<size_t>((pad + row) * paddedW + (pad + col))] = alpha;
                }
            }

            std::vector<unsigned char> sdfBuffer;
            ComputeSDF(paddedBitmap.data(), paddedW, paddedH, static_cast<int>(spread), sdfBuffer);

            for (int row = 0; row < paddedH; row++) {
                for (int col = 0; col < paddedW; col++) {
                    const size_t idx = static_cast<size_t>((bmpY + row) * atlasWidth + (bmpX + col)) * 4;
                    atlasPixels[idx + 0] = 255;
                    atlasPixels[idx + 1] = 255;
                    atlasPixels[idx + 2] = 255;
                    atlasPixels[idx + 3] = sdfBuffer[row * paddedW + col];
                }
            }
        }

        // Normalize UVs
        constexpr float invAtlasW = 1.0f / static_cast<float>(atlasWidth);
        const float invAtlasH = 1.0f / static_cast<float>(atlasHeight);
        for (auto &g : m_Glyphs | std::views::values) {
            g.UVOffset.x *= invAtlasW;
            g.UVOffset.y *= invAtlasH;
            g.UVSize.x *= invAtlasW;
            g.UVSize.y *= invAtlasH;
        }

        m_AtlasTexture = std::make_shared<Rendering::Texture>(atlasWidth, atlasHeight, atlasPixels.data(), GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

        FT_Done_Face(m_Face);
        m_Face = nullptr;
        m_FontData.clear();
        m_FontData.shrink_to_fit();

        m_Valid = true;
        LOG_INFO(LOG_WHO, "Font loaded (SDF): " + filepath + " (" + std::to_string(m_Glyphs.size()) + " glyphs, spread=" + std::to_string(spread) + ", " + std::to_string(atlasWidth) + "x" + std::to_string(atlasHeight) +
                                  " atlas)");
    }

    Font::~Font() {
        // catches failed or partial construction.
        if (m_Face) {
            std::lock_guard ftLock(FreeType::mutex());
            FT_Done_Face(m_Face);
            m_Face = nullptr;
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
        if (!m_Glyphs.empty()) {
            return m_Glyphs.begin()->second;
        }
        static constexpr Glyph fallback{};
        return fallback;
    }

} // namespace UI

#pragma pop_macro("LOG_WHO")
