#pragma once

inline auto& get_scale()
{
    static float scale = 1.0f;
    return scale;
}

inline auto set_scale(float value)
{
    get_scale() = value;
}

inline auto operator""_scaled(unsigned long long value)
{
    return float(value) * get_scale();
}
