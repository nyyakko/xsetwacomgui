#define IMGUI_DEFINE_MATH_OPERATORS
#include "app/ui/components/AreaMapper.hpp"

static auto constexpr MAPPER_GRAB_RADIUS = 6;

#define MAPPER_BACKGROUD_COLOR          ImColor(ImGui::GetStyle().Colors[ImGuiCol_FrameBg])
#define MAPPER_BACKGROUD_CONTRAST_COLOR ImColor(ImGui::GetStyle().Colors[ImGuiCol_MenuBarBg])
#define MAPPER_GRID_COLOR               ImColor(ImGui::GetStyle().Colors[ImGuiCol_FrameBgHovered])
#define MAPPER_GRAB_COLOR               ImColor(ImGui::GetStyle().Colors[ImGuiCol_SliderGrab])
#define MAPPER_GRAB_ACTIVE_COLOR        ImColor(ImGui::GetStyle().Colors[ImGuiCol_SliderGrabActive])

bool AreaMapper(char const* label, ImVec2 anchors[4], ImVec2 size, ImRect* outPosition, bool forceFullArea, bool)
{
    auto window = ImGui::GetCurrentWindow();

    if (window->SkipItems)
    {
        return false;
    }

    bool changed = false;

    ImGui::BeginGroup();

    ImGui::Text("%s", label);
    ImRect frame(window->DC.CursorPos, window->DC.CursorPos + size);
    ImRect frameExtended = frame;
    frameExtended.Min -= ImVec2(MAPPER_GRAB_RADIUS, MAPPER_GRAB_RADIUS) / 2;
    frameExtended.Max += ImVec2(MAPPER_GRAB_RADIUS, MAPPER_GRAB_RADIUS) / 2;
    ImGui::ItemSize(frameExtended);

    if (outPosition)
    {
        *outPosition = frame;
    }

    if (ImGui::ItemAdd(frameExtended, 0))
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        ImGui::RenderFrame(frame.Min, frame.Max, MAPPER_BACKGROUD_CONTRAST_COLOR);
        ImGui::RenderFrame(anchors[0] * (frame.Max - frame.Min) + frame.Min, anchors[3] * (frame.Max - frame.Min) + frame.Min, MAPPER_BACKGROUD_COLOR);

        for (size_t i = 0; i <= size_t(size.x); i += size_t(size.x / 4))
        {
            drawList->AddLine({ frame.Min.x + float(i), frame.Min.y }, { frame.Min.x + float(i), frame.Max.y }, MAPPER_GRID_COLOR);
        }

        for (size_t i = 0; i <= size_t(size.y); i += size_t(size.y / 4))
        {
            drawList->AddLine({ frame.Min.x, frame.Min.y + float(i) }, { frame.Max.x, frame.Min.y + float(i) }, MAPPER_GRID_COLOR);
        }

        for (size_t i = 0; i < 4; i += 1)
        {
            ImVec2 anchor = anchors[i] * (frame.Max - frame.Min) + frame.Min;
            ImGui::SetCursorScreenPos(anchor - ImVec2(MAPPER_GRAB_RADIUS, MAPPER_GRAB_RADIUS));
            char anchorTooltip[256] {};
            snprintf(anchorTooltip, sizeof(anchorTooltip), "%zu##%s", i, label);
            ImGui::InvisibleButton(anchorTooltip, { 2 * MAPPER_GRAB_RADIUS, 2 * MAPPER_GRAB_RADIUS });

            bool active = ImGui::IsItemActive() || ImGui::IsItemHovered();

            if (active)
            {
                ImGui::SetTooltip("(%4.3f, %4.3f)", double(anchors[i].x), double(anchors[i].y));
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }

            if (!forceFullArea && active && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
                anchors[i] += ImGui::GetIO().MouseDelta / size;
                anchors[i] = { ImClamp(anchors[i].x, 0.f, 1.f), ImClamp(anchors[i].y, 0.f, 1.f) };

                if (i == 0)
                {
                    anchors[1].x = anchors[i].x;
                    anchors[2].y = anchors[i].y;
                }
                else if (i == 1)
                {
                    anchors[0].x = anchors[i].x;
                    anchors[3].y = anchors[i].y;
                }
                else if (i == 2)
                {
                    anchors[3].x = anchors[i].x;
                    anchors[0].y = anchors[i].y;
                }
                else if (i == 3)
                {
                    anchors[2].x = anchors[i].x;
                    anchors[1].y = anchors[i].y;
                }

                changed = true;
            }

            drawList->AddCircleFilled(anchor, MAPPER_GRAB_RADIUS, active ? MAPPER_GRAB_ACTIVE_COLOR : MAPPER_GRAB_COLOR);
        }
    }

    ImGui::EndGroup();

    return changed;
}
