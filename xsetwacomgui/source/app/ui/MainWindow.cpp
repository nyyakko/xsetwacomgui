#define IMGUI_DEFINE_MATH_OPERATORS
#include "app/ui/MainWindow.hpp"

#include "app/core/Localisation.hpp"
#include "app/core/Scaling.hpp"
#include "app/ui/components/AreaMapper.hpp"
#include "app/ui/components/DropupButton.hpp"
#include "app/ui/MappingsWindow.hpp"
#include "app/ui/ProfileWindow.hpp"
#include "utils/MakeAsync.hpp"

#include <imgui/extensions/imgui_bezier.hpp>
#include <imgui/extensions/imgui_text.hpp>
#include <imgui/extensions/imgui_toast.hpp>
#include <imgui/imgui.hpp>
#include <liberror/Try.hpp>
#include <range/v3/algorithm.hpp>
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

    static std::array<ImVec2, 4> displayAreaAnchors {{ { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } }};
    static Area displayDefaultArea {};

    if (context.hasChangedDisplayArea && context.tablet.settings.profile()->second.display.forceFullArea && context.tablet.settings.profile()->second.display.name != "INVALID")
    {
        context.tablet.settings.profile()->second.display.area = displayDefaultArea;
    }

    if ((context.hasChangedDeviceSettings && context.display.name != "INVALID") || (context.hasChangedDisplay && context.tablet.settings.profile()->second.display.name != "INVALID"))
    {
        displayDefaultArea = { 0, 0, context.display.area.width, context.display.area.height };
    }

    if (!(context.displays.empty() || context.tablet.settings.profile()->second.display.name == "INVALID" || displayDefaultArea == Area {}))
    {
        displayAreaAnchors[0] = {
            context.tablet.settings.profile()->second.display.area.offsetX / displayDefaultArea.width,
            context.tablet.settings.profile()->second.display.area.offsetY / displayDefaultArea.height
        };
        displayAreaAnchors[1] = {
            context.tablet.settings.profile()->second.display.area.offsetX / displayDefaultArea.width,
            (context.tablet.settings.profile()->second.display.area.height + context.tablet.settings.profile()->second.display.area.offsetY) / displayDefaultArea.height
        };
        displayAreaAnchors[2] = {
            (context.tablet.settings.profile()->second.display.area.width + context.tablet.settings.profile()->second.display.area.offsetX) / displayDefaultArea.width,
            context.tablet.settings.profile()->second.display.area.offsetY / displayDefaultArea.height
        };
        displayAreaAnchors[3] = {
            (context.tablet.settings.profile()->second.display.area.width + context.tablet.settings.profile()->second.display.area.offsetX) / displayDefaultArea.width,
            (context.tablet.settings.profile()->second.display.area.height + context.tablet.settings.profile()->second.display.area.offsetY) / displayDefaultArea.height
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
    context.hasChangedDisplayArea =
        AreaMapper(
            TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_Tabs_Display_Display)),
            displayAreaAnchors,
            displayMapperSize,
            &displayMapperPosition,
            context.tablet.settings.profile()->second.display.forceFullArea
        );
    ImGui::SetCursorPosX(previousCursorPosition.x);

    if (context.hasChangedDisplayArea && context.tablet.settings.profile()->second.display.name != "INVALID")
    {
        context.tablet.settings.profile()->second.display.area = {
            .offsetX = displayAreaAnchors[0].x * displayDefaultArea.width,
            .offsetY = displayAreaAnchors[0].y * displayDefaultArea.height,
            .width = (displayAreaAnchors[2].x - displayAreaAnchors[0].x) * displayDefaultArea.width,
            .height = (displayAreaAnchors[3].y - displayAreaAnchors[2].y) * displayDefaultArea.height
        };
    }

    static std::array<ImVec2, 4> deviceAreaAnchors {{ { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } }};
    static Area deviceDefaultArea {};

    if (context.hasChangedDeviceArea && context.tablet.settings.profile()->second.stylus.forceFullArea && context.tablet.settings.profile()->second.stylus.name != "INVALID")
    {
        context.tablet.settings.profile()->second.stylus.area = deviceDefaultArea;
    }

    if ((context.hasChangedDeviceSettings && context.tablet.stylus.name != "INVALID") || (context.hasChangedDevice && context.tablet.settings.profile()->second.stylus.name != "INVALID"))
    {
        asio::co_spawn(context.stExecutor, [] (Context& context_) -> asio::awaitable<void> {
            deviceDefaultArea = MUST(co_await asio::co_spawn(context_.mtExecutor, make_async<get_stylus_default_area>(auto(context_.tablet.stylus))));
        }(context), asio::detached);
    }

    if (!(context.devices.empty() || context.tablet.settings.profile()->second.stylus.name == "INVALID" || deviceDefaultArea == Area {}))
    {
        deviceAreaAnchors[0] = {
            context.tablet.settings.profile()->second.stylus.area.offsetX / deviceDefaultArea.width,
            context.tablet.settings.profile()->second.stylus.area.offsetY / deviceDefaultArea.height
        };
        deviceAreaAnchors[1] = {
            context.tablet.settings.profile()->second.stylus.area.offsetX / deviceDefaultArea.width,
            (context.tablet.settings.profile()->second.stylus.area.height + context.tablet.settings.profile()->second.stylus.area.offsetY) / deviceDefaultArea.height
        };
        deviceAreaAnchors[2] = {
            (context.tablet.settings.profile()->second.stylus.area.width + context.tablet.settings.profile()->second.stylus.area.offsetX) / deviceDefaultArea.width,
            context.tablet.settings.profile()->second.stylus.area.offsetY / deviceDefaultArea.height
        };
        deviceAreaAnchors[3] = {
            (context.tablet.settings.profile()->second.stylus.area.width + context.tablet.settings.profile()->second.stylus.area.offsetX) / deviceDefaultArea.width,
            (context.tablet.settings.profile()->second.stylus.area.height + context.tablet.settings.profile()->second.stylus.area.offsetY) / deviceDefaultArea.height
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
    context.hasChangedDeviceArea =
        AreaMapper(
            TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_Tabs_Tablet_Device)),
            deviceAreaAnchors,
            deviceMapperSize,
            &deviceMapperPosition,
            context.tablet.settings.profile()->second.stylus.forceFullArea
        );
    ImGui::SetCursorPosX(previousCursorPosition.x);

    if (context.hasChangedDeviceArea && context.tablet.settings.profile()->second.stylus.name != "INVALID")
    {
        context.tablet.settings.profile()->second.stylus.area = {
            .offsetX = deviceAreaAnchors[0].x * deviceDefaultArea.width,
            .offsetY = deviceAreaAnchors[0].y * deviceDefaultArea.height,
            .width = (deviceAreaAnchors[2].x - deviceAreaAnchors[0].x) * deviceDefaultArea.width,
            .height = (deviceAreaAnchors[3].y - deviceAreaAnchors[2].y) * deviceDefaultArea.height
        };
    }

    auto* drawList = ImGui::GetWindowDrawList();

    for (auto [displayAnchor, deviceAnchor] : ranges::views::zip(std::span<ImVec2>(displayAreaAnchors), std::span<ImVec2>(deviceAreaAnchors)))
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

    static Area deviceDefaultArea {};

    if ((context.hasChangedDeviceSettings && context.tablet.stylus.name != "INVALID") || (context.hasChangedDevice && context.tablet.settings.profile()->second.stylus.name != "INVALID"))
    {
        asio::co_spawn(context.stExecutor, [] (Context& context_) -> asio::awaitable<void> {
            deviceDefaultArea = MUST(co_await asio::co_spawn(context_.mtExecutor, make_async<get_stylus_default_area>(auto(context_.tablet.stylus))));
        }(context), asio::detached);
    }

    ImGui::BeginGroup();
    {
        auto deviceNames =
            context.devices
                | ranges::views::filter([] (auto kind) { return kind == Device::Kind::STYLUS; }, &Device::kind)
                | ranges::views::transform([] (auto const& device) { return device.name.data(); })
                | ranges::to_vector;

        static auto deviceIndex = context.tablet.settings.profile()->second.stylus.name == "INVALID" ? 0 : int(
            std::distance(deviceNames.begin(), ranges::find(deviceNames, context.tablet.settings.profile()->second.stylus.name))
        );

        if (context.hasChangedDevice)
        {
            deviceIndex = context.tablet.settings.profile()->second.stylus.name == "INVALID" ? 0 : int(
                std::distance(deviceNames.begin(), ranges::find(deviceNames, context.tablet.settings.profile()->second.stylus.name))
            );
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_Tabs_Tablet_Device)));
        ImGui::SetNextItemWidth(300_scaled + ImGui::GetStyle().WindowPadding.x);
        context.hasChangedDevice = ImGui::Combo("##Device", &deviceIndex, deviceNames.data(), int(deviceNames.size()));

        if (context.hasChangedDevice)
        {
            asio::co_spawn(context.stExecutor, [] (Context& context_) -> asio::awaitable<void> {
                context_.tablet.stylus = context_.devices.at(size_t(deviceIndex));
                context_.tablet.settings.profile()->second.stylus.area = MUST(co_await asio::co_spawn(context_.mtExecutor, make_async<get_stylus_default_area>(auto(context_.tablet.stylus))));
                context_.tablet.settings.profile()->second.stylus.name = context_.tablet.stylus.name;
                context_.tablet.settings.profile()->second.stylus.pressure = { 0, 0, 1, 1 };
                context_.tablet.settings.profile()->second.stylus.forceFullArea = false;
                context_.tablet.settings.profile()->second.stylus.handedness = Device::Handedness::RIGHT;
            }(context), asio::detached);
        }

        ImGui::BeginDisabled(context.tablet.settings.profile()->second.stylus.forceFullArea);
        {
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_Tabs_Tablet_Width)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletWidth", &context.tablet.settings.profile()->second.stylus.area.width, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_Tabs_Tablet_Height)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletHeight", &context.tablet.settings.profile()->second.stylus.area.height, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
        }
        {
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_Tabs_Tablet_OffsetX)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletOffsetX", &context.tablet.settings.profile()->second.stylus.area.offsetX, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_Tabs_Tablet_OffsetY)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletOffsetY", &context.tablet.settings.profile()->second.stylus.area.offsetY, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
        }
        ImGui::EndDisabled();

        ImGui::BeginGroup();
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s", TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_Tabs_Tablet_Orientation)));
            char const* orientations[] = {
                TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_Tabs_Tablet_Orientation_Left)),
                TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_Tabs_Tablet_Orientation_Right)),
            };
            ImGui::SetNextItemWidth(150_scaled);
            static auto orientationIndex = int(context.tablet.settings.profile()->second.stylus.handedness);

            if (context.hasChangedDeviceHandedness)
            {
                orientationIndex = int(context.tablet.settings.profile()->second.stylus.handedness);
            }

            context.hasChangedDeviceHandedness |= ImGui::Combo("##Orientations", &orientationIndex, orientations, std::size(orientations));

            if (context.hasChangedDeviceHandedness)
            {
                context.tablet.settings.profile()->second.stylus.handedness = Device::Handedness(orientationIndex);
            }

            ImGui::SameLine();

            static auto isMappingsSettingsOpen = false;
            isMappingsSettingsOpen |= ImGui::Button(TRY(Localisation::get(context.settings.language(), Localisation::Window_Mappings_Title)), { 150_scaled, 0 });
            if (isMappingsSettingsOpen)
            {
                auto [windowWidth, windowHeight] = ImGui::GetWindowSize();

                float mappingsWindowWidth = float(windowWidth)/1.5f, mappingsWindowHeight = float(windowHeight)/1.5f;
                ImGui::SetNextWindowSize({ mappingsWindowWidth, mappingsWindowHeight });
                ImGui::SetNextWindowPos({ (float(windowWidth) - mappingsWindowWidth)/2, (float(windowHeight) - mappingsWindowHeight)/2 });
                ImGui::Begin(
                    TRY(Localisation::get(context.settings.language(), Localisation::Window_Mappings_Title)),
                    &isMappingsSettingsOpen,
                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
                );
                {
                    TRY(render_mappings_window(isMappingsSettingsOpen, context));
                }
                ImGui::End();
            }
        }
        ImGui::EndGroup();

        ImGui::BeginGroup();
        {
            context.hasChangedDeviceArea |= ImGui::Checkbox(TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_Tabs_Tablet_FullArea)), &context.tablet.settings.profile()->second.stylus.forceFullArea);
        }
        ImGui::EndGroup();
    }
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
    {
        static float pressureAnchors[4] = {};

        if (!context.devices.empty() && context.tablet.settings.profile()->second.stylus.name != "INVALID")
        {
            pressureAnchors[0] = context.tablet.settings.profile()->second.stylus.pressure.minX;
            pressureAnchors[1] = context.tablet.settings.profile()->second.stylus.pressure.minY;
            pressureAnchors[2] = context.tablet.settings.profile()->second.stylus.pressure.maxX;
            pressureAnchors[3] = context.tablet.settings.profile()->second.stylus.pressure.maxY;
        }
        else
        {
            pressureAnchors[0] = 0;
            pressureAnchors[1] = 0;
            pressureAnchors[2] = 1;
            pressureAnchors[3] = 1;
        }

        ImGui::AlignTextToFramePadding();
        context.hasChangedDevicePressure = ImGui::BezierEditor(TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_Tabs_Tablet_PressureCurve)), pressureAnchors, { 250_scaled, 250_scaled });

        if (context.hasChangedDevicePressure)
        {
            context.tablet.settings.profile()->second.stylus.pressure = { pressureAnchors[0], pressureAnchors[1], pressureAnchors[2], pressureAnchors[3] };
        }
    }
    ImGui::EndGroup();

    if (context.hasChangedDeviceArea && context.tablet.settings.profile()->second.stylus.name != "INVALID" && deviceDefaultArea != Area {})
    {
        context.tablet.settings.profile()->second.stylus.area = {
            .offsetX = std::clamp(context.tablet.settings.profile()->second.stylus.area.offsetX, 0.f, deviceDefaultArea.width),
            .offsetY = std::clamp(context.tablet.settings.profile()->second.stylus.area.offsetY, 0.f, deviceDefaultArea.height),
            .width = std::clamp(context.tablet.settings.profile()->second.stylus.area.width, 0.f, deviceDefaultArea.width),
            .height = std::clamp(context.tablet.settings.profile()->second.stylus.area.height, 0.f, deviceDefaultArea.height)
        };
    }

    return {};
}

static Result<void> render_display_tab(Context& context)
{
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (300_scaled + ImGui::GetStyle().WindowPadding.x))/2);

    static Area displayDefaultArea {};

    if ((context.hasChangedDeviceSettings && context.display.name != "INVALID") || (context.hasChangedDisplay && context.tablet.settings.profile()->second.display.name != "INVALID"))
    {
        displayDefaultArea = { 0, 0, context.display.area.width, context.display.area.height };
    }

    ImGui::BeginGroup();
    {
        auto displayNames = context.displays | ranges::views::transform([] (auto const& display) { return display.name.data(); }) | ranges::to_vector;

        static auto displayIndex = context.tablet.settings.profile()->second.display.name == "INVALID" ? 0 : int(
            std::distance(context.displays.begin(), ranges::find(context.displays, context.tablet.settings.profile()->second.display.name, &Display::name))
        );

        if (context.hasChangedDisplay)
        {
            displayIndex = context.tablet.settings.profile()->second.display.name == "INVALID" ? 0 : int(
                std::distance(context.displays.begin(), ranges::find(context.displays, context.tablet.settings.profile()->second.display.name, &Display::name))
            );
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_Tabs_Display_Display)));
        ImGui::SetNextItemWidth(300_scaled + ImGui::GetStyle().WindowPadding.x);
        context.hasChangedDisplay = ImGui::Combo("##Displays", &displayIndex, displayNames.data(), int(displayNames.size()));

        if (context.hasChangedDisplay)
        {
            context.display = context.displays.at(size_t(displayIndex));
            context.tablet.settings.profile()->second.display.name = context.display.name;
            context.tablet.settings.profile()->second.display.area = { 0, 0, context.display.area.width, context.display.area.height };
            context.tablet.settings.profile()->second.display.forceFullArea = false;
        }

        ImGui::BeginDisabled(context.tablet.settings.profile()->second.display.forceFullArea);
        {
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_Tabs_Display_Width)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDisplayArea |= ImGui::InputFloat("##DisplayWidth", &context.tablet.settings.profile()->second.display.area.width, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_Tabs_Display_Height)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDisplayArea |= ImGui::InputFloat("##DisplayHeight", &context.tablet.settings.profile()->second.display.area.height, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
        }
        {
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_Tabs_Display_OffsetX)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDisplayArea |= ImGui::InputFloat("##DisplayOffsetX", &context.tablet.settings.profile()->second.display.area.offsetX, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_Tabs_Display_OffsetY)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDisplayArea |= ImGui::InputFloat("##DisplayOffsetY", &context.tablet.settings.profile()->second.display.area.offsetY, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
        }
        ImGui::EndDisabled();

        ImGui::BeginGroup();
        {
            context.hasChangedDisplayArea |= ImGui::Checkbox(TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_Tabs_Display_FullArea)), &context.tablet.settings.profile()->second.display.forceFullArea);
        }
        ImGui::EndGroup();
    }
    ImGui::EndGroup();

    if (context.hasChangedDisplayArea && context.tablet.settings.profile()->second.display.name != "INVALID" && displayDefaultArea != Area {})
    {
        context.tablet.settings.profile()->second.display.area = {
            .offsetX = std::clamp(context.tablet.settings.profile()->second.display.area.offsetX, 0.f, displayDefaultArea.width),
            .offsetY = std::clamp(context.tablet.settings.profile()->second.display.area.offsetY, 0.f, displayDefaultArea.height),
            .width = std::clamp(context.tablet.settings.profile()->second.display.area.width, 0.f, displayDefaultArea.width),
            .height = std::clamp(context.tablet.settings.profile()->second.display.area.height, 0.f, displayDefaultArea.height)
        };
    }

    return {};
}

static Result<void> render_migration_popup(Context& context)
{
    auto [width, height] = ImGui::GetWindowSize();

    ImGui::BeginGroup();
    {
        for (auto messageLine :
            ImGui::SplitToWidth(
                TRY(Localisation::get(context.settings.language(), Localisation::Popup_Outdated_Device_Settings_Text)),
                int(width)
            ))
        {
            ImGui::Text("%s", messageLine.data());
        }
    }
    ImGui::EndGroup();

    ImGui::SetCursorPosY(height - (25_scaled + ImGui::GetStyle().WindowPadding.y));
    if (ImGui::Button(TRY(Localisation::get(context.settings.language(), Localisation::Popup_Outdated_Device_Settings_Overwrite)), { 0, 25_scaled }))
    {
        context.handleOutdatedDeviceSettings = false;
        asio::co_spawn(context.stExecutor, [] (Context& context_) -> asio::awaitable<void> {
            MUST(co_await asio::co_spawn(context_.mtExecutor, make_async<save_tablet_settings>(auto(context_.tablet.settings))));
            ImGui::PushToast(
                MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Success)),
                MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Device_Settings_Overwritten))
            );
        }(context), asio::detached);
    }

    ImGui::SameLine();

    if (ImGui::Button(TRY(Localisation::get(context.settings.language(), Localisation::Popup_Outdated_Device_Settings_Migrate)), { 0, 25_scaled }))
    {
        context.handleOutdatedDeviceSettings = false;
        asio::co_spawn(context.stExecutor, [] (Context& context_) -> asio::awaitable<void> {
            auto maybeMigrated = co_await asio::co_spawn(context_.mtExecutor, make_async<migrate_tablet_settings>(auto(context_.tablet.settings)));
            if (!maybeMigrated.has_value())
            {
                ImGui::PushToast(
                    MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Error)),
                    MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Device_Settings_Migration_Failed))
                );
                co_return;
            }
        }(context), asio::detached);
    }

    return {};
}

Result<void> render_main_window(Context& context)
{
    static auto hasTriedToInitializeDeviceSettings = false;

    if (context.handleOutdatedDeviceSettings)
    {
        auto [windowWidth, windowHeight] = ImGui::GetWindowSize();

        float migrationPopupWidth = 400_scaled, migrationPopupHeight = 150_scaled;
        ImGui::SetNextWindowSize({ migrationPopupWidth, migrationPopupHeight });
        ImGui::SetNextWindowPos({ (float(windowWidth) - migrationPopupWidth)/2, (float(windowHeight) - migrationPopupHeight)/2 });
        ImGui::Begin(
            TRY(Localisation::get(context.settings.language(), Localisation::Popup_Outdated_Device_Settings_Title)),
            nullptr,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
        );
        {
            TRY(render_migration_popup(context));
        }
        ImGui::End();
    }

    if (context.devices.empty() && !hasTriedToInitializeDeviceSettings)
    {
        hasTriedToInitializeDeviceSettings = true;
        ImGui::PushToast(
            TRY(Localisation::get(context.settings.language(), Localisation::Toast_Warning)),
            TRY(Localisation::get(context.settings.language(), Localisation::Toast_Devices_Missing))
        );
    }

    if (!(context.devices.empty() || hasTriedToInitializeDeviceSettings))
    {
        hasTriedToInitializeDeviceSettings = true;
        asio::co_spawn(context.stExecutor, [] (Context& context_) -> asio::awaitable<void> {
            auto result = co_await asio::co_spawn(context_.mtExecutor, make_async<load_tablet_settings>());

            if (!result.has_value())
            {
                auto stylus = ranges::find(context_.devices, Device::Kind::STYLUS, &Device::kind);
                assert(stylus != context_.devices.end());
                context_.tablet.stylus = *stylus;

                auto pad = ranges::find(context_.devices, Device::Kind::PAD, &Device::kind);
                assert(pad != context_.devices.end());
                context_.tablet.pad = *pad;

                auto display = ranges::find(context_.displays, true, &Display::primary);
                assert(display != context_.displays.end());
                context_.display = *display;

                auto maybeCreated = co_await asio::co_spawn(context_.mtExecutor, make_async<make_tablet_profile>("Default", auto(context_.tablet.stylus), auto(context_.tablet.pad), auto(context_.display)));
                if (!maybeCreated.has_value())
                {
                    ImGui::PushToast(
                        MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Error)),
                        MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Profile_Create_Failed))
                    );
                    co_return;
                }

                auto [iterator, _] = context_.tablet.settings.profiles().insert({ maybeCreated->name, *maybeCreated });
                context_.tablet.settings.profile(iterator);

                context_.hasChangedDeviceSettings = true;

                auto maybeLoaded = co_await asio::co_spawn(context_.mtExecutor, make_async<load_tablet_profile>(auto(context_.tablet.settings.profile()->second), auto(context_.tablet.stylus), auto(context_.tablet.pad), auto(context_.display)));
                if (!maybeLoaded.has_value())
                {
                    ImGui::PushToast(
                        MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Error)),
                        MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Profile_Load_Failed))
                    );
                }

                switch (result.error())
                {
                case SettingsError::WRITE_FAILURE: break;
                case SettingsError::FILE_NOT_FOUND: {
                    MUST(co_await asio::co_spawn(context_.mtExecutor, make_async<save_tablet_settings>(auto(context_.tablet.settings))));
                    ImGui::PushToast(
                        MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Warning)),
                        MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Device_Settings_Missing))
                    );
                    break;
                }
                case SettingsError::READ_FAILURE: {
                    ImGui::PushToast(
                        MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Warning)),
                        MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Device_Settings_Load_Failed))
                    );
                    break;
                }
                case SettingsError::OUTDATED_SCHEMA: {
                    context_.handleOutdatedDeviceSettings = true;
                    break;
                }
                }
            }
            else
            {
                context_.tablet.settings = std::move(*result);

                auto stylus = ranges::find(context_.devices, context_.tablet.settings.profile()->second.stylus.name, &Device::name);
                assert(stylus != context_.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
                context_.tablet.stylus = *stylus;

                auto pad = ranges::find(context_.devices, context_.tablet.settings.profile()->second.pad.name, &Device::name);
                assert(pad != context_.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
                context_.tablet.pad = *pad;

                context_.display = *ranges::find(context_.displays, context_.tablet.settings.profile()->second.display.name, &Display::name);

                context_.hasChangedDeviceSettings = true;

                auto maybeLoaded = co_await asio::co_spawn(context_.mtExecutor, make_async<load_tablet_profile>(auto(context_.tablet.settings.profile()->second), auto(context_.tablet.stylus), auto(context_.tablet.pad), auto(context_.display)));
                if (!maybeLoaded.has_value())
                {
                    ImGui::PushToast(
                        MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Error)),
                        MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Profile_Load_Failed))
                    );
                }
            }
        }(context), asio::detached);
    }

    ImGui::BeginDisabled(context.handleOutdatedDeviceSettings || context.tablet.settings.profile()->second.name == "INVALID");

    TRY(render_area_mappers(context));

    if (ImGui::BeginTabBar("##Tabs"))
    {
        if (ImGui::BeginTabItem(TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_Tabs_Tablet_Title))))
        {
            TRY(render_tablet_tab(context));
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_Tabs_Display_Title))))
        {
            TRY(render_display_tab(context));
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    auto profiles =
        context.tablet.settings.profiles()
            | ranges::views::keys
            | ranges::views::filter([] (auto const& profile) { return profile != "INVALID"; })
            | ranges::views::transform([] (auto const& profile) { return profile.c_str(); })
            | ranges::to_vector;

    std::vector<std::vector<char const*>> items { { TRY(Localisation::get(context.settings.language(), Localisation::New_Profile)) }, profiles };
    static std::pair<int, int> itemIndex { 1, 0 };

    if (context.hasChangedDeviceSettings && context.tablet.settings.profile()->second.name != "INVALID")
    {
        itemIndex = { 1, std::distance(profiles.begin(), ranges::find(profiles, context.tablet.settings.profile()->second.name)) };
    }

    static auto isProfileWindowOpen = false;
    static auto isProfileEditWindowOpen = false;

    auto previousCursorPosition = ImGui::GetCursorPos();
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - (35_scaled + ImGui::GetStyle().WindowPadding.x));
    static auto isDropupButtonDisabled = false;
    ImGui::BeginDisabled(isDropupButtonDisabled);
    auto [pressedPrimary, pressedSecondary] = DropupButton(TRY(Localisation::get(context.settings.language(), Localisation::Save)), &itemIndex, items, { 200_scaled, 35_scaled });
    ImGui::EndDisabled();
    ImGui::SetCursorPos(previousCursorPosition);
    if (pressedPrimary)
    {
        isDropupButtonDisabled = true;
        asio::co_spawn(context.stExecutor, [] (Context& context_) -> asio::awaitable<void> {
            auto maybeLoaded = co_await asio::co_spawn(context_.mtExecutor, make_async<load_tablet_profile>(auto(context_.tablet.settings.profile()->second), auto(context_.tablet.stylus), auto(context_.tablet.pad), auto(context_.display)));
            isDropupButtonDisabled = false;
            if (!maybeLoaded)
            {
                ImGui::PushToast(
                    MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Error)),
                    MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Profile_Load_Failed))
                );
                co_return;
            }

            MUST(co_await asio::co_spawn(context_.mtExecutor, make_async<save_tablet_settings>(auto(context_.tablet.settings))));

            ImGui::PushToast(
                MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Success)),
                MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Device_Settings_Saved))
            );
        }(context), asio::detached);
    }
    else if (pressedSecondary != -1)
    {
        if (pressedSecondary == ImGuiMouseButton_Left)
        {
            if (itemIndex.first == 0)
            {
                isProfileWindowOpen = true;
                itemIndex = { 1, std::distance(profiles.begin(), ranges::find(profiles, context.tablet.settings.profile()->second.name)) };
            }
            else
            {
                asio::co_spawn(context.stExecutor, [] (Context& context_, std::vector<char const*> profiles_) -> asio::awaitable<void> {
                    context_.tablet.settings.profile(ranges::find_if(context_.tablet.settings.profiles(), [&] (auto const& entry) {
                        return entry.first == profiles_.at(size_t(itemIndex.second));
                    }));

                    context_.hasChangedDeviceSettings = true;

                    auto maybeLoaded = co_await asio::co_spawn(context_.mtExecutor, make_async<load_tablet_profile>(auto(context_.tablet.settings.profile()->second), auto(context_.tablet.stylus), auto(context_.tablet.pad), auto(context_.display)));
                    if (!maybeLoaded)
                    {
                        ImGui::PushToast(
                            MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Error)),
                            MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Profile_Load_Failed))
                        );
                        co_return;
                    }
                }(context, profiles), asio::detached);
            }
        }
        else
        {
            isProfileEditWindowOpen = isProfileWindowOpen = (itemIndex.first != 0);
        }
    }

    if (isProfileWindowOpen)
    {
        auto [windowWidth, windowHeight] = ImGui::GetWindowSize();

        float profileWindowWidth = 400_scaled, profileWindowHeight = 200_scaled;
        ImGui::SetNextWindowSize({ profileWindowWidth, profileWindowHeight });
        ImGui::SetNextWindowPos({ (float(windowWidth) - profileWindowWidth)/2, (float(windowHeight) - profileWindowHeight)/2 });
        ImGui::Begin(
            TRY(Localisation::get(context.settings.language(), Localisation::Window_Profile_Title)),
            &isProfileWindowOpen,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
        );
        {
            if (isProfileEditWindowOpen)
            {
                auto profile = ranges::find_if(context.tablet.settings.profiles(), [&] (auto const& entry) {
                    return entry.first == profiles.at(size_t(itemIndex.second));
                });
                assert(profile != context.tablet.settings.profiles().end());
                auto isWindowClosed = TRY(render_profile_window(isProfileWindowOpen, context, profile->second));

                if (isWindowClosed || !isProfileWindowOpen)
                {
                    itemIndex = { 1, std::distance(profiles.begin(), ranges::find(profiles, context.tablet.settings.profile()->second.name)) };
                    isProfileWindowOpen = false;
                }

                isProfileEditWindowOpen = isProfileWindowOpen;
            }
            else
            {
                TRY(render_profile_window(context));
            }
        }
        ImGui::End();
    }

    ImGui::EndDisabled();

    if (context.hasChangedDeviceSettings)
    {
        context.hasChangedDeviceSettings = false;
    }

    return {};
}
