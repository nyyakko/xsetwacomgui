#pragma once

#include <imgui/imgui.hpp>

#include <utility>
#include <vector>

std::pair<bool, bool> DropupButton(char const* label, std::pair<int, int>* const itemIndex, std::vector<std::vector<char const*>> items, ImVec2 const& size);
