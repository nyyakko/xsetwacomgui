#include "ui/Scaling.hpp"

float operator""_scaled(unsigned long long i)
{
    return float(i) * the_scale();
}

float& the_scale()
{
    static float scale = 1.0f;
    return scale;
}

void set_scale(float value)
{
    auto& scale = the_scale();
    scale = value;
}
