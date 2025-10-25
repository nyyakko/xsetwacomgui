#include <spdlog/spdlog.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "MainWindow.hpp"

#include "core/ipc/IPCClient.hpp"
#include "MappingsWindow.hpp"
#include "platform/udev/UDevDevice.hpp"
#include "ProfileWindow.hpp"
#include "ui/components/AreaMapper.hpp"
#include "ui/components/DropupButton.hpp"
#include "ui/Localisation.hpp"
#include "ui/Scaling.hpp"

#include <imgui/extensions/imgui_bezier.hpp>
#include <imgui/extensions/imgui_text.hpp>
#include <imgui/extensions/imgui_toast.hpp>
#include <imgui/imgui.hpp>
#include <liberror/Try.hpp>
#include <range/v3/view.hpp>

#include <sys/poll.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <span>

using namespace liberror;
using namespace std::literals;

static Result<void> render_area_mappers(Context& context)
{
    ImGui::BeginGroup();

    auto previousCursorPosition = ImGui::GetCursorPos();

    static ImVec2 displayAreaAnchors[4] { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } };
    static auto displayDefaultArea = context.displays.empty() ? Display::Area {} : Display::Area { 0, 0, context.display.area.width, context.display.area.height };

    if (context.hasChangedDisplayArea && context.settings.tablet.profile().display.forceFullArea && context.settings.tablet.profile().display.name != "INVALID")
    {
        context.settings.tablet.profile().display.area = displayDefaultArea;
    }

    if (context.hasChangedDisplay && context.settings.tablet.profile().display.name != "INVALID")
    {
        displayDefaultArea = Display::Area { 0, 0, context.display.area.width, context.display.area.height };
    }

    if (!(context.displays.empty() || context.settings.tablet.profile().display.name == "INVALID"))
    {
        displayAreaAnchors[0] = {
            context.settings.tablet.profile().display.area.offsetX / displayDefaultArea.width,
            context.settings.tablet.profile().display.area.offsetY / displayDefaultArea.height
        };
        displayAreaAnchors[1] = {
            context.settings.tablet.profile().display.area.offsetX / displayDefaultArea.width,
            (context.settings.tablet.profile().display.area.height + context.settings.tablet.profile().display.area.offsetY) / displayDefaultArea.height
        };
        displayAreaAnchors[2] = {
            (context.settings.tablet.profile().display.area.width + context.settings.tablet.profile().display.area.offsetX) / displayDefaultArea.width,
            context.settings.tablet.profile().display.area.offsetY / displayDefaultArea.height
        };
        displayAreaAnchors[3] = {
            (context.settings.tablet.profile().display.area.width + context.settings.tablet.profile().display.area.offsetX) / displayDefaultArea.width,
            (context.settings.tablet.profile().display.area.height + context.settings.tablet.profile().display.area.offsetY) / displayDefaultArea.height
        };
    }
    else
    {
        displayAreaAnchors[0] = { 0, 0 };
        displayAreaAnchors[1] = { 0, 1 };
        displayAreaAnchors[2] = { 1, 0 };
        displayAreaAnchors[3] = { 1, 1 };
    }

    static const ImVec2 displayMapperSize { 20 * 16_scaled, 20 * 9_scaled };
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - displayMapperSize.x)/2);
    static ImRect displayMapperPosition {};
    context.hasChangedDisplayArea = AreaMapper(TRY(Localisation::get(context.settings.application.language, Localisation::Window_Main_Tabs_Display_Display)), displayAreaAnchors, displayMapperSize, &displayMapperPosition, context.settings.tablet.profile().display.forceFullArea, context.settings.tablet.profile().display.forceAspectRatio);
    ImGui::SetCursorPosX(previousCursorPosition.x);

    if (context.hasChangedDisplayArea && context.settings.tablet.profile().display.name != "INVALID")
    {
        context.settings.tablet.profile().display.area = {
            .offsetX = displayAreaAnchors[0].x * displayDefaultArea.width,
            .offsetY = displayAreaAnchors[0].y * displayDefaultArea.height,
            .width   = (displayAreaAnchors[2].x - displayAreaAnchors[0].x) * displayDefaultArea.width,
            .height  = (displayAreaAnchors[3].y - displayAreaAnchors[2].y) * displayDefaultArea.height
        };
    }

    static ImVec2 deviceAreaAnchors[4] { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } };
    static auto deviceDefaultArea = context.devices.empty() ? Device::Area {} : TRY(get_stylus_default_area(context.tablet.stylus));

    if (context.hasChangedDeviceArea && context.settings.tablet.profile().stylus.forceFullArea && context.settings.tablet.profile().stylus.name != "INVALID")
    {
        context.settings.tablet.profile().stylus.area = deviceDefaultArea;
    }

    if (context.hasChangedDevice && context.settings.tablet.profile().stylus.name != "INVALID")
    {
        deviceDefaultArea = TRY(get_stylus_default_area(context.tablet.stylus));
    }

    if (!(context.devices.empty() || context.settings.tablet.profile().stylus.name == "INVALID"))
    {
        deviceAreaAnchors[0] = {
            context.settings.tablet.profile().stylus.area.offsetX / deviceDefaultArea.width,
            context.settings.tablet.profile().stylus.area.offsetY / deviceDefaultArea.height
        };
        deviceAreaAnchors[1] = {
            context.settings.tablet.profile().stylus.area.offsetX / deviceDefaultArea.width,
            (context.settings.tablet.profile().stylus.area.height + context.settings.tablet.profile().stylus.area.offsetY) / deviceDefaultArea.height
        };
        deviceAreaAnchors[2] = {
            (context.settings.tablet.profile().stylus.area.width + context.settings.tablet.profile().stylus.area.offsetX) / deviceDefaultArea.width,
            context.settings.tablet.profile().stylus.area.offsetY / deviceDefaultArea.height
        };
        deviceAreaAnchors[3] = {
            (context.settings.tablet.profile().stylus.area.width + context.settings.tablet.profile().stylus.area.offsetX) / deviceDefaultArea.width,
            (context.settings.tablet.profile().stylus.area.height + context.settings.tablet.profile().stylus.area.offsetY) / deviceDefaultArea.height
        };
    }
    else
    {
        deviceAreaAnchors[0] = { 0, 0 };
        deviceAreaAnchors[1] = { 0, 1 };
        deviceAreaAnchors[2] = { 1, 0 };
        deviceAreaAnchors[3] = { 1, 1 };
    }

    static const ImVec2 deviceMapperSize { 15 * 16_scaled, 15 * 9_scaled };
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - deviceMapperSize.x)/2);
    static ImRect deviceMapperPosition {};
    context.hasChangedDeviceArea = AreaMapper(TRY(Localisation::get(context.settings.application.language, Localisation::Window_Main_Tabs_Tablet_Device)), deviceAreaAnchors, deviceMapperSize, &deviceMapperPosition, context.settings.tablet.profile().stylus.forceFullArea, context.settings.tablet.profile().stylus.forceAspectRatio);
    ImGui::SetCursorPosX(previousCursorPosition.x);

    if (context.hasChangedDeviceArea && context.settings.tablet.profile().stylus.name != "INVALID")
    {
        context.settings.tablet.profile().stylus.area = {
            .offsetX = deviceAreaAnchors[0].x * deviceDefaultArea.width,
            .offsetY = deviceAreaAnchors[0].y * deviceDefaultArea.height,
            .width   = (deviceAreaAnchors[2].x - deviceAreaAnchors[0].x) * deviceDefaultArea.width,
            .height  = (deviceAreaAnchors[3].y - deviceAreaAnchors[2].y) * deviceDefaultArea.height
        };
    }

    auto* drawList = ImGui::GetWindowDrawList();

    for (auto [displayAnchor, deviceAnchor] : ranges::views::zip(std::span<ImVec2>(displayAreaAnchors, 4), std::span<ImVec2>(deviceAreaAnchors, 4)))
    {
        auto p1 = displayAnchor * (displayMapperPosition.Max - displayMapperPosition.Min) + displayMapperPosition.Min;
        auto p2 = deviceAnchor * (deviceMapperPosition.Max - deviceMapperPosition.Min) + deviceMapperPosition.Min;
        drawList->AddLine(p1, p2, ImColor(255, 0, 0, 127), 2.f);
    }

    ImGui::EndGroup();

    return {};
}

static Result<void> render_tablet_tab(Context& context)
{
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (250_scaled + 300_scaled + ImGui::GetStyle().WindowPadding.x))/2);

    static auto deviceDefaultArea = context.devices.empty() ? Device::Area {} : TRY(get_stylus_default_area(context.tablet.stylus));

    if (context.hasChangedDevice && context.settings.tablet.profile().stylus.name != "INVALID")
    {
        deviceDefaultArea = TRY(get_stylus_default_area(context.tablet.stylus));
    }

    ImGui::BeginGroup();
    {
        auto deviceNames =
            context.devices
                | ranges::views::filter([] (auto kind) { return kind == Device::Kind::STYLUS; }, &Device::kind)
                | ranges::views::transform([] (auto const& device) { return device.name.data(); })
                | ranges::to_vector;

        static auto deviceIndex = context.settings.tablet.profile().stylus.name == "INVALID" ? 0 : int(
            std::distance(deviceNames.begin(), std::ranges::find(deviceNames, context.settings.tablet.profile().stylus.name))
        );

        if (context.hasChangedDevice)
        {
            deviceIndex = context.settings.tablet.profile().stylus.name == "INVALID" ? 0 : int(
                std::distance(deviceNames.begin(), std::ranges::find(deviceNames, context.settings.tablet.profile().stylus.name))
            );
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Main_Tabs_Tablet_Device)));
        ImGui::SetNextItemWidth(300_scaled + ImGui::GetStyle().WindowPadding.x);
        context.hasChangedDevice = ImGui::Combo("##Device", &deviceIndex, deviceNames.data(), int(deviceNames.size()));

        if (context.hasChangedDevice)
        {
            context.tablet.stylus = context.devices.at(size_t(deviceIndex));
            context.settings.tablet.profile().stylus.name = context.tablet.stylus.name;
            context.settings.tablet.profile().stylus.area = TRY(get_stylus_default_area(context.tablet.stylus));
            context.settings.tablet.profile().stylus.pressure = { 0, 0, 1, 1 };
            context.settings.tablet.profile().stylus.forceFullArea = false;
            context.settings.tablet.profile().stylus.forceAspectRatio = false;
            context.settings.tablet.profile().stylus.handedness = Device::Handedness::RIGHT;
        }

        ImGui::BeginDisabled(context.settings.tablet.profile().stylus.forceFullArea);
        {
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Main_Tabs_Tablet_Width)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletWidth", &context.settings.tablet.profile().stylus.area.width, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Main_Tabs_Tablet_Height)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletHeight", &context.settings.tablet.profile().stylus.area.height, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
        }
        {
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Main_Tabs_Tablet_OffsetX)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletOffsetX", &context.settings.tablet.profile().stylus.area.offsetX, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Main_Tabs_Tablet_OffsetY)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletOffsetY", &context.settings.tablet.profile().stylus.area.offsetY, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
        }
        ImGui::EndDisabled();

        ImGui::BeginGroup();
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Main_Tabs_Tablet_Orientation)));
            char const* orientations[] = {
                TRY(Localisation::get(context.settings.application.language, Localisation::Window_Main_Tabs_Tablet_Orientation_Left)),
                TRY(Localisation::get(context.settings.application.language, Localisation::Window_Main_Tabs_Tablet_Orientation_Right)),
            };
            ImGui::SetNextItemWidth(150_scaled);
            static auto orientationIndex = int(context.settings.tablet.profile().stylus.handedness);

            if (context.hasChangedDeviceHandedness)
            {
                orientationIndex = int(context.settings.tablet.profile().stylus.handedness);
            }

            context.hasChangedDeviceHandedness |= ImGui::Combo("##Orientations", &orientationIndex, orientations, std::size(orientations));

            if (context.hasChangedDeviceHandedness)
            {
                context.settings.tablet.profile().stylus.handedness = Device::Handedness(orientationIndex);
            }

            ImGui::SameLine();

            static auto isMappingsSettingsOpen = false;
            isMappingsSettingsOpen |= ImGui::Button(TRY(Localisation::get(context.settings.application.language, Localisation::Window_Mappings_Title)), { 150_scaled, 0 });
            if (isMappingsSettingsOpen)
            {
                auto [windowWidth, windowHeight] = ImGui::GetWindowSize();

                float mappingsWindowWidth = float(windowWidth)/1.5f, mappingsWindowHeight = float(windowHeight)/1.5f;
                ImGui::SetNextWindowSize({ mappingsWindowWidth, mappingsWindowHeight });
                ImGui::SetNextWindowPos({ (float(windowWidth) - mappingsWindowWidth)/2, (float(windowHeight) - mappingsWindowHeight)/2 });
                ImGui::Begin(
                    TRY(Localisation::get(context.settings.application.language, Localisation::Window_Mappings_Title)),
                    &isMappingsSettingsOpen,
                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
                );
                {
                    TRY(render_mappings_window(context));
                }
                ImGui::End();
            }
        }
        ImGui::EndGroup();

        ImGui::BeginGroup();
        {
            context.hasChangedDeviceArea |= ImGui::Checkbox(TRY(Localisation::get(context.settings.application.language, Localisation::Window_Main_Tabs_Tablet_FullArea)), &context.settings.tablet.profile().stylus.forceFullArea);
            ImGui::BeginDisabled();
            ImGui::Checkbox(TRY(Localisation::get(context.settings.application.language, Localisation::Window_Main_Tabs_Tablet_ForceProportions)), &context.settings.tablet.profile().stylus.forceAspectRatio);
            ImGui::EndDisabled();
        }
        ImGui::EndGroup();
    }
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
    {
        static float pressureAnchors[4] = {};

        if (!context.devices.empty() && context.settings.tablet.profile().stylus.name != "INVALID")
        {
            pressureAnchors[0] = context.settings.tablet.profile().stylus.pressure.minX;
            pressureAnchors[1] = context.settings.tablet.profile().stylus.pressure.minY;
            pressureAnchors[2] = context.settings.tablet.profile().stylus.pressure.maxX;
            pressureAnchors[3] = context.settings.tablet.profile().stylus.pressure.maxY;
        }
        else
        {
            pressureAnchors[0] = 0;
            pressureAnchors[1] = 0;
            pressureAnchors[2] = 1;
            pressureAnchors[3] = 1;
        }

        ImGui::AlignTextToFramePadding();
        context.hasChangedDevicePressure = ImGui::BezierEditor(TRY(Localisation::get(context.settings.application.language, Localisation::Window_Main_Tabs_Tablet_PressureCurve)), pressureAnchors, { 250_scaled, 250_scaled });

        if (context.hasChangedDevicePressure)
        {
            context.settings.tablet.profile().stylus.pressure = { pressureAnchors[0], pressureAnchors[1], pressureAnchors[2], pressureAnchors[3] };
        }
    }
    ImGui::EndGroup();

    if (context.hasChangedDeviceArea && context.settings.tablet.profile().stylus.name != "INVALID")
    {
        context.settings.tablet.profile().stylus.area = {
            .offsetX = std::clamp(context.settings.tablet.profile().stylus.area.offsetX, 0.f, deviceDefaultArea.width),
            .offsetY = std::clamp(context.settings.tablet.profile().stylus.area.offsetY, 0.f, deviceDefaultArea.height),
            .width   = std::clamp(context.settings.tablet.profile().stylus.area.width, 0.f, deviceDefaultArea.width),
            .height  = std::clamp(context.settings.tablet.profile().stylus.area.height, 0.f, deviceDefaultArea.height)
        };
    }

    return {};
}

static Result<void> render_display_tab(Context& context)
{
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (300_scaled + ImGui::GetStyle().WindowPadding.x))/2);

    static auto displayDefaultArea = context.displays.empty() ? Display::Area {} : Display::Area { 0, 0, context.display.area.width, context.display.area.height };

    if (context.hasChangedDisplay && context.settings.tablet.profile().display.name != "INVALID")
    {
        displayDefaultArea = Display::Area { 0, 0, context.display.area.width, context.display.area.height };
    }

    ImGui::BeginGroup();
    {
        auto displayNames = context.displays | ranges::views::transform([] (auto const& display) { return display.name.data(); }) | ranges::to_vector;

        static auto displayIndex = context.settings.tablet.profile().display.name == "INVALID" ? 0 : int(
            std::distance(context.displays.begin(), std::ranges::find(context.displays, context.settings.tablet.profile().display.name, &Display::name))
        );

        if (context.hasChangedDisplay)
        {
            displayIndex = context.settings.tablet.profile().display.name == "INVALID" ? 0 : int(
                std::distance(context.displays.begin(), std::ranges::find(context.displays, context.settings.tablet.profile().display.name, &Display::name))
            );
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Main_Tabs_Display_Display)));
        ImGui::SetNextItemWidth(300_scaled + ImGui::GetStyle().WindowPadding.x);
        context.hasChangedDisplay = ImGui::Combo("##Displays", &displayIndex, displayNames.data(), int(displayNames.size()));

        if (context.hasChangedDisplay)
        {
            context.display = context.displays.at(size_t(displayIndex));
            context.settings.tablet.profile().display.name = context.display.name;
            context.settings.tablet.profile().display.area = Display::Area { 0, 0, context.display.area.width, context.display.area.height };
            context.settings.tablet.profile().display.forceFullArea = false;
            context.settings.tablet.profile().display.forceAspectRatio = false;
        }

        ImGui::BeginDisabled(context.settings.tablet.profile().display.forceFullArea);
        {
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Main_Tabs_Display_Width)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDisplayArea |= ImGui::InputFloat("##DisplayWidth", &context.settings.tablet.profile().display.area.width, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Main_Tabs_Display_Height)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDisplayArea |= ImGui::InputFloat("##DisplayHeight", &context.settings.tablet.profile().display.area.height, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
        }
        {
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Main_Tabs_Display_OffsetX)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDisplayArea |= ImGui::InputFloat("##DisplayOffsetX", &context.settings.tablet.profile().display.area.offsetX, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Main_Tabs_Display_OffsetY)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDisplayArea |= ImGui::InputFloat("##DisplayOffsetY", &context.settings.tablet.profile().display.area.offsetY, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
        }
        ImGui::EndDisabled();

        ImGui::BeginGroup();
        {
            context.hasChangedDisplayArea |= ImGui::Checkbox(TRY(Localisation::get(context.settings.application.language, Localisation::Window_Main_Tabs_Display_FullArea)), &context.settings.tablet.profile().display.forceFullArea);
            ImGui::BeginDisabled();
            ImGui::Checkbox(TRY(Localisation::get(context.settings.application.language, Localisation::Window_Main_Tabs_Display_ForceProportions)), &context.settings.tablet.profile().display.forceAspectRatio);
            ImGui::EndDisabled();
        }
        ImGui::EndGroup();
    }
    ImGui::EndGroup();

    if (context.hasChangedDisplayArea && context.settings.tablet.profile().display.name != "INVALID")
    {
        context.settings.tablet.profile().display.area = {
            .offsetX = std::clamp(context.settings.tablet.profile().display.area.offsetX, 0.f, displayDefaultArea.width),
            .offsetY = std::clamp(context.settings.tablet.profile().display.area.offsetY, 0.f, displayDefaultArea.height),
            .width   = std::clamp(context.settings.tablet.profile().display.area.width, 0.f, displayDefaultArea.width),
            .height  = std::clamp(context.settings.tablet.profile().display.area.height, 0.f, displayDefaultArea.height)
        };
    }

    return {};
}

Result<void> render_main_window(Context& context)
{
#ifdef DEBUG
    static auto shouldWarnAboutDebugBuild = true;

    if (shouldWarnAboutDebugBuild)
    {
        ImGui::PushToast("Debug", "You are running a debug build!");
        shouldWarnAboutDebugBuild = false;
    }
#endif

    static auto hasTriedToInitializeDeviceSettings = false;

    if (context.handleOutdatedDeviceSettings)
    {
        auto [windowWidth, windowHeight] = ImGui::GetWindowSize();

        float migrationPopupWidth = 400_scaled, migrationPopupHeight = 150_scaled;
        ImGui::SetNextWindowSize({ migrationPopupWidth, migrationPopupHeight });
        ImGui::SetNextWindowPos({ (float(windowWidth) - migrationPopupWidth)/2, (float(windowHeight) - migrationPopupHeight)/2 });
        ImGui::Begin(
            TRY(Localisation::get(context.settings.application.language, Localisation::Popup_Outdated_Device_Settings_Title)),
            nullptr,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
        );
        {
            auto [popupWidth, popupHeight] = ImGui::GetWindowSize();

            ImGui::BeginGroup();
            {
                for (auto messageLine :
                    ImGui::SplitToWidth(
                        TRY(Localisation::get(context.settings.application.language, Localisation::Popup_Outdated_Device_Settings_Text)),
                        int(popupWidth)
                    ))
                {
                    ImGui::Text("%s", messageLine.data());
                }
            }
            ImGui::EndGroup();

            ImGui::SetCursorPosY(popupHeight - (25_scaled + ImGui::GetStyle().WindowPadding.y));
            if (ImGui::Button(TRY(Localisation::get(context.settings.application.language, Localisation::Popup_Outdated_Device_Settings_Overwrite)), { 0, 25_scaled }))
            {
                ImGui::PushToast(
                    TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Success)),
                    TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Device_Settings_Overwritten))
                );

                save_tablet_settings(context.settings.tablet);
                context.handleOutdatedDeviceSettings = false;
            }

            ImGui::SameLine();

            if (ImGui::Button(TRY(Localisation::get(context.settings.application.language, Localisation::Popup_Outdated_Device_Settings_Migrate)), { 0, 25_scaled }))
            {
                TRY(migrate_tablet_settings(context.settings.tablet));
            }
        }
        ImGui::End();
    }

    if (context.devices.empty() && !hasTriedToInitializeDeviceSettings)
    {
        ImGui::PushToast(
            TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Warning)),
            TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Devices_Missing))
        );
        hasTriedToInitializeDeviceSettings = true;
    }

    if (!(context.devices.empty() || hasTriedToInitializeDeviceSettings))
    {
        auto result = load_tablet_settings();

        if (!result.has_value())
        {
            context.tablet.stylus = (context.devices
                | ranges::views::filter([] (auto kind) { return kind == Device::Kind::STYLUS; }, &Device::kind)
                | ranges::to_vector
            ).back();

            context.tablet.pad = (context.devices
                | ranges::views::filter([] (auto kind) { return kind == Device::Kind::PAD; }, &Device::kind)
                | ranges::to_vector
            ).back();

            context.display = (context.displays
                | ranges::views::filter(&Display::primary)
                | ranges::to_vector
            ).back();

            asio::co_spawn(context.stExecutor, [] (Context& context, auto error) -> asio::awaitable<void> {
                auto maybeProfile = co_await asio::co_spawn(context.mtExecutor, [] (auto tablet, auto display) -> asio::awaitable<Result<TabletProfile>> {
                    co_return make_tablet_profile("Default", tablet, display);
                }(context.tablet, context.display));

                if (!maybeProfile.has_value())
                {
                    ImGui::PushToast(
                        MUST(Localisation::get(context.settings.application.language, Localisation::Toast_Error)),
                        "Failed to create profile"
                    );
                    co_return;
                }

                context.settings.tablet.add_profile(*maybeProfile);
                context.settings.tablet.profile("Default");

                if (error == SettingsError::Type::FILE_NOT_FOUND)
                {
                    co_await asio::co_spawn(context.mtExecutor, [] (auto settings) -> asio::awaitable<void> {
                        save_tablet_settings(settings);
                        co_return;
                    }(context.settings.tablet));
                }

                auto maybeLoaded = co_await asio::co_spawn(context.mtExecutor, [] (auto settings, auto tablet, auto display) -> asio::awaitable<Result<void>> {
                    co_return load_tablet_profile(settings.profile(), tablet, display);
                }(context.settings.tablet, context.tablet, context.display));

                if (!maybeLoaded.has_value())
                {
                    ImGui::PushToast(
                        MUST(Localisation::get(context.settings.application.language, Localisation::Toast_Error)),
                        "Failed to load profile"
                    );
                }

                co_return;
            }(context, result.error().message()), asio::detached);
            context.stExecutor.restart();

            switch (result.error().message())
            {
            case SettingsError::Type::WRITE_FAILURE: break;
            case SettingsError::Type::FILE_NOT_FOUND: {
                ImGui::PushToast(
                    TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Warning)),
                    TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Device_Settings_Missing))
                );
                break;
            }
            case SettingsError::Type::PROFILE_NOT_FOUND: {
                ImGui::PushToast(
                    TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Error)),
                    TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Profile_Missing))
                );
                break;
            }
            case SettingsError::Type::READ_FAILURE: {
                ImGui::PushToast(
                    TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Warning)),
                    TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Device_Settings_Load_Failed))
                );
                break;
            }
            case SettingsError::Type::OUTDATED_SCHEMA: {
                context.handleOutdatedDeviceSettings = true;
                break;
            }
            }
        }
        else
        {
            context.settings.tablet = *result;

            auto stylus = std::ranges::find(context.devices, context.settings.tablet.profile().stylus.name, &Device::name);
            assert(stylus != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
            context.tablet.stylus = *stylus;

            auto pad = std::ranges::find(context.devices, context.settings.tablet.profile().pad.name, &Device::name);
            assert(pad != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
            context.tablet.pad = *pad;

            context.display = *std::ranges::find(context.displays, context.settings.tablet.profile().display.name, &Display::name);

            TRY(load_tablet_profile(context.settings.tablet.profile(), context.tablet, context.display));
        }

        hasTriedToInitializeDeviceSettings = true;
    }

    auto message = TRY(IPCClient::the().receive_message_async());
    if (!message.empty())
    {
        auto action = magic_enum::enum_cast<UDevDevice::Action>(message.data());
        assert(action && "INVALID ACTION");

        static auto fnGetAvailableDevices = [] (auto shouldRetry) -> asio::awaitable<Result<std::vector<Device>>> {
            Result<std::vector<Device>> maybeDevices {};

            for (auto i = 0; i < 3; i += 1)
            {
                maybeDevices = get_available_devices();
                if (!maybeDevices.has_value()) co_return make_error(maybeDevices.error());
                if (!(maybeDevices->empty() && shouldRetry)) break;
                std::this_thread::sleep_for(250ms);
            }

            co_return maybeDevices;
        };

        if (*action == UDevDevice::Action::BIND && std::ranges::count(context.devices, Device::Kind::STYLUS, &Device::kind) < 1)
        {
            asio::co_spawn(context.stExecutor, [] (Context& context) -> asio::awaitable<void> {
                auto maybeDevices = co_await asio::co_spawn(context.mtExecutor, fnGetAvailableDevices(true));
                if (!maybeDevices.has_value()) make_error(maybeDevices.error());

                if (maybeDevices->empty())
                {
                    ImGui::PushToast(
                        MUST(Localisation::get(context.settings.application.language, Localisation::Toast_Error)),
                        MUST(Localisation::get(context.settings.application.language, Localisation::Toast_Devices_Missing))
                    );
                    co_return;
                }

                context.devices = *maybeDevices;

                auto maybeSettings = co_await asio::co_spawn(context.mtExecutor, [] -> asio::awaitable<Result<SettingsTablet, SettingsError>> {
                    co_return load_tablet_settings();
                });
                assert(maybeSettings.has_value() && "how did you even manage to make this happen?");

                context.settings.tablet = *maybeSettings;

                auto stylus = std::ranges::find(context.devices, context.settings.tablet.profile().stylus.name, &Device::name);
                assert(stylus != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
                context.tablet.stylus = *stylus;

                auto pad = std::ranges::find(context.devices, context.settings.tablet.profile().pad.name, &Device::name);
                assert(pad != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
                context.tablet.pad = *pad;

                context.display = *std::ranges::find(context.displays, context.settings.tablet.profile().display.name, &Display::name);

                context.hasChangedDeviceHandedness = true;
                context.hasChangedDevice = true;
                context.hasChangedDisplay = true;
                context.hasChangedProfile = true;

                auto maybeLoaded = co_await asio::co_spawn(context.mtExecutor, [] (auto settings, auto tablet, auto display) -> asio::awaitable<Result<void>> {
                    co_return load_tablet_profile(settings.profile(), tablet, display);
                }(context.settings.tablet, context.tablet, context.display));

                if (!maybeLoaded.has_value())
                {
                    ImGui::PushToast(
                        MUST(Localisation::get(context.settings.application.language, Localisation::Toast_Error)),
                        "Failed to load profile"
                    );
                }

                co_return;
            }(context), asio::detached);
            context.stExecutor.restart();
        }

        if (*action == UDevDevice::Action::UNBIND && std::ranges::count(context.devices, Device::Kind::STYLUS, &Device::kind) <= 1)
        {
            asio::co_spawn(context.stExecutor, [] (Context& context) -> asio::awaitable<void> {
                auto maybeDevices = co_await asio::co_spawn(context.mtExecutor, fnGetAvailableDevices(false));
                if (!maybeDevices.has_value()) make_error(maybeDevices.error());

                context.devices = *maybeDevices;

                auto maybeDevice = std::ranges::find(context.devices, context.settings.tablet.profile().stylus.name, &Device::name);

                if (maybeDevice == context.devices.end())
                {
                    context.display = {};
                    context.settings.tablet = {};
                    context.tablet = {};
                }

                co_return;
            }(context), asio::detached);
            context.stExecutor.restart();
        }
    }

    ImGui::BeginDisabled(context.handleOutdatedDeviceSettings || context.settings.tablet.profile().name == "INVALID");

    TRY(render_area_mappers(context));

    if (ImGui::BeginTabBar("##Tabs"))
    {
        if (ImGui::BeginTabItem(TRY(Localisation::get(context.settings.application.language, Localisation::Window_Main_Tabs_Tablet_Title))))
        {
            TRY(render_tablet_tab(context));
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(TRY(Localisation::get(context.settings.application.language, Localisation::Window_Main_Tabs_Display_Title))))
        {
            TRY(render_display_tab(context));
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    auto profileNames =
        context.settings.tablet.profiles()
            | ranges::views::keys
            | ranges::views::filter([] (auto const& profile) { return profile != "INVALID"; })
            | ranges::views::transform([] (auto const& profile) { return profile.c_str(); })
            | ranges::to_vector;

    std::vector<std::vector<char const*>> items { { TRY(Localisation::get(context.settings.application.language, Localisation::New_Profile)) }, profileNames };
    static std::pair<int, int> itemIndex { 1, context.settings.tablet.profile().name == "INVALID" ? 0 : std::distance(profileNames.begin(), std::ranges::find(profileNames, context.settings.tablet.profile().name)) };

    if (context.hasChangedProfile)
    {
        itemIndex = { 1, std::distance(profileNames.begin(), std::ranges::find(profileNames, context.settings.tablet.profile().name)) };
    }

    static auto isProfileWindowOpen = false;

    auto previousCursorPosition = ImGui::GetCursorPos();
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - (35_scaled + ImGui::GetStyle().WindowPadding.x));
    static auto isDropupButtonDisabled = false;
    ImGui::BeginDisabled(isDropupButtonDisabled);
    auto [pressedPrimary, pressedSecondary] = DropupButton(TRY(Localisation::get(context.settings.application.language, Localisation::Save_Apply)), &itemIndex, items, { 200_scaled, 35_scaled });
    ImGui::EndDisabled();
    ImGui::SetCursorPos(previousCursorPosition);
    if (pressedPrimary)
    {
        isDropupButtonDisabled = true;
        asio::co_spawn(context.stExecutor, [] (Context& context) -> asio::awaitable<void> {
            auto result = co_await asio::co_spawn(context.mtExecutor, [] (auto settings, auto tablet, auto display) -> asio::awaitable<Result<void>> {
                co_return load_tablet_profile(settings.tablet.profile(), tablet, display);
            }(context.settings, context.tablet, context.display));

            isDropupButtonDisabled = false;

            if (!result)
            {
                ImGui::PushToast(
                    MUST(Localisation::get(context.settings.application.language, Localisation::Toast_Error)),
                    "Failed to load profile"
                );
                co_return;
            }

            co_await asio::co_spawn(context.mtExecutor, [] (auto settings) -> asio::awaitable<void> {
                save_tablet_settings(settings);
                co_return;
            }(context.settings.tablet));

            ImGui::PushToast(
                MUST(Localisation::get(context.settings.application.language, Localisation::Toast_Success)),
                MUST(Localisation::get(context.settings.application.language, Localisation::Toast_Device_Settings_Saved))
            );

            co_return;
        }(context), asio::detached);
        context.stExecutor.restart();
    }
    else if (pressedSecondary)
    {
        if (itemIndex.first == 0 && itemIndex.second == 0)
        {
            isProfileWindowOpen = true;
            itemIndex = { 1, std::distance(profileNames.begin(), std::ranges::find(profileNames, context.settings.tablet.profile().name)) };
        }
        else
        {
            context.settings.tablet.profile(profileNames.at(size_t(itemIndex.second)));
            context.hasChangedProfile = true;
        }
    }

    if (isProfileWindowOpen)
    {
        auto [windowWidth, windowHeight] = ImGui::GetWindowSize();

        float profileWindowWidth = 400_scaled, profileWindowHeight = 200_scaled;
        ImGui::SetNextWindowSize({ profileWindowWidth, profileWindowHeight });
        ImGui::SetNextWindowPos({ (float(windowWidth) - profileWindowWidth)/2, (float(windowHeight) - profileWindowHeight)/2 });
        ImGui::Begin(
            TRY(Localisation::get(context.settings.application.language, Localisation::Window_Profile_Title)),
            &isProfileWindowOpen,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
        );
        {
            TRY(render_profile_window(context));
        }
        ImGui::End();
    }

    ImGui::EndDisabled();

    if (!message.empty())
    {
        context.hasChangedDevice = false;
        context.hasChangedDisplay = false;
    }

    return {};
}
