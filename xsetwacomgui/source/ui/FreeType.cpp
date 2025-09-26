#include "ui/FreeType.hpp"

#include "platform/Environment.hpp"

#include <array>

std::map<std::string, std::vector<Font>> get_available_fonts()
{
    std::map<std::string, std::vector<Font>> fonts {
        { "Default", { Font { .family = "Default", .style = "Regular", .path = "" } }}
    };

    FT_Library library {};
    FT_Init_FreeType(&library);

    static std::array FONT_PATHS {
        get_system_home_path() / ".fonts",
        get_system_home_path() / ".local/share/fonts",
        std::filesystem::path("/usr/share/fonts"),
        std::filesystem::path("/usr/local/share/fonts")
    };

    for (auto const& path : FONT_PATHS)
    {
        if (!std::filesystem::exists(path)) continue;

        for (auto const& entry : std::filesystem::recursive_directory_iterator(path))
        {
            if (entry.path().extension() != ".ttf") continue;

            if (FT_Face face; FT_New_Face(library, entry.path().c_str(), 0, &face) == 0)
            {
                std::string_view family = face->family_name ? face->family_name : "Unknown";
                std::string_view style = face->style_name ? face->style_name : "Regular";
                fonts[family.data()].push_back({ .family = family.data(), .style = style.data(), .path = entry });
                FT_Done_Face(face);
            }
        }
    }

    FT_Done_FreeType(library);

    return fonts;
}
