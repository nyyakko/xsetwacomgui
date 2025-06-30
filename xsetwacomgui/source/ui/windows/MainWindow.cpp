#define IMGUI_DEFINE_MATH_OPERATORS
#include "ui/windows/MainWindow.hpp"

#include "platform/events/USBEvent.hpp"
#include "ui/Localisation.hpp"
#include "ui/Scaling.hpp"
#include "ui/widgets/AreaMapper.hpp"

#include <fplus/fplus.hpp>
#include <imgui/extensions/imgui_bezier.hpp>
#include <imgui/extensions/imgui_text.hpp>
#include <imgui/extensions/imgui_toast.hpp>
#include <imgui/imgui.hpp>
#include <liberror/Try.hpp>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <span>

static liberror::Result<void> apply_to_device(Context const& context)
{
    TRY(libwacom::set_stylus_area(context.device.id, context.tabletSettings.device.area));
    TRY(libwacom::set_stylus_handedness(context.device.id, context.tabletSettings.device.handedness));
    TRY(libwacom::set_stylus_pressure_curve(context.device.id, context.tabletSettings.device.pressure));
    auto displayArea = context.tabletSettings.display.area;
    displayArea.offsetX += context.display.area.offsetX;
    displayArea.offsetY += context.display.area.offsetY;
    TRY(libwacom::set_stylus_output_from_display_area(context.device.id, displayArea));
    return {};
}

static liberror::Result<void> render_region_mappers(Context& context)
{
    auto [cursorX, cursorY] = ImGui::GetCursorPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    static ImVec2 displayAreaAnchors[4] { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } };
    static libwacom::Area displayDefaultArea = context.displays.empty() ? libwacom::Area {} : libwacom::Area { 0, 0, context.display.area.width, context.display.area.height };

    if (context.hasChangedDisplayArea && context.tabletSettings.display.forceFullArea && context.tabletSettings.display.name != "INVALID")
    {
        context.tabletSettings.display.area = displayDefaultArea;
    }

    if (context.hasChangedDisplay && context.tabletSettings.display.name != "INVALID")
    {
        displayDefaultArea = libwacom::Area { 0, 0, context.display.area.width, context.display.area.height };
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
    static libwacom::Area deviceDefaultArea = context.devices.empty() ? libwacom::Area {} : TRY(libwacom::get_stylus_default_area(context.device.id));

    if (context.hasChangedDeviceArea && context.tabletSettings.device.forceFullArea && context.tabletSettings.device.name != "INVALID")
    {
        context.tabletSettings.device.area = deviceDefaultArea;
    }

    if (context.hasChangedDevice && context.tabletSettings.device.name != "INVALID")
    {
        deviceDefaultArea = TRY(libwacom::get_stylus_default_area(context.device.id));
    }

    if (!context.devices.empty() && context.tabletSettings.device.name != "INVALID")
    {
        deviceAreaAnchors[0] = {
            context.tabletSettings.device.area.offsetX / deviceDefaultArea.width,
            context.tabletSettings.device.area.offsetY / deviceDefaultArea.height
        };
        deviceAreaAnchors[1] = {
            context.tabletSettings.device.area.offsetX / deviceDefaultArea.width,
            (context.tabletSettings.device.area.height + context.tabletSettings.device.area.offsetY) / deviceDefaultArea.height
        };
        deviceAreaAnchors[2] = {
            (context.tabletSettings.device.area.width + context.tabletSettings.device.area.offsetX) / deviceDefaultArea.width,
            context.tabletSettings.device.area.offsetY / deviceDefaultArea.height
        };
        deviceAreaAnchors[3] = {
            (context.tabletSettings.device.area.width + context.tabletSettings.device.area.offsetX) / deviceDefaultArea.width,
            (context.tabletSettings.device.area.height + context.tabletSettings.device.area.offsetY) / deviceDefaultArea.height
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
    context.hasChangedDeviceArea = area_mapper(TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_Device)), deviceAreaAnchors, deviceMapperSize, &deviceMapperPosition, context.tabletSettings.device.forceFullArea, context.tabletSettings.device.forceAspectRatio);
    ImGui::SetCursorPosX(cursorX);

    if (context.hasChangedDeviceArea && context.tabletSettings.device.name != "INVALID")
    {
        context.tabletSettings.device.area = {
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

static liberror::Result<void> render_tablet_tab(Context& context)
{
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (250_scaled + 300_scaled + ImGui::GetStyle().WindowPadding.x))/2);

    static libwacom::Area deviceDefaultArea = context.devices.empty() ? libwacom::Area {} : TRY(libwacom::get_stylus_default_area(context.device.id));

    if (context.hasChangedDevice && context.tabletSettings.device.name != "INVALID")
    {
        deviceDefaultArea = TRY(libwacom::get_stylus_default_area(context.device.id));
    }

    ImGui::BeginGroup();
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_Device)));
        auto deviceNames = fplus::transform([] (libwacom::Device const& device) { return device.name.data(); }, context.devices);
        ImGui::SetNextItemWidth(300_scaled + ImGui::GetStyle().WindowPadding.x);
        static int deviceIndex = context.tabletSettings.device.name == "INVALID" ? 0 : static_cast<int>(
            std::distance(context.devices.begin(), std::ranges::find(context.devices, context.tabletSettings.device.name, &libwacom::Device::name))
        );

        if (context.hasChangedDevice && (context.hasChangedDevice & USBEvent::MAGIC) == 0)
        {
            deviceIndex = context.tabletSettings.device.name == "INVALID" ? 0 : static_cast<int>(
                std::distance(context.devices.begin(), std::ranges::find(context.devices, context.tabletSettings.device.name, &libwacom::Device::name))
            );
        }

        context.hasChangedDevice = ImGui::Combo("##Device", &deviceIndex, deviceNames.data(), static_cast<int>(deviceNames.size()));

        if (context.hasChangedDevice)
        {
            context.device = context.devices.at(static_cast<size_t>(deviceIndex));
            context.tabletSettings.device.name = context.device.name;
            context.tabletSettings.device.area = TRY(libwacom::get_stylus_default_area(context.device.id));
            context.tabletSettings.device.pressure = { 0, 0, 1, 1 };
            context.tabletSettings.device.forceFullArea = false;
            context.tabletSettings.device.forceAspectRatio = false;
        }

        ImGui::BeginDisabled(context.tabletSettings.device.forceFullArea);
        {
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_Width)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletWidth", &context.tabletSettings.device.area.width, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_Height)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletHeight", &context.tabletSettings.device.area.height, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
        }
        {
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_OffsetX)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletOffsetX", &context.tabletSettings.device.area.offsetX, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_OffsetY)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletOffsetY", &context.tabletSettings.device.area.offsetY, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
        }
        ImGui::EndDisabled();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_Orientation)));
        char const* orientations[] = {
            TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_Orientation_Left)),
            TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_Orientation_Right)),
        };
        ImGui::SetNextItemWidth(150_scaled);
        static int orientationIndex = static_cast<int>(context.tabletSettings.device.handedness.to_int());

        if (context.hasChangedDeviceHandedness && (context.hasChangedDeviceHandedness & USBEvent::MAGIC) == 0)
        {
            orientationIndex = static_cast<int>(context.tabletSettings.device.handedness.to_int());
        }

        context.hasChangedDeviceHandedness = ImGui::Combo("##Orientations", &orientationIndex, orientations, std::size(orientations));

        if (context.hasChangedDeviceHandedness)
        {
            context.tabletSettings.device.handedness = libwacom::Handedness::from_int(orientationIndex);
        }

        ImGui::BeginGroup();
        {
            context.hasChangedDeviceArea |= ImGui::Checkbox(TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_FullArea)), &context.tabletSettings.device.forceFullArea);
            ImGui::BeginDisabled();
            ImGui::Checkbox(TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Tablet_ForceProportions)), &context.tabletSettings.device.forceAspectRatio);
            ImGui::EndDisabled();
        }
        ImGui::EndGroup();
    }
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
    {
        static float devicePressureAnchors[4] = {};

        if (!context.devices.empty() && context.tabletSettings.device.name != "INVALID")
        {
            devicePressureAnchors[0] = context.tabletSettings.device.pressure.minX;
            devicePressureAnchors[1] = context.tabletSettings.device.pressure.minY;
            devicePressureAnchors[2] = context.tabletSettings.device.pressure.maxX;
            devicePressureAnchors[3] = context.tabletSettings.device.pressure.maxY;
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
            context.tabletSettings.device.pressure = { devicePressureAnchors[0], devicePressureAnchors[1], devicePressureAnchors[2], devicePressureAnchors[3] };
        }
    }
    ImGui::EndGroup();

    if (context.hasChangedDeviceArea && context.tabletSettings.device.name != "INVALID")
    {
        context.tabletSettings.device.area = {
            .offsetX = std::clamp(context.tabletSettings.device.area.offsetX, 0.f, deviceDefaultArea.width),
            .offsetY = std::clamp(context.tabletSettings.device.area.offsetY, 0.f, deviceDefaultArea.height),
            .width   = std::clamp(context.tabletSettings.device.area.width, 0.f, deviceDefaultArea.width),
            .height  = std::clamp(context.tabletSettings.device.area.height, 0.f, deviceDefaultArea.height)
        };
    }

    return {};
}

static liberror::Result<void> render_display_tab(Context& context)
{
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (300_scaled + ImGui::GetStyle().WindowPadding.x))/2);

    static libwacom::Area displayDefaultArea = context.displays.empty() ? libwacom::Area {} : libwacom::Area { 0, 0, context.display.area.width, context.display.area.height };

    if (context.hasChangedDisplay && context.tabletSettings.display.name != "INVALID")
    {
        displayDefaultArea = libwacom::Area { 0, 0, context.display.area.width, context.display.area.height };
    }

    ImGui::BeginGroup();
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Window_Main_Tabs_Display_Display)));
        auto displayNames = fplus::transform([] (Display const& display) { return fmt::format("{} ({}x{})", display.name, display.area.width, display.area.height); }, context.displays);
        auto displayNamesData = fplus::transform([] (std::string const& name) { return name.data(); }, displayNames);
        ImGui::SetNextItemWidth(300_scaled + ImGui::GetStyle().WindowPadding.x);
        static int displayIndex = context.tabletSettings.display.name == "INVALID" ? 0 : static_cast<int>(
            std::distance(context.displays.begin(), std::ranges::find(context.displays, context.tabletSettings.display.name, &Display::name))
        );

        if (context.hasChangedDisplay && (context.hasChangedDisplay & USBEvent::MAGIC) == 0)
        {
            displayIndex = context.tabletSettings.display.name == "INVALID" ? 0 : static_cast<int>(
                std::distance(context.displays.begin(), std::ranges::find(context.displays, context.tabletSettings.display.name, &Display::name))
            );
        }

        context.hasChangedDisplay = ImGui::Combo("##Displays", &displayIndex, displayNamesData.data(), static_cast<int>(displayNamesData.size()));

        if (context.hasChangedDisplay)
        {
            context.display = context.displays.at(static_cast<size_t>(displayIndex));
            context.tabletSettings.display.name = context.display.name;
            context.tabletSettings.display.area = libwacom::Area { 0, 0, context.display.area.width, context.display.area.height };
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

liberror::Result<void> render_main_window(Context& context)
{
#ifdef DEBUG
    static bool shouldWarnAboutDebugBuild = true;

    if (shouldWarnAboutDebugBuild)
    {
        ImGui::PushToast("Debug", "You are running a debug build!");
        shouldWarnAboutDebugBuild = false;
    }
#endif

    static bool hasSubscribedUsbActionListeners = false;
    static bool hasTriedToInitializeDeviceSettings = false;

    static USBEvent usbAction {};

    if (!hasSubscribedUsbActionListeners)
    {
        usbAction.subscribe([&] (std::string_view, USBEvent::Action event) -> liberror::Result<void> {
            if (event != USBEvent::Action::UNBIND) return {};

            auto hadMoreThanOneDevice = context.devices.size() > 1;
            context.devices = fplus::keep_if([] (auto&& device) { return device.kind == libwacom::Device::Kind::STYLUS; }, TRY(libwacom::get_available_devices()));
            if (hadMoreThanOneDevice) return {};

            auto maybeDevice = std::ranges::find(context.devices, context.tabletSettings.device.name, &libwacom::Device::name);
            if (maybeDevice == context.devices.end())
            {
                context.display = {};
                context.device = {};
                context.tabletSettings = {};
            }

            return {};
        });

        usbAction.subscribe([&] (std::string_view, USBEvent::Action event) -> liberror::Result<void> {
            if (event != USBEvent::Action::BIND) return {};

            auto hadAtleastOneDevice = !context.devices.empty();
            context.devices = fplus::keep_if([] (auto&& device) { return device.kind == libwacom::Device::Kind::STYLUS; }, TRY(libwacom::get_available_devices()));
            if (hadAtleastOneDevice) return {};

            auto result = load_tablet_settings(context.tabletSettings);
            if (!result.has_value())
            {
                context.device = context.devices.back();
                context.hasChangedDevice = 0xFF ^ USBEvent::MAGIC;
                context.hasChangedDeviceHandedness = 0xFF ^ USBEvent::MAGIC;
                context.display = TRY(get_primary_display());
                context.hasChangedDisplay = 0xFF ^ USBEvent::MAGIC;
                context.tabletSettings.device.name = context.device.name;
                context.tabletSettings.device.area = TRY(libwacom::get_stylus_area(context.device.id));
                context.tabletSettings.device.pressure = TRY(libwacom::get_stylus_pressure_curve(context.device.id));
                context.tabletSettings.display.name = context.display.name;
                context.tabletSettings.display.area = { 0, 0, context.display.area.width, context.display.area.height };

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
            else if (context.devices.back().name == context.tabletSettings.device.name)
            {
                ImGui::PushToast(
                    TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Success)),
                    TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Device_Settings_Load_Success))
                );
                context.device = context.devices.back();
                context.hasChangedDevice = 0xFF ^ USBEvent::MAGIC;
                context.hasChangedDeviceHandedness = 0xFF ^ USBEvent::MAGIC;
                context.display = *std::ranges::find(context.displays, context.tabletSettings.display.name, &Display::name);
                context.hasChangedDisplay = 0xFF ^ USBEvent::MAGIC;
                TRY(apply_to_device(context));
            }

            return {};
        });

        hasSubscribedUsbActionListeners = true;
    }

    TRY(usbAction.update());

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
        hasTriedToInitializeDeviceSettings = true;
        ImGui::PushToast(
            TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Warning)),
            TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Devices_Missing))
        );
    }

    if (!(context.devices.empty() || hasTriedToInitializeDeviceSettings))
    {
        hasTriedToInitializeDeviceSettings = true;
        auto result = load_tablet_settings(context.tabletSettings);
        if (!result.has_value())
        {
            context.device = context.devices.back();
            context.hasChangedDevice = true;
            context.hasChangedDeviceHandedness = true;
            context.display = TRY(get_primary_display());
            context.hasChangedDisplay = true;
            context.tabletSettings.device.name = context.device.name;
            context.tabletSettings.device.area = TRY(libwacom::get_stylus_area(context.device.id));
            context.tabletSettings.device.pressure = TRY(libwacom::get_stylus_pressure_curve(context.device.id));
            context.tabletSettings.device.forceFullArea = false;
            context.tabletSettings.device.forceAspectRatio = false;
            context.tabletSettings.display.name = context.display.name;
            context.tabletSettings.display.area = { 0, 0, context.display.area.width, context.display.area.height };
            context.tabletSettings.display.forceFullArea = false;
            context.tabletSettings.display.forceAspectRatio = false;

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
            context.display = *std::ranges::find(context.displays, context.tabletSettings.display.name, &Display::name);
            context.device  = *std::ranges::find(context.devices, context.tabletSettings.device.name, &libwacom::Device::name);
        }
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
        save_tablet_settings(context.tabletSettings);
        ImGui::PushToast(
            TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Success)),
            TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Device_Settings_Saved))
        );
        TRY(apply_to_device(context));
    }
    ImGui::SetCursorPos(previousCursorPosition);
    ImGui::EndDisabled();

    if (context.hasChangedDevice && (context.hasChangedDevice & USBEvent::MAGIC) == 0) context.hasChangedDevice = false;
    if (context.hasChangedDisplay && (context.hasChangedDisplay & USBEvent::MAGIC) == 0) context.hasChangedDisplay = false;

    return {};
}
