#pragma once

#include <liberror/Result.hpp>

#include "core/Context.hpp"

liberror::Result<void> render_main_window(bool isWindowVisible, Context& context);
