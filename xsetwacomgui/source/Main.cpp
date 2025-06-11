#define IMGUI_DEFINE_MATH_OPERATORS

#include <spdlog/spdlog.h>

#include "AreaMapper.hpp"
#include "Environment.hpp"
#include "Localisation.hpp"
#include "Monitor.hpp"
#include "Scaling.hpp"
#include "Settings.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image/stb_image.h"

#include <imgui/extensions/imgui_toast.hpp>
#include <imgui/extensions/imgui_bezier.hpp>
#include <imgui/imgui.hpp>
#include <imgui/imgui_impl_glfw.hpp>
#include <imgui/imgui_impl_opengl3.hpp>
#include <libwacom/Device.hpp>
#include <liberror/Try.hpp>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <fplus/fplus.hpp>

#include <filesystem>
#include <cstdlib>
#include <span>
#include <ranges>
#include <algorithm>

std::vector<std::pair<std::string, std::filesystem::path>> get_available_fonts()
{
    std::vector<std::pair<std::string, std::filesystem::path>> fonts {
        { "default", "default" }
    };

    for (auto const& fontHome : std::array {
        get_system_home_path() / ".fonts",
        get_system_home_path() / ".local/share/fonts",
        std::filesystem::path("/usr/share/fonts"),
        std::filesystem::path("/usr/local/share/fonts"),
    })
    {
        if (!std::filesystem::exists(fontHome)) continue;

        for (auto const& entry : std::filesystem::recursive_directory_iterator(fontHome))
        {
            if (entry.path().extension() == ".ttf") fonts.push_back({ entry.path().filename(), entry.path() });
        }
    }

    return fonts;
}

void render_settings_popup_appearance_tab(ApplicationSettings& settings)
{
    ImGui::Text("%s", Localisation::get(settings.language, Localisation::Popup_Settings_Tabs_Appearance_Theme));
    char const* themes[] = {
        Localisation::get(settings.language, Localisation::Popup_Settings_Tabs_Appearance_Theme_Dark),
        Localisation::get(settings.language, Localisation::Popup_Settings_Tabs_Appearance_Theme_Light)
    };
    static int themeIndex = static_cast<int>(settings.theme);
    auto hasChangedUITheme = ImGui::Combo("##Theme", &themeIndex, themes, std::size(themes));

    if (hasChangedUITheme)
    {
        settings.theme = ApplicationSettings::Theme::from_int(themeIndex);
    }

    ImGui::Text("%s", Localisation::get(settings.language, Localisation::Popup_Settings_Tabs_Appearance_Font));
    static auto fonts = get_available_fonts();
    static auto fontsData = fplus::transform([] (auto const& font) { return font.first.data(); }, fonts );
    static auto fontIndex = settings.font == "default" ? 0 : static_cast<int>(
        std::distance(fonts.begin(), std::ranges::find(fonts, std::filesystem::path(settings.font), &decltype(fonts)::value_type::second))
    );
    auto hasChangedUIFont = ImGui::Combo("##Font", &fontIndex, fontsData.data(), static_cast<int>(fontsData.size()));

    if (hasChangedUIFont)
    {
        settings.font = fonts.at(static_cast<size_t>(fontIndex)).second;
    }
}

void render_settings_popup_display_tab(ApplicationSettings& settings)
{
    ImGui::Text("%s", Localisation::get(settings.language, Localisation::Popup_Settings_Tabs_Display_Scale));
    static float scale = settings.scale;
    auto hasChangedUIScale = ImGui::InputFloat("##UiScale", &scale, 0.1f);

    if (hasChangedUIScale)
    {
        scale = ImClamp(scale, 1.0f, 10.f);
        settings.scale = scale;
    }
}

void render_settings_popup_language_tab(ApplicationSettings& settings)
{
    ImGui::Text("%s", Localisation::get(settings.language, Localisation::Popup_Settings_Tabs_Language_Language));
    static char const* languages[] = {
        "en_us",
        "pt_br",
        "ru_ru",
    };
    static int languageIndex = static_cast<int>(
        std::distance(&languages[0], std::ranges::find(&languages[0], &languages[std::size(languages)], settings.language))
    );
    auto hasChangedUILanguage = ImGui::Combo("##Language", &languageIndex, languages, std::size(languages));

    if (hasChangedUILanguage)
    {
        settings.language = languages[languageIndex];
    }
}

void render_settings_popup(ApplicationSettings& settings)
{
    if (ImGui::BeginTabBar("##Tabs_2"))
    {
        if (ImGui::BeginTabItem(Localisation::get(settings.language, Localisation::Popup_Settings_Tabs_Appearance_Title)))
        {
            render_settings_popup_appearance_tab(settings);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(Localisation::get(settings.language, Localisation::Popup_Settings_Tabs_Display_Title)))
        {
            render_settings_popup_display_tab(settings);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(Localisation::get(settings.language, Localisation::Popup_Settings_Tabs_Language_Title)))
        {
            render_settings_popup_language_tab(settings);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    auto previousCursorPosition = ImGui::GetCursorPos();
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - (25_scaled + ImGui::GetStyle().WindowPadding.x));
    if (ImGui::Button(Localisation::get(settings.language, Localisation::Save), { 100_scaled, 25_scaled }))
    {
        if (save_application_settings(settings))
        {
            ImGui::PushToast(Localisation::get(settings.language, Localisation::Toast_Success), Localisation::get(settings.language, Localisation::Toast_Application_Settings_Saved));
        }
    }
    ImGui::SetCursorPos(previousCursorPosition);
}

void render_goddess()
{
    static auto width = 0, height = 0;
    static auto channels = 0;
    static auto image = stbi_load((DATA_PATH / "resources/images/jahy.png").c_str(), &width, &height, &channels, STBI_rgb_alpha);
    static GLuint imageTexture;

    if (image != nullptr)
    {
        glGenTextures(1, &imageTexture);
        glBindTexture(GL_TEXTURE_2D, imageTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        stbi_image_free(image);
        image = nullptr;
    }

    static ImVec2 frameDimensions { static_cast<float>(width) * 70/100, static_cast<float>(height) * 70/100 };
    ImGui::SetCursorPos({ (ImGui::GetWindowWidth() - frameDimensions.x) / 2, (ImGui::GetWindowHeight() - frameDimensions.y) / 2 });
    ImGui::Image(imageTexture, frameDimensions);
}

struct Context
{
    libwacom::Device device;
    libwacom::Area deviceDefaultArea;

    Monitor monitor;
    libwacom::Area monitorDefaultArea;

    bool hasChangedDevice = false;
    bool hasChangedDeviceArea = false;
    bool hasChangedDevicePressure = false;
    bool hasChangedMonitor = false;
    bool hasChangedMonitorArea = false;
};

liberror::Result<void> set_settings_to_device(libwacom::Device const& device, Monitor const& monitor, DeviceSettings const& settings)
{
    TRY(libwacom::set_stylus_area(device.id, settings.deviceArea));
    TRY(libwacom::set_stylus_pressure_curve(device.id, settings.devicePressure));
    TRY(libwacom::set_stylus_output_from_display_area(device.id, {
        monitor.offsetX + settings.monitorArea.offsetX,
        monitor.offsetY + settings.monitorArea.offsetY,
        settings.monitorArea.width,
        settings.monitorArea.height,
    }));
    return {};
}

void render_region_mappers(Context& context, DeviceSettings& deviceSettings, std::vector<libwacom::Device> const& devices, std::vector<Monitor> const& monitors, ApplicationSettings const& applicationSettings)
{
    auto [cursorPosX, cursorPosY] = ImGui::GetCursorPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    static ImVec2 monitorAreaAnchors[4] { { 0, 0 }, { 0, 1 }, { 1, 0 }, { 1, 1 } };

    if (!monitors.empty())
    {
        monitorAreaAnchors[0] = { deviceSettings.monitorArea.offsetX / context.monitorDefaultArea.width, deviceSettings.monitorArea.offsetY / context.monitorDefaultArea.height };
        monitorAreaAnchors[1] = { deviceSettings.monitorArea.offsetX / context.monitorDefaultArea.width, (deviceSettings.monitorArea.height + deviceSettings.monitorArea.offsetY) / context.monitorDefaultArea.height };
        monitorAreaAnchors[2] = { (deviceSettings.monitorArea.width + deviceSettings.monitorArea.offsetX) / context.monitorDefaultArea.width, deviceSettings.monitorArea.offsetY / context.monitorDefaultArea.height };
        monitorAreaAnchors[3] = { (deviceSettings.monitorArea.width + deviceSettings.monitorArea.offsetX) / context.monitorDefaultArea.width, (deviceSettings.monitorArea.height + deviceSettings.monitorArea.offsetY) / context.monitorDefaultArea.height };
    }
    else
    {
        monitorAreaAnchors[0] = { 0, 0 };
        monitorAreaAnchors[1] = { 0, 1 };
        monitorAreaAnchors[2] = { 1, 0 };
        monitorAreaAnchors[3] = { 1, 1 };
    }

    static const ImVec2 monitorMapperSize { 20 * 16_scaled, 20 * 9_scaled };
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - monitorMapperSize.x)/2);
    static ImRect monitorMapperPosition {};
    context.hasChangedMonitorArea |= area_mapper(Localisation::get(applicationSettings.language, Localisation::Tabs_Monitor_Monitor), monitorAreaAnchors, monitorMapperSize, &monitorMapperPosition, deviceSettings.monitorForceFullArea, deviceSettings.monitorForceAspectRatio);
    ImGui::SetCursorPosX(cursorPosX);

    if (context.hasChangedMonitorArea)
    {
        deviceSettings.monitorArea = {
            .offsetX = monitorAreaAnchors[0].x * context.monitorDefaultArea.width,
            .offsetY = monitorAreaAnchors[0].y * context.monitorDefaultArea.height,
            .width   = (monitorAreaAnchors[2].x - monitorAreaAnchors[0].x) * context.monitorDefaultArea.width,
            .height  = (monitorAreaAnchors[3].y - monitorAreaAnchors[2].y) * context.monitorDefaultArea.height
        };
    }

    if (context.hasChangedMonitorArea && deviceSettings.monitorForceFullArea)
    {
        deviceSettings.monitorArea = context.monitorDefaultArea;
    }

    static ImVec2 deviceAreaAnchors[4] { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } };

    if (!devices.empty())
    {
        deviceAreaAnchors[0] = { deviceSettings.deviceArea.offsetX / context.deviceDefaultArea.width, deviceSettings.deviceArea.offsetY / context.deviceDefaultArea.height };
        deviceAreaAnchors[1] = { deviceSettings.deviceArea.offsetX / context.deviceDefaultArea.width, (deviceSettings.deviceArea.height + deviceSettings.deviceArea.offsetY) / context.deviceDefaultArea.height };
        deviceAreaAnchors[2] = { (deviceSettings.deviceArea.width + deviceSettings.deviceArea.offsetX) / context.deviceDefaultArea.width, deviceSettings.deviceArea.offsetY / context.deviceDefaultArea.height };
        deviceAreaAnchors[3] = { (deviceSettings.deviceArea.width + deviceSettings.deviceArea.offsetX) / context.deviceDefaultArea.width, (deviceSettings.deviceArea.height + deviceSettings.deviceArea.offsetY) / context.deviceDefaultArea.height };
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
    context.hasChangedDeviceArea |= area_mapper(Localisation::get(applicationSettings.language, Localisation::Tabs_Tablet_Device), deviceAreaAnchors, deviceMapperSize, &deviceMapperPosition, deviceSettings.deviceForceFullArea, deviceSettings.deviceForceAspectRatio);
    ImGui::SetCursorPosX(cursorPosX);

    if (context.hasChangedDeviceArea)
    {
        deviceSettings.deviceArea = {
            .offsetX = deviceAreaAnchors[0].x * context.deviceDefaultArea.width,
            .offsetY = deviceAreaAnchors[0].y * context.deviceDefaultArea.height,
            .width   = (deviceAreaAnchors[2].x - deviceAreaAnchors[0].x) * context.deviceDefaultArea.width,
            .height  = (deviceAreaAnchors[3].y - deviceAreaAnchors[2].y) * context.deviceDefaultArea.height
        };
    }

    if (context.hasChangedDeviceArea && deviceSettings.deviceForceFullArea)
    {
        deviceSettings.deviceArea = context.deviceDefaultArea;
    }

    for (auto [monitorAnchor, deviceAnchor] : fplus::zip(std::span<ImVec2>(monitorAreaAnchors, 4), std::span<ImVec2>(deviceAreaAnchors, 4)))
    {
        auto p1 = monitorAnchor * (monitorMapperPosition.Max - monitorMapperPosition.Min) + monitorMapperPosition.Min;
        auto p2 = deviceAnchor * (deviceMapperPosition.Max - deviceMapperPosition.Min) + deviceMapperPosition.Min;
        drawList->AddLine(p1, p2, ImColor(255, 0, 0, 127), 2.f);
    }
}

liberror::Result<void> render_tablet_settings_tab(Context& context, DeviceSettings& deviceSettings, std::vector<libwacom::Device> const& devices, ApplicationSettings const& applicationSettings)
{
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (250_scaled + 300_scaled + ImGui::GetStyle().WindowPadding.x))/2);

    ImGui::BeginGroup();
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", Localisation::get(applicationSettings.language, Localisation::Tabs_Tablet_Device));
        auto deviceNames = fplus::transform([] (libwacom::Device const& device) { return device.name.data(); }, devices);
        ImGui::SetNextItemWidth(300_scaled + ImGui::GetStyle().WindowPadding.x);
        static int deviceIndex;
        context.hasChangedDevice = ImGui::Combo("##Device", &deviceIndex, deviceNames.data(), static_cast<int>(deviceNames.size()));

        if (context.hasChangedDevice)
        {
            context.device = devices.at(static_cast<size_t>(deviceIndex));
            context.deviceDefaultArea = TRY(libwacom::get_stylus_default_area(context.device.id));
            deviceSettings.deviceArea = context.deviceDefaultArea;
        }

        {
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", Localisation::get(applicationSettings.language, Localisation::Tabs_Tablet_Width));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletWidth", &deviceSettings.deviceArea.width, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", Localisation::get(applicationSettings.language, Localisation::Tabs_Tablet_Height));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletHeight", &deviceSettings.deviceArea.height, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
        }
        {
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", Localisation::get(applicationSettings.language, Localisation::Tabs_Tablet_OffsetX));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletOffsetX", &deviceSettings.deviceArea.offsetX, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", Localisation::get(applicationSettings.language, Localisation::Tabs_Tablet_OffsetY));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletOffsetY", &deviceSettings.deviceArea.offsetY, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
        }

        context.hasChangedDeviceArea |= ImGui::Checkbox(Localisation::get(applicationSettings.language, Localisation::Tabs_Tablet_FullArea), &deviceSettings.deviceForceFullArea);
        ImGui::BeginDisabled();
        ImGui::Checkbox(Localisation::get(applicationSettings.language, Localisation::Tabs_Tablet_ForceProportions), &deviceSettings.deviceForceAspectRatio);
        ImGui::EndDisabled();
    }
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
    {
        static float devicePressureAnchors[4] = {};

        if (!devices.empty())
        {
            devicePressureAnchors[0] = deviceSettings.devicePressure.minX;
            devicePressureAnchors[1] = deviceSettings.devicePressure.minY;
            devicePressureAnchors[2] = deviceSettings.devicePressure.maxX;
            devicePressureAnchors[3] = deviceSettings.devicePressure.maxY;
        }
        else
        {
            devicePressureAnchors[0] = 0;
            devicePressureAnchors[1] = 0;
            devicePressureAnchors[2] = 1;
            devicePressureAnchors[3] = 1;
        }

        ImGui::AlignTextToFramePadding();
        context.hasChangedDevicePressure = ImGui::BezierEditor(Localisation::get(applicationSettings.language, Localisation::Tabs_Tablet_PressureCurve), devicePressureAnchors, { 250_scaled, 250_scaled });

        if (context.hasChangedDevicePressure)
        {
            deviceSettings.devicePressure = { devicePressureAnchors[0], devicePressureAnchors[1], devicePressureAnchors[2], devicePressureAnchors[3] };
        }
    }
    ImGui::EndGroup();

    return {};
}

liberror::Result<void> render_monitor_settings_tab(Context& context, DeviceSettings& deviceSettings, std::vector<Monitor> const& monitors, ApplicationSettings const& applicationSettings)
{
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (300_scaled + ImGui::GetStyle().WindowPadding.x))/2);

    ImGui::BeginGroup();
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", Localisation::get(applicationSettings.language, Localisation::Tabs_Monitor_Monitor));
        auto monitorNames = fplus::transform([] (Monitor const& monitor) { return fmt::format("{} ({}x{})", monitor.name, monitor.width, monitor.height); }, monitors);
        auto monitorNamesData = fplus::transform([] (std::string const& name) { return name.data(); }, monitorNames);
        ImGui::SetNextItemWidth(300_scaled + ImGui::GetStyle().WindowPadding.x);
        static int monitorIndex;
        context.hasChangedMonitor = ImGui::Combo("##Monitors", &monitorIndex, monitorNamesData.data(), static_cast<int>(monitorNamesData.size()));

        if (context.hasChangedMonitor)
        {
            context.monitor = monitors.at(static_cast<size_t>(monitorIndex));
            context.monitorDefaultArea = libwacom::Area { 0, 0, context.monitor.width, context.monitor.height };
            deviceSettings.monitorArea = context.monitorDefaultArea;
        }

        {
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", Localisation::get(applicationSettings.language, Localisation::Tabs_Monitor_Width));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedMonitorArea |= ImGui::InputFloat("##MonitorWidth", &deviceSettings.monitorArea.width, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", Localisation::get(applicationSettings.language, Localisation::Tabs_Monitor_Height));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedMonitorArea |= ImGui::InputFloat("##MonitorHeight", &deviceSettings.monitorArea.height, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
        }
        {
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", Localisation::get(applicationSettings.language, Localisation::Tabs_Monitor_OffsetX));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedMonitorArea |= ImGui::InputFloat("##MonitorOffsetX", &deviceSettings.monitorArea.offsetX, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", Localisation::get(applicationSettings.language, Localisation::Tabs_Monitor_OffsetY));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedMonitorArea |= ImGui::InputFloat("##MonitorOffsetY", &deviceSettings.monitorArea.offsetY, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
        }

        context.hasChangedMonitorArea |= ImGui::Checkbox(Localisation::get(applicationSettings.language, Localisation::Tabs_Monitor_FullArea), &deviceSettings.monitorForceFullArea);
        ImGui::BeginDisabled();
        ImGui::Checkbox(Localisation::get(applicationSettings.language, Localisation::Tabs_Monitor_ForceProportions), &deviceSettings.monitorForceAspectRatio);
        ImGui::EndDisabled();
    }
    ImGui::EndGroup();

    return {};
}

liberror::Result<void> render_window(DeviceSettings& deviceSettings, std::vector<libwacom::Device> const& devices, std::vector<Monitor> const& monitors, ApplicationSettings const& applicationSettings)
{
    static Context context = [&] () {
        libwacom::Device device = devices.empty() ? libwacom::Device {} : devices.front();
        libwacom::Area deviceDefaultArea = devices.empty() ? libwacom::Area {} : MUST(libwacom::get_stylus_default_area(device.id));
        Monitor monitor = *std::ranges::find_if(monitors, &Monitor::primary);
        libwacom::Area monitorDefaultArea = monitors.empty() ? libwacom::Area {} : libwacom::Area { 0, 0, monitor.width, monitor.height };
        return Context { device, deviceDefaultArea, monitor, monitorDefaultArea };
    }();

    if (devices.empty() && deviceSettings.devicePressure.minX == -1 && deviceSettings.devicePressure.minY == -1 && deviceSettings.deviceArea.width == -1 && deviceSettings.deviceArea.height == -1)
    {
        ImGui::PushToast(Localisation::get(applicationSettings.language, Localisation::Toast_Warning), Localisation::get(applicationSettings.language, Localisation::Toast_Devices_Missing));
        deviceSettings.deviceArea = { 0, 0, 0, 0 };
        deviceSettings.devicePressure = { 0, 0, 1, 1 };
        deviceSettings.monitorArea = context.monitorDefaultArea;
    }

    if (!devices.empty() && deviceSettings.devicePressure.minX == -1 && deviceSettings.devicePressure.minY == -1 && deviceSettings.deviceArea.width == -1 && deviceSettings.deviceArea.height == -1)
    {
        if (std::filesystem::exists(DEVICE_SETTINGS_FILE))
        {
            if (!load_device_settings(deviceSettings))
            {
                ImGui::PushToast(Localisation::get(applicationSettings.language, Localisation::Toast_Warning), Localisation::get(applicationSettings.language, Localisation::Toast_Device_Settings_Load_Failed));
            }
        }
        else
        {
            ImGui::PushToast(Localisation::get(applicationSettings.language, Localisation::Toast_Warning), Localisation::get(applicationSettings.language, Localisation::Toast_Device_Settings_Missing));
            deviceSettings.deviceName = context.device.name;
            deviceSettings.deviceArea = MUST(libwacom::get_stylus_area(context.device.id));
            deviceSettings.devicePressure = MUST(libwacom::get_stylus_pressure_curve(context.device.id));
            deviceSettings.monitorArea = context.monitorDefaultArea;
            deviceSettings.monitorName = context.monitor.name;
        }
    }

    ImGui::BeginGroup();
    {
        render_region_mappers(context, deviceSettings, devices, monitors, applicationSettings);
    }
    ImGui::EndGroup();

    if (ImGui::BeginTabBar("##Tabs_1"))
    {
        if (ImGui::BeginTabItem(Localisation::get(applicationSettings.language, Localisation::Tabs_Tablet_Title)))
        {
            TRY(render_tablet_settings_tab(context, deviceSettings, devices, applicationSettings));
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(Localisation::get(applicationSettings.language, Localisation::Tabs_Monitor_Title)))
        {
            TRY(render_monitor_settings_tab(context, deviceSettings, monitors, applicationSettings));
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    auto previousCursorPosition = ImGui::GetCursorPos();
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - (35_scaled + ImGui::GetStyle().WindowPadding.x));
    if (ImGui::Button(Localisation::get(applicationSettings.language, Localisation::Save_Apply), { 200_scaled, 35_scaled }))
    {
        if (save_device_settings(deviceSettings))
        {
            ImGui::PushToast(Localisation::get(applicationSettings.language, Localisation::Toast_Success), Localisation::get(applicationSettings.language, Localisation::Toast_Device_Settings_Saved));
        }

        TRY(set_settings_to_device(context.device, context.monitor, deviceSettings));
    }
    ImGui::SetCursorPos(previousCursorPosition);

    return {};
}

int main(int argc, char const** argv)
{
    auto arguments =
        std::span<char const*>(argv, size_t(argc))
            | std::views::transform([] (auto arg) { return std::string_view(arg); });

    std::vector<Monitor> monitors = MUST(get_available_monitors());

    std::vector<libwacom::Device> devices = MUST(libwacom::get_available_devices());
    devices = fplus::keep_if([] (auto&& device) { return device.kind == libwacom::Device::Kind::STYLUS; }, devices);

    DeviceSettings deviceSettings {
        .deviceName = "INVALID",
        .deviceArea = { -1, -1, -1, -1 },
        .devicePressure = { -1, -1, -1, -1 },
        .deviceForceFullArea = false,
        .deviceForceAspectRatio = false,
        .monitorName = "INVALID",
        .monitorArea = { -1, -1, -1, -1 },
        .monitorForceFullArea = false,
        .monitorForceAspectRatio = false
    };

    if (!(std::filesystem::exists(SETTINGS_PATH) || std::filesystem::create_directory(SETTINGS_PATH)))
    {
        spdlog::error("Failed to create settings directory");
        return EXIT_FAILURE;
    }

    if (std::find(arguments.begin(), arguments.end(), "--help") != arguments.end())
    {
        fmt::println("A graphical xsetwacom wrapper for ease of use.");
        fmt::println("Usage:");
        fmt::println("  xsetwacomgui [OPTION...]");
        fmt::println("");
        fmt::println("  --no-gui        Launches the program without the UI. This is intended for");
        fmt::println("                  loading saved device settings on system boot.");
        return EXIT_SUCCESS;
    }

    if (std::find(arguments.begin(), arguments.end(), "--no-gui") != arguments.end())
    {
        if (!std::filesystem::exists(DEVICE_SETTINGS_FILE))
        {
            spdlog::error("Device settings could not be found");
            return EXIT_FAILURE;
        }

        if (!load_device_settings(deviceSettings))
        {
            spdlog::error("Failed to load device settings");
            return EXIT_FAILURE;
        }

        if (devices.empty() || monitors.empty())
        {
            return EXIT_SUCCESS;
        }

        auto device  = devices.front();
        auto monitor = *std::ranges::find_if(monitors, &Monitor::primary);

        MUST(set_settings_to_device(device, monitor, deviceSettings));

        fmt::println("Device settings loaded successfully");

        return EXIT_SUCCESS;
    }

    ApplicationSettings applicationSettings {
        .scale = 1.0,
        .theme = ApplicationSettings::Theme::DARK,
        .language = "en_us",
        .font = "default",
    };

    if (!std::filesystem::exists(APPLICATION_SETTINGS_FILE))
    {
        save_application_settings(applicationSettings);
    }
    else
    {
        if (!load_application_settings(applicationSettings))
        {
            spdlog::error("Failed to load application settings");
            return EXIT_FAILURE;
        }

        set_scale(applicationSettings.scale);
    }

    if (!glfwInit())
    {
        spdlog::error("Failed to initialize glfw");
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    auto window = glfwCreateWindow(static_cast<int>(800_scaled), static_cast<int>(815_scaled), NAME, nullptr, nullptr);

    glfwMakeContextCurrent(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    auto& io = ImGui::GetIO();

    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    ImFont* font = nullptr;
    if (applicationSettings.font != "default")
    {
        font = io.Fonts->AddFontFromFileTTF(applicationSettings.font.data(), 20_scaled, nullptr, io.Fonts->GetGlyphRangesDefault());
    }

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            break;
        }

        if (applicationSettings.theme == ApplicationSettings::Theme::DARK)
        {
            ImGui::StyleColorsDark();
        }
        else
        {
            ImGui::StyleColorsLight();
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::PushFont(font);
        {
            int windowWidth, windowHeight;
            glfwGetWindowSize(window, &windowWidth, &windowHeight);
            ImGui::SetNextWindowPos({});
            ImGui::SetNextWindowSize({ static_cast<float>(windowWidth), static_cast<float>(windowHeight) });
            ImGui::Begin(NAME, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);
            ImGui::RenderToasts();
            {
                static bool isApplicationSettingsOpen = false;
                static bool isGoddessOpen = false;

                if (ImGui::BeginMenuBar())
                {
                    if (ImGui::BeginMenu(Localisation::get(applicationSettings.language, Localisation::MenuBar_Settings)))
                    {
                        if (ImGui::MenuItem(Localisation::get(applicationSettings.language, Localisation::MenuBar_Settings_Application)))
                        {
                            isApplicationSettingsOpen = true;
                        }

                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu(Localisation::get(applicationSettings.language, Localisation::MenuBar_Other)))
                    {
                        if (ImGui::MenuItem(Localisation::get(applicationSettings.language, Localisation::MenuBar_Other_Goddess)))
                        {
                            isGoddessOpen = true;
                        }

                        ImGui::EndMenu();
                    }

                    ImGui::EndMenuBar();
                }

                if (isApplicationSettingsOpen)
                {
                    float applicationSettingsWidth = static_cast<float>(windowWidth)/1.5f, applicationSettingsHeight = static_cast<float>(windowHeight)/1.5f;
                    ImGui::SetNextWindowSize({ applicationSettingsWidth, applicationSettingsHeight });
                    ImGui::SetNextWindowPos({ (static_cast<float>(windowWidth) - applicationSettingsWidth)/2, (static_cast<float>(windowHeight) - applicationSettingsHeight)/2 });
                    ImGui::Begin(
                        Localisation::get(applicationSettings.language, Localisation::MenuBar_Settings_Application),
                        &isApplicationSettingsOpen,
                        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
                    );
                    {
                        render_settings_popup(applicationSettings);
                    }
                    ImGui::End();
                }

                if (isGoddessOpen)
                {
                    float goddessWidth = static_cast<float>(windowWidth)/1.5f, goddessHeight = static_cast<float>(windowHeight)/1.5f;
                    ImGui::SetNextWindowSize({ goddessWidth, goddessHeight });
                    ImGui::SetNextWindowPos({ (static_cast<float>(windowWidth) - goddessWidth)/2, (static_cast<float>(windowHeight) - goddessHeight)/2 });
                    ImGui::Begin(
                        Localisation::get(applicationSettings.language, Localisation::MenuBar_Other_Goddess),
                        &isGoddessOpen,
                        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
                    );
                    {
                        render_goddess();
                    }
                    ImGui::End();
                }

                ImGui::BeginDisabled(devices.empty());
                {
                    MUST(render_window(deviceSettings, devices, monitors, applicationSettings));
                }
                ImGui::EndDisabled();
            }
            ImGui::End();
        }
        ImGui::PopFont();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);

    glfwTerminate();

    return EXIT_SUCCESS;
}
