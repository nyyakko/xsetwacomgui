#pragma once

#include <liberror/Result.hpp>

enum class IsDaemon { FALSE, TRUE };
enum class QuitParent { FALSE, TRUE };

liberror::Result<IsDaemon> daemonize(std::string_view name = NAME, QuitParent killParent = QuitParent::FALSE);
