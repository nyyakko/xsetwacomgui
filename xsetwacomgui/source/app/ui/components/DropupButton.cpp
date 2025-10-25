#define IMGUI_DEFINE_MATH_OPERATORS
#include "app/ui/components/DropupButton.hpp"

#include "app/core/Scaling.hpp"

#include <spdlog/spdlog.h>

#include <numeric>

#define DROPUP_BUTTON_COLOR        ImGui::GetStyle().Colors[ImGuiCol_FrameBg]
#define DROPUP_BUTTON_COLOR_ACTIVE ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]

std::pair<bool, bool> DropupButton(char const* label, std::pair<int, int>* const itemIndex, std::vector<std::vector<char const*>> items, ImVec2 const& size)
{
    std::pair<bool, bool> pressed { false, false };

    auto [previousX, previousY] = ImGui::GetCursorPos();

    if (ImGui::Button(label, size))
    {
        pressed.first = true;
    }

    auto* drawList = ImGui::GetWindowDrawList();
    auto previouslyPreviousY = previousY;
    previousY = ImGui::GetCursorPosY();

    ImGui::SameLine();

    ImGui::SetCursorPosX(previousX + size.x);
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::IsPopupOpen("Popup", ImGuiPopupFlags_None) ? DROPUP_BUTTON_COLOR_ACTIVE : DROPUP_BUTTON_COLOR);
    if (ImGui::Button("##Popup", { size.y, size.y }))
    {
        ImGui::OpenPopup("Popup");
    }
    ImGui::PopStyleColor();
    auto center = ImGui::GetCursorPos();
    center.x += previousX + size.x + size.y/2 - ImGui::GetStyle().WindowPadding.x;
    center.y -= (size.y + ImGui::GetStyle().WindowPadding.y)/2;
    static auto constexpr radius = 8.f;
    center.y -= radius * 0.25f;
    drawList->AddTriangleFilled(center + ImVec2(0, 1) * radius, center + ImVec2(-0.866f, -0.5f) * radius, center + ImVec2(0.866f, -0.5f) * radius, ImGui::GetColorU32(ImGuiCol_Text));

    auto const popupOffsetY = 2 * ImGui::GetStyle().WindowPadding.y + std::accumulate(items.begin(), items.end(), 0.f, [] (auto total, auto const& current) {
        return total + float(current.size())*(ImGui::GetStyle().ItemSpacing.y + 25_scaled);
    });

    ImGui::SetNextWindowPos({ previousX, previousY - size.y - popupOffsetY - 3 * ImGui::GetStyle().ItemSpacing.y });
    ImGui::SetNextWindowSize({ size.x + size.y, 0 });
    static auto const popupMaxHeight = 2 * ImGui::GetStyle().WindowPadding.y + 6 * (ImGui::GetStyle().ItemSpacing.y + 25_scaled);
    ImGui::SetNextWindowSizeConstraints({}, { size.x + size.y, popupMaxHeight - 4 * ImGui::GetStyle().ItemSpacing.y });
    if (ImGui::BeginPopup("Popup"))
    {
        for (auto i = 0; i < int(items.size()); i += 1)
        {
            auto clicked = false;

            for (auto j = 0; j < int(items.at(size_t(i)).size()); j += 1)
            {
                ImGui::PushID(i + j);
                if (clicked = ImGui::Selectable(items.at(size_t(i)).at(size_t(j)), i == itemIndex->first && j == itemIndex->second, 0, { 0, 25_scaled }); clicked)
                {
                    pressed.second = true;
                    *itemIndex = { i, j };
                }
                ImGui::PopID();
            }

            if (items.size() > 1 && size_t(i) < items.size() - 1)
            {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
            }
        }

        ImGui::EndPopup();
    }
    ImGui::SetCursorPos({ previousX, previouslyPreviousY });

    return pressed;
}
