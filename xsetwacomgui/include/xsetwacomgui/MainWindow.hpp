#pragma once

#include <liberror/Result.hpp>
#include <libwacom/Device.hpp>

#include "core/Context.hpp"

liberror::Result<void> render_main_window(Context& context);
