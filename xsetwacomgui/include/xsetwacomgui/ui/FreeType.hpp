#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <filesystem>
#include <string>
#include <map>
#include <vector>

struct FontInfo
{
    std::string family;
    std::string style;
    std::filesystem::path path;
};

std::map<std::string, std::vector<FontInfo>> get_available_fonts();
