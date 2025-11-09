#pragma once

#include <imgui/imgui_internal.hpp>

#include <array>

bool AreaMapper(char const* label, std::array<ImVec2, 4>& anchors, ImVec2 size, ImRect* position = nullptr, bool forceFullArea = false);
