#pragma once

inline float& the_scale()
{
    static float scale = 1.0f;
    return scale;
}

inline void set_scale(float value)
{
    auto& scale = the_scale();
    scale = value;
}

inline float operator""_scaled(unsigned long long i)
{
    return static_cast<float>(i) * the_scale();
}
