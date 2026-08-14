#pragma once
#include "Logger/LoggerService.h"
#include <freetype/freetype.h>

//
// wrapper for shared freetype lib
//

class FreeType {
public:
    static FT_Library library() {
        static FreeType instance;
        return instance.library_;
    }

    FreeType(const FreeType &) = delete;
    FreeType &operator=(const FreeType &) = delete;

private:
    FreeType() {
        if (FT_Init_FreeType(&library_) != 0)
            LOG_ERROR("FreeType", "Could not load freetype.");
    }

    ~FreeType() { FT_Done_FreeType(library_); }

    FT_Library library_ = nullptr;
};
