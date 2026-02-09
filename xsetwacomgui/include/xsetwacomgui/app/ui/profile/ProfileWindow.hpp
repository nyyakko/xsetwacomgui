#pragma once

#include <liberror/Result.hpp>

#include "app/Context.hpp"

liberror::Result<void> render_profile_window(Context& context);
liberror::Result<bool> render_profile_window(bool isWindowVisible, Context& context, TabletProfile& profile);
