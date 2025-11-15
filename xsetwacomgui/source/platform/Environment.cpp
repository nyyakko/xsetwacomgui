#include "platform/Environment.hpp"

#include <array>
#include <filesystem>
#include <range/v3/algorithm/find_if.hpp>

std::filesystem::path get_system_home_path()
{
    auto home = getenv("HOME");
    assert(home != nullptr && "Why are you homeless?");
    return home;
}

std::filesystem::path get_application_config_path()
{
#ifdef DEBUG
    return std::filesystem::path(HOME) / "build" / "debug";
#else
    return get_system_home_path() / ".config" / NAME;
#endif
}

std::filesystem::path get_application_data_path()
{
#if DEBUG
    return std::filesystem::path(HOME) / "resources";
#else
    static std::array paths {
        std::filesystem::path("/usr/local/share") / NAME,
        std::filesystem::path("/usr/share") / NAME,
        get_system_home_path() / ".local" / "share" / NAME
    };

    auto path = ranges::find_if(paths, [] (auto const& path) {
        return std::filesystem::exists(path);
    });
    assert(path != paths.end());

    return *path;
#endif
}

std::filesystem::path get_application_languages_path()
{
#if DEBUG
    return std::filesystem::path(HOME) / "resources" / "languages";
#else
    return get_application_data_path() / "languages";
#endif

}

std::filesystem::path get_application_images_path()
{
#if DEBUG
    return std::filesystem::path(HOME) / "resources" / "images";
#else
    return get_application_data_path() / "images";
#endif

}
