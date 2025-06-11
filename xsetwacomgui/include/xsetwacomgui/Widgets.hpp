#pragma once

#include <imgui/imgui_internal.hpp>

bool area_mapper(char const* label, ImVec2 anchors[4], ImVec2 size, ImRect* outPosition = nullptr, bool forceFullArea = false, bool forceAspectRatio = false);

