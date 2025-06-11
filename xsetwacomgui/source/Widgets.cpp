#define IMGUI_DEFINE_MATH_OPERATORS
#include "Widgets.hpp"

static auto constexpr MAPPER_GRAB_RADIUS = 6;

#define MAPPER_BACKGROUD_COLOR   ImColor(ImGui::GetStyle().Colors[ImGuiCol_FrameBg])
#define MAPPER_GRID_COLOR        ImColor(ImGui::GetStyle().Colors[ImGuiCol_FrameBgHovered])
#define MAPPER_GRAB_COLOR        ImColor(ImGui::GetStyle().Colors[ImGuiCol_SliderGrab])
#define MAPPER_GRAB_ACTIVE_COLOR ImColor(ImGui::GetStyle().Colors[ImGuiCol_SliderGrabActive])

static void area_mapper_render_grid(ImDrawList* drawList, ImRect frame, ImVec2 size)
{
    for (size_t i = 0; i <= static_cast<size_t>(size.x); i += static_cast<size_t>(size.x / 4))
        drawList->AddLine({ frame.Min.x + static_cast<float>(i), frame.Min.y }, { frame.Min.x + static_cast<float>(i), frame.Max.y }, MAPPER_GRID_COLOR);
    for (size_t i = 0; i <= static_cast<size_t>(size.y); i += static_cast<size_t>(size.y / 4))
        drawList->AddLine({ frame.Min.x, frame.Min.y + static_cast<float>(i) }, { frame.Max.x, frame.Min.y + static_cast<float>(i) }, MAPPER_GRID_COLOR);
}

static void area_mapper_render_region(ImGuiStyle& style, ImRect frame, ImVec2 const anchors[4])
{
    ImGui::RenderFrame(anchors[0] * (frame.Max - frame.Min) + frame.Min, anchors[3] * (frame.Max - frame.Min) + frame.Min, MAPPER_BACKGROUD_COLOR, true, style.FrameRounding);
}

static bool area_mapper_render_grabbers(ImDrawList* const drawList, ImRect frame, ImVec2 size, char const* label, ImVec2 anchors[4], bool fullArea = false, bool = false)
{
    bool changed = false;

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
            ImGui::SetTooltip("(%4.3f, %4.3f)", static_cast<double>(anchors[i].x), static_cast<double>(anchors[i].y));
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        if (!fullArea && active && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
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

    return changed;
}

bool area_mapper(char const* label, ImVec2 anchors[4], ImVec2 size, ImRect* outPosition, bool forceFullArea, bool forceAspectRatio)
{
    auto window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    bool changed = false;

    ImGui::BeginGroup();

    ImGui::Text("%s", label);
    ImRect frame(window->DC.CursorPos, window->DC.CursorPos + size);
    ImRect frameExtended = frame;
    frameExtended.Min -= ImVec2(MAPPER_GRAB_RADIUS, MAPPER_GRAB_RADIUS) / 2;
    frameExtended.Max += ImVec2(MAPPER_GRAB_RADIUS, MAPPER_GRAB_RADIUS) / 2;
    ImGui::ItemSize(frameExtended);

    if (outPosition) *outPosition = frame;

    if (ImGui::ItemAdd(frameExtended, 0))
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        ImGui::RenderFrame(frame.Min, frame.Max, ImColor(ImGui::GetStyle().Colors[ImGuiCol_MenuBarBg]), true, style.FrameRounding);

        area_mapper_render_region(style, frame, anchors);
        area_mapper_render_grid(drawList, frame, size);
        changed = area_mapper_render_grabbers(drawList, frame, size, label, anchors, forceFullArea, forceAspectRatio);
    }

    ImGui::EndGroup();

    return changed;
}

