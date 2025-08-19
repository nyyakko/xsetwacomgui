#pragma once

#include "platform/Display.hpp"

#include <liberror/Result.hpp>
#include <libenum/Enum.hpp>

#include <string_view>

struct Device
{
    struct Pressure
    {
        float minX, minY;
        float maxX, maxY;
    };

    struct Area
    {
        float offsetX, offsetY;
        float width, height;
    };

    enum class Kind { STYLUS, PAD, ERASER, TOUCH };
    enum class Handedness { LEFT, RIGHT };

    std::string name;
    int id;
    Kind kind;
};

liberror::Result<std::vector<Device>> get_available_devices();

liberror::Result<Device::Pressure> get_stylus_pressure_curve(int stylus);
liberror::Result<void> set_stylus_pressure_curve(int stylus, Device::Pressure pressure);
liberror::Result<int> get_stylus_threshold(int stylus);
liberror::Result<void> set_stylus_threshold(int stylus, int threshold);
liberror::Result<int> get_stylus_cursor_proximity(int stylus);
liberror::Result<void> set_stylus_cursor_proximity(int stylus, int proximity);
liberror::Result<Device::Area> get_stylus_default_area(int stylus);
liberror::Result<Device::Area> get_stylus_area(int stylus);
liberror::Result<void> set_stylus_area(int stylus, Device::Area area);
liberror::Result<void> reset_stylus_area(int stylus);
liberror::Result<void> set_stylus_output_from_display_name(int stylus, std::string_view displayName);
liberror::Result<void> set_stylus_output_from_display_area(int stylus, Display::Area area);
liberror::Result<void> set_stylus_handedness(int stylus, Device::Handedness handedness);

