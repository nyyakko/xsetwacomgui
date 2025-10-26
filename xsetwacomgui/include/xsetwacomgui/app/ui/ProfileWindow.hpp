#pragma once

#include <liberror/Result.hpp>

#include "app/Context.hpp"
#include "app/core/settings/TabletSettings.hpp"

liberror::Result<void> render_profile_window(Context& context, TabletSettings& settings);
liberror::Result<bool> render_profile_window(bool isWindowVisible, Context& context, TabletSettings& settings, TabletProfile& profile);
