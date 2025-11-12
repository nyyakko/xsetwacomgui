#include "platform/Environment.hpp"

#include <filesystem>

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
    return "/usr/local/share";
#endif
}

std::filesystem::path get_application_languages_path()
{
#if DEBUG
    return std::filesystem::path(HOME) / "resources" / "languages";
#else
    return get_application_data_path() / NAME / "languages";
#endif

}

std::filesystem::path get_application_images_path()
{
#if DEBUG
    return std::filesystem::path(HOME) / "resources" / "images";
#else
    return get_application_data_path() / NAME / "images";
#endif

}

std::filesystem::path get_application_icon_path()
{
#if DEBUG
    return std::filesystem::path(HOME) / "resources" / "icons";
#else
    return get_application_data_path() / "icons" / "hicolor";
#endif
}
