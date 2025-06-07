#define IMGUI_DEFINE_MATH_OPERATORS

#include <spdlog/spdlog.h>

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

static constexpr auto WIDTH = 800;
static constexpr auto HEIGHT = 815;
static constexpr auto FPS = 60;

liberror::Result<void> apply_settings_to_device(libwacom::Device const& device, Monitor const& monitor, Settings const& settings)
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

liberror::Result<void> render_window(Settings& settings, std::vector<libwacom::Device>& devices, std::vector<Monitor>& monitors)
{
    static libwacom::Device device = devices.empty() ? libwacom::Device {} : devices.front();
    static libwacom::Area deviceDefaultArea = devices.empty() ? libwacom::Area {} : TRY(libwacom::get_stylus_default_area(device.id));

    static Monitor monitor = fplus::find_first_by([] (Monitor const& monitor) { return monitor.primary; }, monitors).get_with_default({});
    static libwacom::Area monitorDefaultArea = monitors.empty() ? libwacom::Area {} : libwacom::Area { 0, 0, monitor.width, monitor.height };

    static bool hasChangedDevice; // cppcheck-suppress variableScope
    static bool hasChangedDeviceArea; // cppcheck-suppress variableScope
    static bool hasChangedDevicePressure; // cppcheck-suppress variableScope
    static bool hasChangedMonitor; // cppcheck-suppress variableScope
    static bool hasChangedMonitorArea; // cppcheck-suppress variableScope

    if (devices.empty() && settings.devicePressure.minX == -1 && settings.devicePressure.minY == -1 && settings.deviceArea.width == -1 && settings.deviceArea.height == -1)
    {
        ImGui::PushToast("Warning", "No devices were found");
        settings.deviceArea = { 0, 0, 0, 0 };
        settings.devicePressure = { 0, 0, 1, 1 };
        settings.monitorArea = monitorDefaultArea;
    }

    if (!devices.empty() && settings.devicePressure.minX == -1 && settings.devicePressure.minY == -1 && settings.deviceArea.width == -1 && settings.deviceArea.height == -1)
    {
        if (std::filesystem::exists(SETTINGS_FILE))
        {
            if (!load_device_settings(settings))
            {
                ImGui::PushToast("Error", "Failed to load device settings");
            }
        }
        else
        {
            ImGui::PushToast("Warning", "Device settings could not be found, reading directly from xsetwacom instead");
            settings.deviceName = device.name;
            settings.deviceArea = MUST(libwacom::get_stylus_area(device.id));
            settings.devicePressure = MUST(libwacom::get_stylus_pressure_curve(device.id));
            settings.monitorArea = monitorDefaultArea;
            settings.monitorName = monitor.name;
        }
    }

    ImGui::BeginGroup();
    {
        auto [cursorPosX, cursorPosY] = ImGui::GetCursorPos();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        static ImVec2 monitorAreaAnchors[4] { { 0, 0 }, { 0, 1 }, { 1, 0 }, { 1, 1 } };

        if (!monitors.empty())
        {
            monitorAreaAnchors[0] = { settings.monitorArea.offsetX / monitorDefaultArea.width, settings.monitorArea.offsetY / monitorDefaultArea.height };
            monitorAreaAnchors[1] = { settings.monitorArea.offsetX / monitorDefaultArea.width, (settings.monitorArea.height + settings.monitorArea.offsetY) / monitorDefaultArea.height };
            monitorAreaAnchors[2] = { (settings.monitorArea.width + settings.monitorArea.offsetX) / monitorDefaultArea.width, settings.monitorArea.offsetY / monitorDefaultArea.height };
            monitorAreaAnchors[3] = { (settings.monitorArea.width + settings.monitorArea.offsetX) / monitorDefaultArea.width, (settings.monitorArea.height + settings.monitorArea.offsetY) / monitorDefaultArea.height };
        }
        else
        {
            monitorAreaAnchors[0] = { 0, 0 };
            monitorAreaAnchors[1] = { 0, 1 };
            monitorAreaAnchors[2] = { 1, 0 };
            monitorAreaAnchors[3] = { 1, 1 };
        }

        static const ImVec2 monitorMapperSize { 20 * 16, 20 * 9 };
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - monitorMapperSize.x)/2);
        static ImRect monitorMapperPosition {};
        hasChangedMonitorArea |= area_mapper("Monitor", monitorAreaAnchors, monitorMapperSize, &monitorMapperPosition, settings.monitorForceFullArea, settings.monitorForceAspectRatio);
        ImGui::SetCursorPosX(cursorPosX);

        if (hasChangedMonitorArea)
        {
            settings.monitorArea = {
                .offsetX = monitorAreaAnchors[0].x * monitorDefaultArea.width,
                .offsetY = monitorAreaAnchors[0].y * monitorDefaultArea.height,
                .width   = (monitorAreaAnchors[2].x - monitorAreaAnchors[0].x) * monitorDefaultArea.width,
                .height  = (monitorAreaAnchors[3].y - monitorAreaAnchors[2].y) * monitorDefaultArea.height
            };
        }

        if (hasChangedMonitorArea && settings.monitorForceFullArea)
        {
            settings.monitorArea = monitorDefaultArea;
        }

        static ImVec2 deviceAreaAnchors[4] { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } };

        if (!devices.empty())
        {
            deviceAreaAnchors[0] = { settings.deviceArea.offsetX / deviceDefaultArea.width, settings.deviceArea.offsetY / deviceDefaultArea.height };
            deviceAreaAnchors[1] = { settings.deviceArea.offsetX / deviceDefaultArea.width, (settings.deviceArea.height + settings.deviceArea.offsetY) / deviceDefaultArea.height };
            deviceAreaAnchors[2] = { (settings.deviceArea.width + settings.deviceArea.offsetX) / deviceDefaultArea.width, settings.deviceArea.offsetY / deviceDefaultArea.height };
            deviceAreaAnchors[3] = { (settings.deviceArea.width + settings.deviceArea.offsetX) / deviceDefaultArea.width, (settings.deviceArea.height + settings.deviceArea.offsetY) / deviceDefaultArea.height };
        }
        else
        {
            deviceAreaAnchors[0] = { 0, 0 };
            deviceAreaAnchors[1] = { 0, 1 };
            deviceAreaAnchors[2] = { 1, 0 };
            deviceAreaAnchors[3] = { 1, 1 };
        }

        static const ImVec2 deviceMapperSize { 15 * 16, 15 * 9 };
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - deviceMapperSize.x)/2);
        static ImRect deviceMapperPosition {};
        hasChangedDeviceArea |= area_mapper("Tablet", deviceAreaAnchors, deviceMapperSize, &deviceMapperPosition, settings.deviceForceFullArea, settings.deviceForceAspectRatio);
        ImGui::SetCursorPosX(cursorPosX);

        if (hasChangedDeviceArea)
        {
            settings.deviceArea = {
                .offsetX = deviceAreaAnchors[0].x * deviceDefaultArea.width,
                .offsetY = deviceAreaAnchors[0].y * deviceDefaultArea.height,
                .width   = (deviceAreaAnchors[2].x - deviceAreaAnchors[0].x) * deviceDefaultArea.width,
                .height  = (deviceAreaAnchors[3].y - deviceAreaAnchors[2].y) * deviceDefaultArea.height
            };
        }

        if (hasChangedDeviceArea && settings.deviceForceFullArea)
        {
            settings.deviceArea = deviceDefaultArea;
        }

        for (auto [monitorAnchor, deviceAnchor] : fplus::zip(std::span<ImVec2>(monitorAreaAnchors, 4), std::span<ImVec2>(deviceAreaAnchors, 4)))
        {
            auto p1 = monitorAnchor * (monitorMapperPosition.Max - monitorMapperPosition.Min) + monitorMapperPosition.Min;
            auto p2 = deviceAnchor * (deviceMapperPosition.Max - deviceMapperPosition.Min) + deviceMapperPosition.Min;
            drawList->AddLine(p1, p2, ImColor(255, 0, 0, 127), 2.f);
        }
    }
    ImGui::EndGroup();

    if (ImGui::BeginTabBar("##Tabs"))
    {
        if (ImGui::BeginTabItem("Tablet Settings"))
        {
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (250 + 308))/2);
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Device");
                auto deviceNames = fplus::transform([] (libwacom::Device const& device) { return device.name.data(); }, devices);
                ImGui::SetNextItemWidth(308);
                static int deviceIndex;
                hasChangedDevice |= ImGui::Combo("##Device", &deviceIndex, deviceNames.data(), static_cast<int>(deviceNames.size()));

                if (hasChangedDevice)
                {
                    device = devices.at(static_cast<size_t>(deviceIndex));
                    deviceDefaultArea = TRY(libwacom::get_stylus_default_area(device.id));
                    settings.deviceArea = deviceDefaultArea;
                }

                {
                    ImGui::BeginGroup();
                    {
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Width");
                        ImGui::SetNextItemWidth(150);
                        hasChangedDeviceArea |= ImGui::InputFloat("##TabletWidth", &settings.deviceArea.width, 0.f, 0.f, "%.0f");
                    }
                    ImGui::EndGroup();
                    ImGui::SameLine();
                    ImGui::BeginGroup();
                    {
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Height");
                        ImGui::SetNextItemWidth(150);
                        hasChangedDeviceArea |= ImGui::InputFloat("##TabletHeight", &settings.deviceArea.height, 0.f, 0.f, "%.0f");
                    }
                    ImGui::EndGroup();
                }
                {
                    ImGui::BeginGroup();
                    {
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Offset X");
                        ImGui::SetNextItemWidth(150);
                        hasChangedDeviceArea |= ImGui::InputFloat("##TabletOffsetX", &settings.deviceArea.offsetX, 0.f, 0.f, "%.0f");
                    }
                    ImGui::EndGroup();
                    ImGui::SameLine();
                    ImGui::BeginGroup();
                    {
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Offset Y");
                        ImGui::SetNextItemWidth(150);
                        hasChangedDeviceArea |= ImGui::InputFloat("##TabletOffsetY", &settings.deviceArea.offsetY, 0.f, 0.f, "%.0f");
                    }
                    ImGui::EndGroup();
                }

                hasChangedDeviceArea |= ImGui::Checkbox("Full Area", &settings.deviceForceFullArea);
                ImGui::BeginDisabled();
                ImGui::Checkbox("Force Proportions", &settings.deviceForceAspectRatio);
                ImGui::EndDisabled();
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                static float devicePressureAnchors[4] = {};

                if (!devices.empty())
                {
                    devicePressureAnchors[0] = settings.devicePressure.minX;
                    devicePressureAnchors[1] = settings.devicePressure.minY;
                    devicePressureAnchors[2] = settings.devicePressure.maxX;
                    devicePressureAnchors[3] = settings.devicePressure.maxY;
                }
                else
                {
                    devicePressureAnchors[0] = 0;
                    devicePressureAnchors[1] = 0;
                    devicePressureAnchors[2] = 1;
                    devicePressureAnchors[3] = 1;
                }

                ImGui::AlignTextToFramePadding();
                hasChangedDevicePressure = ImGui::BezierEditor("Pressure Curve", devicePressureAnchors, { 250, 250 });

                if (hasChangedDevicePressure)
                {
                    settings.devicePressure = { devicePressureAnchors[0], devicePressureAnchors[1], devicePressureAnchors[2], devicePressureAnchors[3] };
                }
            }
            ImGui::EndGroup();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Monitor Settings"))
        {
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 308)/2);
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Monitor");
                auto monitorNames = fplus::transform([] (Monitor const& monitor) { return fmt::format("{} ({}x{})", monitor.name, monitor.width, monitor.height); }, monitors);
                auto monitorNamesData = fplus::transform([] (std::string const& name) { return name.data(); }, monitorNames);
                ImGui::SetNextItemWidth(308);
                static int monitorIndex;
                hasChangedMonitor |= ImGui::Combo("##Monitors", &monitorIndex, monitorNamesData.data(), static_cast<int>(monitorNamesData.size()));

                if (hasChangedMonitor)
                {
                    monitor = monitors.at(static_cast<size_t>(monitorIndex));
                    monitorDefaultArea = libwacom::Area { 0, 0, monitor.width, monitor.height };
                    settings.monitorArea = monitorDefaultArea;
                }

                {
                    ImGui::BeginGroup();
                    {
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Width");
                        ImGui::SetNextItemWidth(150);
                        hasChangedMonitorArea |= ImGui::InputFloat("##MonitorWidth", &settings.monitorArea.width, 0.f, 0.f, "%.0f");
                    }
                    ImGui::EndGroup();
                    ImGui::SameLine();
                    ImGui::BeginGroup();
                    {
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Height");
                        ImGui::SetNextItemWidth(150);
                        hasChangedMonitorArea |= ImGui::InputFloat("##MonitorHeight", &settings.monitorArea.height, 0.f, 0.f, "%.0f");
                    }
                    ImGui::EndGroup();
                }
                {
                    ImGui::BeginGroup();
                    {
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Offset X");
                        ImGui::SetNextItemWidth(150);
                        hasChangedMonitorArea |= ImGui::InputFloat("##MonitorOffsetX", &settings.monitorArea.offsetX, 0.f, 0.f, "%.0f");
                    }
                    ImGui::EndGroup();
                    ImGui::SameLine();
                    ImGui::BeginGroup();
                    {
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Offset Y");
                        ImGui::SetNextItemWidth(150);
                        hasChangedMonitorArea |= ImGui::InputFloat("##MonitorOffsetY", &settings.monitorArea.offsetY, 0.f, 0.f, "%.0f");
                    }
                    ImGui::EndGroup();
                }

                hasChangedMonitorArea |= ImGui::Checkbox("Full Area", &settings.monitorForceFullArea);
                ImGui::BeginDisabled();
                ImGui::Checkbox("Force Proportions", &settings.monitorForceAspectRatio);
                ImGui::EndDisabled();
            }
            ImGui::EndGroup();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    auto previousCursorPosition = ImGui::GetCursorPos();
    ImGui::SetCursorPosY(HEIGHT - 43);
    if (ImGui::Button("Save & Apply", { 200, 35 }))
    {
        if (save_device_settings(settings))
        {
            ImGui::PushToast("Success", "Successfully saved device settings");
        }

        TRY(apply_settings_to_device(device, monitor, settings));
    }
    ImGui::SetCursorPos(previousCursorPosition);

    return {};
}

void show_help()
{
    fmt::println("A graphical xsetwacom wrapper for ease of use.");
    fmt::println("Usage:");
    fmt::println("  xsetwacomgui [OPTION...]");
    fmt::println("");
    fmt::println("  --no-gui        Launches the program without the UI. This is intended for");
    fmt::println("                  loading saved device settings on system boot.");
}

int main(int argc, char const** argv)
{
    auto arguments =
        std::span<char const*>(argv, size_t(argc))
            | std::views::transform([] (auto arg) { return std::string_view(arg); });

    std::vector<Monitor> monitors = MUST(get_available_monitors());

    std::vector<libwacom::Device> devices = MUST(libwacom::get_available_devices());
    devices = fplus::keep_if([] (auto&& device) { return device.kind == libwacom::Device::Kind::STYLUS; }, devices);

    Settings settings {
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
        if (!std::filesystem::exists(SETTINGS_FILE))
        {
            spdlog::error("Device settings could not be found");
            return EXIT_FAILURE;
        }

        if (!load_device_settings(settings))
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

        MUST(apply_settings_to_device(device, monitor, settings));

        fmt::println("Device settings loaded successfully");

        return EXIT_SUCCESS;
    }

    InitWindow(WIDTH, HEIGHT, NAME);
    SetTargetFPS(FPS);

    auto const window = static_cast<GLFWwindow*>(GetWindowHandle());
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    auto& io = ImGui::GetIO();

    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    auto* font = io.Fonts->AddFontFromFileTTF(HOME"/resources/fonts/iosevka.ttf", 20.f, nullptr, io.Fonts->GetGlyphRangesDefault());

    while (!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(BLACK);

        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int width, height;
        glfwGetWindowSize(window, &width, &height);
        ImGui::SetNextWindowPos({});
        ImGui::SetNextWindowSize({ static_cast<float>(width), static_cast<float>(height) });
        ImGui::PushFont(font);
        {
            ImGui::Begin(NAME, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings);
            ImGui::RenderToasts();
            {
                ImGui::BeginDisabled(devices.empty());
                {
                    MUST(render_window(settings, devices, monitors));
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
