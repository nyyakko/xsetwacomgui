#pragma once

#include "app/settings/TabletSettings.hpp"
#include "platform/hid/X11/Device.hpp"

struct Tablet
{
    Device stylus;
    Device pad;
    TabletSettings settings;
};
