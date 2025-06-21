#include "Environment.hpp"

std::filesystem::path get_system_home_path()
{
    auto home = getenv("HOME");
    assert(home != nullptr && "Why are you homeless?");
    return { home };
}

std::filesystem::path get_application_config_path()
{
#ifdef DEBUG
    return std::filesystem::path(HOME) / "build" / "debug";
#else
    auto configHome = getenv("XDG_CONFIG_HOME");
    if (configHome) return std::filesystem::path(configHome) / NAME;
    return get_system_home_path() / ".config" / NAME;
#endif
}

std::filesystem::path get_application_data_path()
{
#if DEBUG
    return std::filesystem::path(HOME) / "resources";
#else
    auto dataHome = getenv("XDG_DATA_HOME");
    if (dataHome) return std::filesystem::path(dataHome) / NAME;
    return get_system_home_path() / ".local" / "share" / NAME;
#endif
}

