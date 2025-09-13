#define IMGUI_DEFINE_MATH_OPERATORS
#include "MainWindow.hpp"

#include "core/ipc/Client.hpp"
#include "MappingsWindow.hpp"
#include "platform/udev/UDevDevice.hpp"
#include "ui/Localisation.hpp"
#include "ui/Scaling.hpp"
#include "ui/widgets/AreaMapper.hpp"

#include <fplus/fplus.hpp>
#include <imgui/extensions/imgui_bezier.hpp>
#include <imgui/extensions/imgui_text.hpp>
#include <imgui/extensions/imgui_toast.hpp>
#include <imgui/imgui.hpp>
#include <libcoro/Generator.hpp>
#include <liberror/Try.hpp>

#include <sys/poll.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <span>

using namespace liberror;
using namespace libcoro;

static Result<void> render_region_mappers(Context& context)
{
    auto [cursorX, cursorY] = ImGui::GetCursorPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    static ImVec2 displayAreaAnchors[4] { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } };
    static auto displayDefaultArea = context.displays.empty() ? Display::Area {} : Display::Area { 0, 0, context.display.area.width, context.display.area.height };

    if (context.hasChangedDisplayArea && context.tabletSettings.display.forceFullArea && context.tabletSettings.display.name != "INVALID")
    {
        context.tabletSettings.display.area = displayDefaultArea;
    }

    if (context.hasChangedDisplay && context.tabletSettings.display.name != "INVALID")
    {
        displayDefaultArea = Display::Area { 0, 0, context.display.area.width, context.display.area.height };
    }

    if (!context.displays.empty() && context.tabletSettings.display.name != "INVALID")
    {
        displayAreaAnchors[0] = {
            context.tabletSettings.display.area.offsetX / displayDefaultArea.width,
            context.tabletSettings.display.area.offsetY / displayDefaultArea.height
        };
        displayAreaAnchors[1] = {
            context.tabletSettings.display.area.offsetX / displayDefaultArea.width,
            (context.tabletSettings.display.area.height + context.tabletSettings.display.area.offsetY) / displayDefaultArea.height
        };
        displayAreaAnchors[2] = {
            (context.tabletSettings.display.area.width + context.tabletSettings.display.area.offsetX) / displayDefaultArea.width,
            context.tabletSettings.display.area.offsetY / displayDefaultArea.height
        };
        displayAreaAnchors[3] = {
            (context.tabletSettings.display.area.width + context.tabletSettings.display.area.offsetX) / displayDefaultArea.width,
            (context.tabletSettings.display.area.height + context.tabletSettings.display.area.offsetY) / displayDefaultArea.height
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
    context.hasChangedDisplayArea = area_mapper(TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Display_Display)), displayAreaAnchors, displayMapperSize, &displayMapperPosition, context.tabletSettings.display.forceFullArea, context.tabletSettings.display.forceAspectRatio);
    ImGui::SetCursorPosX(cursorX);

    if (context.hasChangedDisplayArea && context.tabletSettings.display.name != "INVALID")
    {
        context.tabletSettings.display.area = {
            .offsetX = displayAreaAnchors[0].x * displayDefaultArea.width,
            .offsetY = displayAreaAnchors[0].y * displayDefaultArea.height,
            .width   = (displayAreaAnchors[2].x - displayAreaAnchors[0].x) * displayDefaultArea.width,
            .height  = (displayAreaAnchors[3].y - displayAreaAnchors[2].y) * displayDefaultArea.height
        };
    }

    static ImVec2 deviceAreaAnchors[4] { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } };
    static Device::Area deviceDefaultArea = context.devices.empty() ? Device::Area {} : TRY(get_stylus_default_area(context.tablet.stylus));

    if (context.hasChangedDeviceArea && context.tabletSettings.stylus.forceFullArea && context.tabletSettings.stylus.name != "INVALID")
    {
        context.tabletSettings.stylus.area = deviceDefaultArea;
    }

    if (context.hasChangedDevice && context.tabletSettings.stylus.name != "INVALID")
    {
        deviceDefaultArea = TRY(get_stylus_default_area(context.tablet.stylus));
    }

    if (!context.devices.empty() && context.tabletSettings.stylus.name != "INVALID")
    {
        deviceAreaAnchors[0] = {
            context.tabletSettings.stylus.area.offsetX / deviceDefaultArea.width,
            context.tabletSettings.stylus.area.offsetY / deviceDefaultArea.height
        };
        deviceAreaAnchors[1] = {
            context.tabletSettings.stylus.area.offsetX / deviceDefaultArea.width,
            (context.tabletSettings.stylus.area.height + context.tabletSettings.stylus.area.offsetY) / deviceDefaultArea.height
        };
        deviceAreaAnchors[2] = {
            (context.tabletSettings.stylus.area.width + context.tabletSettings.stylus.area.offsetX) / deviceDefaultArea.width,
            context.tabletSettings.stylus.area.offsetY / deviceDefaultArea.height
        };
        deviceAreaAnchors[3] = {
            (context.tabletSettings.stylus.area.width + context.tabletSettings.stylus.area.offsetX) / deviceDefaultArea.width,
            (context.tabletSettings.stylus.area.height + context.tabletSettings.stylus.area.offsetY) / deviceDefaultArea.height
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
    context.hasChangedDeviceArea = area_mapper(TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_Device)), deviceAreaAnchors, deviceMapperSize, &deviceMapperPosition, context.tabletSettings.stylus.forceFullArea, context.tabletSettings.stylus.forceAspectRatio);
    ImGui::SetCursorPosX(cursorX);

    if (context.hasChangedDeviceArea && context.tabletSettings.stylus.name != "INVALID")
    {
        context.tabletSettings.stylus.area = {
            .offsetX = deviceAreaAnchors[0].x * deviceDefaultArea.width,
            .offsetY = deviceAreaAnchors[0].y * deviceDefaultArea.height,
            .width   = (deviceAreaAnchors[2].x - deviceAreaAnchors[0].x) * deviceDefaultArea.width,
            .height  = (deviceAreaAnchors[3].y - deviceAreaAnchors[2].y) * deviceDefaultArea.height
        };
    }

    for (auto [displayAnchor, deviceAnchor] : fplus::zip(std::span<ImVec2>(displayAreaAnchors, 4), std::span<ImVec2>(deviceAreaAnchors, 4)))
    {
        auto p1 = displayAnchor * (displayMapperPosition.Max - displayMapperPosition.Min) + displayMapperPosition.Min;
        auto p2 = deviceAnchor * (deviceMapperPosition.Max - deviceMapperPosition.Min) + deviceMapperPosition.Min;
        drawList->AddLine(p1, p2, ImColor(255, 0, 0, 127), 2.f);
    }

    return {};
}

static Result<void> render_tablet_tab(Context& context)
{
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (250_scaled + 300_scaled + ImGui::GetStyle().WindowPadding.x))/2);

    static auto deviceDefaultArea = context.devices.empty() ? Device::Area {} : TRY(get_stylus_default_area(context.tablet.stylus));

    if (context.hasChangedDevice && context.tabletSettings.stylus.name != "INVALID")
    {
        deviceDefaultArea = TRY(get_stylus_default_area(context.tablet.stylus));
    }

    ImGui::BeginGroup();
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_Device)));
        auto devicesFiltered = fplus::keep_if([] (auto&& device) { return device.kind == Device::Kind::STYLUS; }, context.devices);
        auto deviceNames = fplus::transform([] (auto const& device) { return device.name.data(); }, devicesFiltered);
        ImGui::SetNextItemWidth(300_scaled + ImGui::GetStyle().WindowPadding.x);
        static int deviceIndex = context.tabletSettings.stylus.name == "INVALID" ? 0 : static_cast<int>(
            std::distance(devicesFiltered.begin(), std::ranges::find(devicesFiltered, context.tabletSettings.stylus.name, &Device::name))
        );

        if (context.hasChangedDevice)
        {
            deviceIndex = context.tabletSettings.stylus.name == "INVALID" ? 0 : static_cast<int>(
                std::distance(devicesFiltered.begin(), std::ranges::find(devicesFiltered, context.tabletSettings.stylus.name, &Device::name))
            );
        }

        context.hasChangedDevice = ImGui::Combo("##Device", &deviceIndex, deviceNames.data(), static_cast<int>(deviceNames.size()));

        if (context.hasChangedDevice)
        {
            context.tablet.stylus = context.devices.at(static_cast<size_t>(deviceIndex));
            context.tabletSettings.stylus.name = context.tablet.stylus.name;
            context.tabletSettings.stylus.area = TRY(get_stylus_default_area(context.tablet.stylus));
            context.tabletSettings.stylus.pressure = { 0, 0, 1, 1 };
            context.tabletSettings.stylus.forceFullArea = false;
            context.tabletSettings.stylus.forceAspectRatio = false;
            context.tabletSettings.stylus.handedness = Device::Handedness::RIGHT;
        }

        ImGui::BeginDisabled(context.tabletSettings.stylus.forceFullArea);
        {
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_Width)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletWidth", &context.tabletSettings.stylus.area.width, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_Height)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletHeight", &context.tabletSettings.stylus.area.height, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
        }
        {
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_OffsetX)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletOffsetX", &context.tabletSettings.stylus.area.offsetX, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_OffsetY)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletOffsetY", &context.tabletSettings.stylus.area.offsetY, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
        }
        ImGui::EndDisabled();

        ImGui::BeginGroup();
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_Orientation)));
            char const* orientations[] = {
                TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_Orientation_Left)),
                TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_Orientation_Right)),
            };
            ImGui::SetNextItemWidth(150_scaled);
            static auto orientationIndex = static_cast<int>(context.tabletSettings.stylus.handedness);

            if (context.hasChangedDeviceHandedness)
            {
                orientationIndex = static_cast<int>(context.tabletSettings.stylus.handedness);
            }

            context.hasChangedDeviceHandedness |= ImGui::Combo("##Orientations", &orientationIndex, orientations, std::size(orientations));

            if (context.hasChangedDeviceHandedness)
            {
                context.tabletSettings.stylus.handedness = Device::Handedness(orientationIndex);
            }

            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                static auto isMappingsSettingsOpen = false;

                isMappingsSettingsOpen |= ImGui::Button(TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Mappings_Title)), { 150_scaled, 0 });

                if (isMappingsSettingsOpen)
                {
                    auto [windowWidth, windowHeight] = ImGui::GetWindowSize();

                    float mappingsSettingsWidth = static_cast<float>(windowWidth)/1.5f, applicationSettingsHeight = static_cast<float>(windowHeight)/1.5f;
                    ImGui::SetNextWindowSize({ mappingsSettingsWidth, applicationSettingsHeight });
                    ImGui::SetNextWindowPos({ (static_cast<float>(windowWidth) - mappingsSettingsWidth)/2, (static_cast<float>(windowHeight) - applicationSettingsHeight)/2 });
                    ImGui::Begin(
                        TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Mappings_Title)),
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
        }

        ImGui::EndGroup();
        ImGui::BeginGroup();
        {
            context.hasChangedDeviceArea |= ImGui::Checkbox(TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_FullArea)), &context.tabletSettings.stylus.forceFullArea);
            ImGui::BeginDisabled();
            ImGui::Checkbox(TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_ForceProportions)), &context.tabletSettings.stylus.forceAspectRatio);
            ImGui::EndDisabled();
        }
        ImGui::EndGroup();
    }
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
    {
        static float devicePressureAnchors[4] = {};

        if (!context.devices.empty() && context.tabletSettings.stylus.name != "INVALID")
        {
            devicePressureAnchors[0] = context.tabletSettings.stylus.pressure.minX;
            devicePressureAnchors[1] = context.tabletSettings.stylus.pressure.minY;
            devicePressureAnchors[2] = context.tabletSettings.stylus.pressure.maxX;
            devicePressureAnchors[3] = context.tabletSettings.stylus.pressure.maxY;
        }
        else
        {
            devicePressureAnchors[0] = 0;
            devicePressureAnchors[1] = 0;
            devicePressureAnchors[2] = 1;
            devicePressureAnchors[3] = 1;
        }

        ImGui::AlignTextToFramePadding();
        context.hasChangedDevicePressure = ImGui::BezierEditor(TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_PressureCurve)), devicePressureAnchors, { 250_scaled, 250_scaled });

        if (context.hasChangedDevicePressure)
        {
            context.tabletSettings.stylus.pressure = { devicePressureAnchors[0], devicePressureAnchors[1], devicePressureAnchors[2], devicePressureAnchors[3] };
        }
    }
    ImGui::EndGroup();

    if (context.hasChangedDeviceArea && context.tabletSettings.stylus.name != "INVALID")
    {
        context.tabletSettings.stylus.area = {
            .offsetX = std::clamp(context.tabletSettings.stylus.area.offsetX, 0.f, deviceDefaultArea.width),
            .offsetY = std::clamp(context.tabletSettings.stylus.area.offsetY, 0.f, deviceDefaultArea.height),
            .width   = std::clamp(context.tabletSettings.stylus.area.width, 0.f, deviceDefaultArea.width),
            .height  = std::clamp(context.tabletSettings.stylus.area.height, 0.f, deviceDefaultArea.height)
        };
    }

    return {};
}

static Result<void> render_display_tab(Context& context)
{
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (300_scaled + ImGui::GetStyle().WindowPadding.x))/2);

    static auto displayDefaultArea = context.displays.empty() ? Display::Area {} : Display::Area { 0, 0, context.display.area.width, context.display.area.height };

    if (context.hasChangedDisplay && context.tabletSettings.display.name != "INVALID")
    {
        displayDefaultArea = Display::Area { 0, 0, context.display.area.width, context.display.area.height };
    }

    ImGui::BeginGroup();
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Display_Display)));
        auto displaysNames = fplus::transform([] (auto const& display) { return display.name.data(); }, context.displays);
        ImGui::SetNextItemWidth(300_scaled + ImGui::GetStyle().WindowPadding.x);
        static int displayIndex = context.tabletSettings.display.name == "INVALID" ? 0 : static_cast<int>(
            std::distance(context.displays.begin(), std::ranges::find(context.displays, context.tabletSettings.display.name, &Display::name))
        );

        if (context.hasChangedDisplay)
        {
            displayIndex = context.tabletSettings.display.name == "INVALID" ? 0 : static_cast<int>(
                std::distance(context.displays.begin(), std::ranges::find(context.displays, context.tabletSettings.display.name, &Display::name))
            );
        }

        context.hasChangedDisplay = ImGui::Combo("##Displays", &displayIndex, displaysNames.data(), static_cast<int>(displaysNames.size()));

        if (context.hasChangedDisplay)
        {
            context.display = context.displays.at(static_cast<size_t>(displayIndex));
            context.tabletSettings.display.name = context.display.name;
            context.tabletSettings.display.area = Display::Area { 0, 0, context.display.area.width, context.display.area.height };
            context.tabletSettings.display.forceFullArea = false;
            context.tabletSettings.display.forceAspectRatio = false;
        }

        ImGui::BeginDisabled(context.tabletSettings.display.forceFullArea);
        {
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Display_Width)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDisplayArea |= ImGui::InputFloat("##DisplayWidth", &context.tabletSettings.display.area.width, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Display_Height)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDisplayArea |= ImGui::InputFloat("##DisplayHeight", &context.tabletSettings.display.area.height, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
        }
        {
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Display_OffsetX)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDisplayArea |= ImGui::InputFloat("##DisplayOffsetX", &context.tabletSettings.display.area.offsetX, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Display_OffsetY)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDisplayArea |= ImGui::InputFloat("##DisplayOffsetY", &context.tabletSettings.display.area.offsetY, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
        }
        ImGui::EndDisabled();

        ImGui::BeginGroup();
        {
            context.hasChangedDisplayArea |= ImGui::Checkbox(TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Display_FullArea)), &context.tabletSettings.display.forceFullArea);
            ImGui::BeginDisabled();
            ImGui::Checkbox(TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Display_ForceProportions)), &context.tabletSettings.display.forceAspectRatio);
            ImGui::EndDisabled();
        }
        ImGui::EndGroup();
    }
    ImGui::EndGroup();

    if (context.hasChangedDisplayArea && context.tabletSettings.display.name != "INVALID")
    {
        context.tabletSettings.display.area = {
            .offsetX = std::clamp(context.tabletSettings.display.area.offsetX, 0.f, displayDefaultArea.width),
            .offsetY = std::clamp(context.tabletSettings.display.area.offsetY, 0.f, displayDefaultArea.height),
            .width   = std::clamp(context.tabletSettings.display.area.width, 0.f, displayDefaultArea.width),
            .height  = std::clamp(context.tabletSettings.display.area.height, 0.f, displayDefaultArea.height)
        };
    }

    return {};
}

Result<void> render_main_window(Context& context)
{
#ifdef DEBUG
    static bool shouldWarnAboutDebugBuild = true;

    if (shouldWarnAboutDebugBuild)
    {
        ImGui::PushToast("Debug", "You are running a debug build!");
        shouldWarnAboutDebugBuild = false;
    }
#endif

    static auto hasTriedToInitializeDeviceSettings = false;

    static auto messageReceiver = IPCClient::the().receive_message_async();
    auto message = messageReceiver.next();

    if (message.has_value())
    {
        auto action = magic_enum::enum_cast<UDevDevice::Action>(message->data());
        assert(action && "INVALID ACTION");

        switch (*action)
        {
            case UDevDevice::Action::BIND: {
                if (std::ranges::count(context.devices, Device::Kind::STYLUS, &Device::kind) >= 1) break;

                context.devices = TRY(get_available_devices());

                auto result = load_tablet_settings();

                if (!result.has_value())
                {
                    TRY(apply_settings_from_driver_to_context(context));

                    switch (result.error().message())
                    {
                    case SettingsError::Type::WRITE_FAILURE: break;
                    case SettingsError::Type::FILE_NOT_FOUND: {
                        ImGui::PushToast(
                            TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Warning)),
                            TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Device_Settings_Missing))
                        );
                        break;
                    }
                    case SettingsError::Type::READ_FAILURE: {
                        ImGui::PushToast(
                            TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Warning)),
                            TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Device_Settings_Load_Failed))
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
                    context.tabletSettings = *result;

                    ImGui::PushToast(
                        TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Success)),
                        TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Device_Settings_Load_Success))
                    );

                    auto stylus = std::ranges::find(context.devices, context.tabletSettings.stylus.name, &Device::name);
                    assert(stylus != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
                    context.tablet.stylus = *stylus;

                    auto pad = std::ranges::find(context.devices, context.tabletSettings.pad.name, &Device::name);
                    assert(pad != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
                    context.tablet.pad = *pad;

                    context.hasChangedDevice = true;
                    context.hasChangedDeviceHandedness = true;

                    context.display = *std::ranges::find(context.displays, context.tabletSettings.display.name, &Display::name);
                    context.hasChangedDisplay = true;

                    TRY(apply_settings_from_context_to_device(context));
                }

                break;
            }
            case UDevDevice::Action::UNBIND: {
                if (std::ranges::count(context.devices, Device::Kind::STYLUS, &Device::kind) > 1) break;

                context.devices = TRY(get_available_devices());

                auto maybeDevice = std::ranges::find(context.devices, context.tabletSettings.stylus.name, &Device::name);

                if (maybeDevice == context.devices.end())
                {
                    context.display = {};
                    context.tablet.stylus = {};
                    context.tablet.pad = {};
                    context.tabletSettings = {};
                }

                break;
            }
            case UDevDevice::Action::REMOVE: break;
            case UDevDevice::Action::ADD: break;
            case UDevDevice::Action::NONE: break;
        }
    }

    if (context.handleOutdatedDeviceSettings)
    {
        auto [windowWidth, windowHeight] = ImGui::GetWindowSize();

        float deviceSettingsMigrationWidth = 400_scaled, deviceSettingsMigrationHeight = 150_scaled;
        ImGui::SetNextWindowSize({ deviceSettingsMigrationWidth, deviceSettingsMigrationHeight });
        ImGui::SetNextWindowPos({ (static_cast<float>(windowWidth) - deviceSettingsMigrationWidth)/2, (static_cast<float>(windowHeight) - deviceSettingsMigrationHeight)/2 });
        ImGui::Begin(
            TRY(Localisation::get(context.applicationSettings.language, Localisation::Popup_Outdated_Device_Settings_Title)),
            nullptr,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
        );
        {
            auto [popupWidth, popupHeight] = ImGui::GetWindowSize();

            ImGui::BeginGroup();
            {
                for (auto messageLine :
                    ImGui::SplitToWidth(
                        TRY(Localisation::get(context.applicationSettings.language, Localisation::Popup_Outdated_Device_Settings_Text)),
                        static_cast<int>(popupWidth)
                    ))
                {
                    ImGui::Text("%s", messageLine.data());
                }
            }
            ImGui::EndGroup();

            ImGui::SetCursorPosY(popupHeight - (25_scaled + ImGui::GetStyle().WindowPadding.y));
            if (ImGui::Button(TRY(Localisation::get(context.applicationSettings.language, Localisation::Popup_Outdated_Device_Settings_Overwrite)), { 0, 25_scaled }))
            {
                ImGui::PushToast(
                    TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Success)),
                    TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Device_Settings_Overwritten))
                );

                save_tablet_settings(context.tabletSettings);
                context.handleOutdatedDeviceSettings = false;
            }

            ImGui::SameLine();

            if (ImGui::Button(TRY(Localisation::get(context.applicationSettings.language, Localisation::Popup_Outdated_Device_Settings_Migrate)), { 0, 25_scaled }))
            {
                migrate_tablet_settings(context.tabletSettings);
            }
        }
        ImGui::End();
    }

    if (context.devices.empty() && !hasTriedToInitializeDeviceSettings)
    {
        ImGui::PushToast(
            TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Warning)),
            TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Devices_Missing))
        );
        hasTriedToInitializeDeviceSettings = true;
    }

    if (!(context.devices.empty() || hasTriedToInitializeDeviceSettings))
    {
        auto result = load_tablet_settings();

        if (!result.has_value())
        {
            TRY(apply_settings_from_driver_to_context(context));

            switch (result.error().message())
            {
            case SettingsError::Type::WRITE_FAILURE: break;
            case SettingsError::Type::FILE_NOT_FOUND: {
                ImGui::PushToast(
                    TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Warning)),
                    TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Device_Settings_Missing))
                );
                save_tablet_settings(context.tabletSettings);
                break;
            }
            case SettingsError::Type::READ_FAILURE: {
                ImGui::PushToast(
                    TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Warning)),
                    TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Device_Settings_Load_Failed))
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
            context.tabletSettings = *result;

            auto stylus = std::ranges::find(context.devices, context.tabletSettings.stylus.name, &Device::name);
            assert(stylus != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
            context.tablet.stylus = *stylus;

            auto pad = std::ranges::find(context.devices, context.tabletSettings.pad.name, &Device::name);
            assert(pad != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
            context.tablet.pad = *pad;

            context.display = *std::ranges::find(context.displays, context.tabletSettings.display.name, &Display::name);

            TRY(apply_settings_from_context_to_device(context));
        }

        hasTriedToInitializeDeviceSettings = true;
    }

    ImGui::BeginDisabled(context.handleOutdatedDeviceSettings);
    ImGui::BeginGroup();
    {
        render_region_mappers(context);
    }
    ImGui::EndGroup();

    if (ImGui::BeginTabBar("##Tabs_1"))
    {
        if (ImGui::BeginTabItem(TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_Title))))
        {
            TRY(render_tablet_tab(context));
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Display_Title))))
        {
            TRY(render_display_tab(context));
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    auto previousCursorPosition = ImGui::GetCursorPos();
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - (35_scaled + ImGui::GetStyle().WindowPadding.x));
    if (ImGui::Button(TRY(Localisation::get(context.applicationSettings.language, Localisation::Save_Apply)), { 200_scaled, 35_scaled }))
    {
        ImGui::PushToast(
            TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Success)),
            TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Device_Settings_Saved))
        );
        save_tablet_settings(context.tabletSettings);
        TRY(apply_settings_from_context_to_device(context));
    }
    ImGui::SetCursorPos(previousCursorPosition);
    ImGui::EndDisabled();

    if (message.has_value())
    {
        context.hasChangedDevice = false;
        context.hasChangedDisplay = false;
    }

    return {};
}
