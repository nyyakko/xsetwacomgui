#pragma once

#include <liberror/Result.hpp>

enum class IsDaemon { FALSE, TRUE };
enum class Detached { FALSE, TRUE };

liberror::Result<IsDaemon> daemonize(std::string_view name = NAME, Detached detached = Detached::FALSE);
