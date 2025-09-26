#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <filesystem>
#include <string>
#include <map>
#include <vector>

struct Font
{
    std::string family;
    std::string style;
    std::filesystem::path path;
};

std::map<std::string, std::vector<Font>> get_available_fonts();
