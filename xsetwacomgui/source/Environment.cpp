#include "Environment.hpp"

std::filesystem::path get_home_path()
{
    auto home = getenv("HOME");
    assert(home != nullptr && "Why are you homeless?");
    return { home };
}

std::filesystem::path get_settings_path()
{
    auto configHome = getenv("XDG_CONFIG_HOME");
    if (configHome) return std::filesystem::path(configHome);
    return get_home_path() / ".config";
}

