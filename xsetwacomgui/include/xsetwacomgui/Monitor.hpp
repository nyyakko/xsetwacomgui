#pragma once

#include <string>
#include <vector>

#include <liberror/Result.hpp>

struct Monitor
{
    int id;
    bool primary;
    float offsetX, offsetY;
    float width, height;
    std::string name;
};

liberror::Result<std::vector<Monitor>> get_available_monitors();
