#pragma once

#include <liberror/Result.hpp>

#include "app/Context.hpp"
#include "app/core/settings/TabletSettings.hpp"

liberror::Result<void> render_profile_window(Context& context, TabletSettings& settings);
