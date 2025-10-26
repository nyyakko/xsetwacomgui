#pragma once

#include "Area.hpp"

#include <liberror/Result.hpp>

#include <string_view>
#include <map>

struct Device
{
    struct Pressure
    {
        float minX, minY;
        float maxX, maxY;
    };

    enum class Kind { STYLUS, PAD, ERASER, TOUCH };
    enum class Handedness { LEFT, RIGHT };

    std::string name = "INVALID";
    int id;
    Kind kind;
};

liberror::Result<std::vector<Device>> get_available_devices();

liberror::Result<Device::Pressure> get_stylus_pressure_curve(Device stylus);
liberror::Result<void> set_stylus_pressure_curve(Device stylus, Device::Pressure pressure);
liberror::Result<int> get_stylus_threshold(Device stylus);
liberror::Result<void> set_stylus_threshold(Device stylus, int threshold);
liberror::Result<int> get_stylus_cursor_proximity(Device stylus);
liberror::Result<void> set_stylus_cursor_proximity(Device stylus, int proximity);
liberror::Result<Area> get_stylus_default_area(Device stylus);
liberror::Result<Area> get_stylus_area(Device stylus);
liberror::Result<void> set_stylus_area(Device stylus, Area area);
liberror::Result<void> reset_stylus_area(Device stylus);
liberror::Result<void> set_stylus_output_from_display_name(Device stylus, std::string_view displayName);
liberror::Result<void> set_stylus_output_from_display_area(Device stylus, Area area);
liberror::Result<void> set_stylus_handedness(Device stylus, Device::Handedness handedness);

enum class X11Action
{
    LEFT_BUTTON = 1,
    MIDDLE_BUTTON,
    RIGHT_BUTTON,
    SCROLL_UP,
    SCROLL_DOWN,
    SCROLL_LEFT,
    SCROLL_RIGHT,
    BACKWARD_BUTTON,
    FORWARD_BUTTON
};

liberror::Result<std::map<int, X11Action>> get_device_button_mappings(Device device);
liberror::Result<void> set_device_button_mappings(Device device, std::map<int, X11Action> const& mappings);
liberror::Result<void> reset_device_button_mappings(Device device);
liberror::Result<std::map<int, X11Action>> get_device_default_button_mappings(Device device);
