#pragma once

#include <liberror/Result.hpp>

enum class IsDaemon
{
    TRUE, FALSE
};

liberror::Result<IsDaemon> daemonize(std::string_view name = NAME);
