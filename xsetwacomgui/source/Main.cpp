#include <algorithm>
#define IMGUI_DEFINE_MATH_OPERATORS

#include <spdlog/spdlog.h>

#include "Localisation.hpp"
#include "AreaMapper.hpp"
#include "Monitor.hpp"
#include "Settings.hpp"

#include <imgui/extensions/imgui_toast.hpp>
#include <imgui/extensions/imgui_bezier.hpp>
#include <imgui/imgui.hpp>
#include <imgui/imgui_impl_glfw.hpp>
#include <imgui/imgui_impl_opengl3.hpp>
#include <libwacom/Device.hpp>
#include <liberror/Try.hpp>
#include <GLFW/glfw3.h>
#include <raylib.h>
#include <fplus/fplus.hpp>

#include <filesystem>
#include <cstdlib>
#include <span>
#include <ranges>

#define LOCALISATION(LANGUAGE, VALUE) Localisation::the()[LANGUAGE][VALUE].data()

static constexpr auto FPS = 60;

void show_help()
{
    fmt::println("A graphical xsetwacom wrapper for ease of use.");
    fmt::println("Usage:");
    fmt::println("  xsetwacomgui [OPTION...]");
    fmt::println("");
    fmt::println("  --no-gui        Launches the program without the UI. This is intended for");
    fmt::println("                  loading saved device settings on system boot.");
}

liberror::Result<void> apply_settings_to_device(libwacom::Device const& device, Monitor const& monitor, DeviceSettings const& settings)
{
    TRY(libwacom::set_stylus_output_from_display_area(device.id, {
        monitor.offsetX + settings.monitorArea.offsetX,
        monitor.offsetY + settings.monitorArea.offsetY,
        settings.monitorArea.width,
        settings.monitorArea.height,
    }));

    TRY(libwacom::set_stylus_area(device.id, settings.deviceArea));
    TRY(libwacom::set_stylus_pressure_curve(device.id, settings.devicePressure));

    return {};
}

float& the_scale()
{
    static float scale = 1.0f;
    return scale;
}

void set_scale(float value)
{
    auto& scale = the_scale();
    scale = value;
}

float operator""_scaled(unsigned long long i)
{
    return static_cast<float>(i) * the_scale();
}

void render_application_settings_popup(ApplicationSettings& settings)
{
    if (ImGui::BeginTabBar("##Tabs_2"))
    {
        if (ImGui::BeginTabItem(Localisation::get(settings.language, Localisation::Popup_Settings_Tabs_Appearance_Title)))
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

            ImGui::Text("%s", Localisation::get(settings.language, Localisation::Popup_Settings_Tabs_Appearance_Scale));
            static float scale = settings.scale;
            auto hasChangedUIScale = ImGui::InputFloat("##UiScale", &scale, 0.1f);

            if (hasChangedUIScale)
            {
                scale = ImClamp(scale, 1.0f, 10.f);
                settings.scale = scale;
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(Localisation::get(settings.language, Localisation::Popup_Settings_Tabs_Language_Title)))
        {
            // ImGui::AlignTextToFramePadding();
            ImGui::Text("%s", Localisation::get(settings.language, Localisation::Popup_Settings_Tabs_Language_Language));
            // ImGui::SameLine();
            static char const* languages[] = {
                "en_us",
                "pt_br"
            };
            static int languageIndex = static_cast<int>(
                std::distance(&languages[0], std::ranges::find(&languages[0], &languages[std::size(languages)], settings.language))
            );
            auto hasChangedUILanguage = ImGui::Combo("##Language", &languageIndex, languages, std::size(languages));

            if (hasChangedUILanguage)
            {
                settings.language = languages[languageIndex];
            }

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

void render_region_mappers(Context& context, DeviceSettings& settings, std::vector<libwacom::Device>& devices, std::vector<Monitor>& monitors)
{
    auto [cursorPosX, cursorPosY] = ImGui::GetCursorPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    static ImVec2 monitorAreaAnchors[4] { { 0, 0 }, { 0, 1 }, { 1, 0 }, { 1, 1 } };

    if (!monitors.empty())
    {
        monitorAreaAnchors[0] = { settings.monitorArea.offsetX / context.monitorDefaultArea.width, settings.monitorArea.offsetY / context.monitorDefaultArea.height };
        monitorAreaAnchors[1] = { settings.monitorArea.offsetX / context.monitorDefaultArea.width, (settings.monitorArea.height + settings.monitorArea.offsetY) / context.monitorDefaultArea.height };
        monitorAreaAnchors[2] = { (settings.monitorArea.width + settings.monitorArea.offsetX) / context.monitorDefaultArea.width, settings.monitorArea.offsetY / context.monitorDefaultArea.height };
        monitorAreaAnchors[3] = { (settings.monitorArea.width + settings.monitorArea.offsetX) / context.monitorDefaultArea.width, (settings.monitorArea.height + settings.monitorArea.offsetY) / context.monitorDefaultArea.height };
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
    context.hasChangedMonitorArea |= area_mapper("Monitor", monitorAreaAnchors, monitorMapperSize, &monitorMapperPosition, settings.monitorForceFullArea, settings.monitorForceAspectRatio);
    ImGui::SetCursorPosX(cursorPosX);

    if (context.hasChangedMonitorArea)
    {
        settings.monitorArea = {
            .offsetX = monitorAreaAnchors[0].x * context.monitorDefaultArea.width,
            .offsetY = monitorAreaAnchors[0].y * context.monitorDefaultArea.height,
            .width   = (monitorAreaAnchors[2].x - monitorAreaAnchors[0].x) * context.monitorDefaultArea.width,
            .height  = (monitorAreaAnchors[3].y - monitorAreaAnchors[2].y) * context.monitorDefaultArea.height
        };
    }

    if (context.hasChangedMonitorArea && settings.monitorForceFullArea)
    {
        settings.monitorArea = context.monitorDefaultArea;
    }

    static ImVec2 deviceAreaAnchors[4] { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } };

    if (!devices.empty())
    {
        deviceAreaAnchors[0] = { settings.deviceArea.offsetX / context.deviceDefaultArea.width, settings.deviceArea.offsetY / context.deviceDefaultArea.height };
        deviceAreaAnchors[1] = { settings.deviceArea.offsetX / context.deviceDefaultArea.width, (settings.deviceArea.height + settings.deviceArea.offsetY) / context.deviceDefaultArea.height };
        deviceAreaAnchors[2] = { (settings.deviceArea.width + settings.deviceArea.offsetX) / context.deviceDefaultArea.width, settings.deviceArea.offsetY / context.deviceDefaultArea.height };
        deviceAreaAnchors[3] = { (settings.deviceArea.width + settings.deviceArea.offsetX) / context.deviceDefaultArea.width, (settings.deviceArea.height + settings.deviceArea.offsetY) / context.deviceDefaultArea.height };
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
    context.hasChangedDeviceArea |= area_mapper("Tablet", deviceAreaAnchors, deviceMapperSize, &deviceMapperPosition, settings.deviceForceFullArea, settings.deviceForceAspectRatio);
    ImGui::SetCursorPosX(cursorPosX);

    if (context.hasChangedDeviceArea)
    {
        settings.deviceArea = {
            .offsetX = deviceAreaAnchors[0].x * context.deviceDefaultArea.width,
            .offsetY = deviceAreaAnchors[0].y * context.deviceDefaultArea.height,
            .width   = (deviceAreaAnchors[2].x - deviceAreaAnchors[0].x) * context.deviceDefaultArea.width,
            .height  = (deviceAreaAnchors[3].y - deviceAreaAnchors[2].y) * context.deviceDefaultArea.height
        };
    }

    if (context.hasChangedDeviceArea && settings.deviceForceFullArea)
    {
        settings.deviceArea = context.deviceDefaultArea;
    }

    for (auto [monitorAnchor, deviceAnchor] : fplus::zip(std::span<ImVec2>(monitorAreaAnchors, 4), std::span<ImVec2>(deviceAreaAnchors, 4)))
    {
        auto p1 = monitorAnchor * (monitorMapperPosition.Max - monitorMapperPosition.Min) + monitorMapperPosition.Min;
        auto p2 = deviceAnchor * (deviceMapperPosition.Max - deviceMapperPosition.Min) + deviceMapperPosition.Min;
        drawList->AddLine(p1, p2, ImColor(255, 0, 0, 127), 2.f);
    }
}

liberror::Result<void> render_tablet_settings_tab(Context& context, DeviceSettings& deviceSettings, std::vector<libwacom::Device>& devices, ApplicationSettings& applicationSettings)
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

liberror::Result<void> render_monitor_settings_tab(Context& context, DeviceSettings& deviceSettings, std::vector<Monitor>& monitors, ApplicationSettings& applicationSettings)
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

liberror::Result<void> render_window(DeviceSettings& deviceSettings, std::vector<libwacom::Device>& devices, std::vector<Monitor>& monitors, ApplicationSettings& applicationSettings)
{
    static Context context = [&] () {
        libwacom::Device device = devices.empty() ? libwacom::Device {} : devices.front();
        libwacom::Area deviceDefaultArea = devices.empty() ? libwacom::Area {} : MUST(libwacom::get_stylus_default_area(device.id));
        Monitor monitor = fplus::find_first_by([] (Monitor const& monitor) { return monitor.primary; }, monitors).get_with_default({});
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
        render_region_mappers(context, deviceSettings, devices, monitors);
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

        TRY(apply_settings_to_device(context.device, context.monitor, deviceSettings));
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

    if (!std::filesystem::exists(SETTINGS_PATH))
    {
        std::filesystem::create_directory(SETTINGS_PATH);
    }

    if (std::find(arguments.begin(), arguments.end(), "--help") != arguments.end())
    {
        show_help();
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
        auto monitor = fplus::find_first_by([] (Monitor const& monitor) { return monitor.primary; }, monitors).get_with_default({});

        MUST(apply_settings_to_device(device, monitor, deviceSettings));

        fmt::println("Device settings loaded successfully");

        return EXIT_SUCCESS;
    }

    ApplicationSettings applicationSettings {
        .scale = 1.0,
        .theme = ApplicationSettings::Theme::DARK,
        .language = "en_us"
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
        }
    }

    set_scale(applicationSettings.scale);

    InitWindow(static_cast<int>(800_scaled), static_cast<int>(815_scaled), NAME);
    SetTargetFPS(FPS);

    auto const window = static_cast<GLFWwindow*>(GetWindowHandle());
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    auto& io = ImGui::GetIO();

    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    auto* font = io.Fonts->AddFontFromFileTTF(HOME"/resources/fonts/iosevka.ttf", 20_scaled, nullptr, io.Fonts->GetGlyphRangesDefault());

    while (!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(BLACK);

        if (applicationSettings.theme == ApplicationSettings::Theme::DARK)
        {
            ImGui::StyleColorsDark();
        }
        else
        {
            ImGui::StyleColorsLight();
        }

        glfwPollEvents();
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
                        render_application_settings_popup(applicationSettings);
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
        EndDrawing();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    CloseWindow();

    return EXIT_SUCCESS;
}
