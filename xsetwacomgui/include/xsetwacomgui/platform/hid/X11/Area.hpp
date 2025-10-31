#pragma once

#include <cmath>

struct Area
{
    float offsetX, offsetY;
    float width, height;
};

inline constexpr bool operator==(Area const& lhs, Area const& rhs)
{
    return
        std::fabs(lhs.offsetX - rhs.offsetX) < std::numeric_limits<float>::epsilon() &&
        std::fabs(lhs.offsetY - rhs.offsetY) < std::numeric_limits<float>::epsilon() &&
        std::fabs(lhs.width - rhs.width) < std::numeric_limits<float>::epsilon() &&
        std::fabs(lhs.height - rhs.height) < std::numeric_limits<float>::epsilon()
    ;
}
