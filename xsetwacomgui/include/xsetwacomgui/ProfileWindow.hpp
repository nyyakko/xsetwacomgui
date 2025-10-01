#pragma once

#include <liberror/Result.hpp>

#include "core/Context.hpp"

liberror::Result<void> render_profile_window(Context& context);
liberror::Result<void> render_profile_window(Context& context, size_t profileId);
