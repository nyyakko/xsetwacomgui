#include <algorithm>
#define IMGUI_DEFINE_MATH_OPERATORS

#include "Widgets.hpp"

#include <imgui/extensions/imgui_bezier.hpp>
#include <imgui/imgui.hpp>
#include <imgui/imgui_internal.hpp>
#include <imgui/imgui_impl_glfw.hpp>
#include <imgui/imgui_impl_opengl3.hpp>
#include <spdlog/spdlog.h>
#include <libwacom/Device.hpp>
#include <liberror/Try.hpp>
#include <GLFW/glfw3.h>
#include <raylib.h>
#include <fplus/fplus.hpp>
#include <nlohmann/json.hpp>

#include <span>
#include <filesystem>
#include <fstream>
#include <sstream>

static constexpr auto APPLICATION = "XSetWacomGUI";
static constexpr auto APPLICATION_DEVICE_SETTINGS = "settings.json";

static constexpr auto WIDTH = 800;
static constexpr auto HEIGHT = 785;
static constexpr auto FPS = 60;

liberror::Result<void> load_device_settings(libwacom::Area& area, libwacom::Pressure& pressure, bool& forceFullArea, bool& forceAspectRatio)
{
    std::ifstream settings(APPLICATION_DEVICE_SETTINGS);
    std::stringstream content;
    content << settings.rdbuf();

    try
    {
        auto settingsJson = nlohmann::json::parse(content.str());

        forceFullArea = settingsJson["forceFullArea"].get<bool>();
        forceAspectRatio = settingsJson["forceAspectRatio"].get<bool>();
        area.offsetX = settingsJson["area"]["offsetX"].get<float>();
        area.offsetY = settingsJson["area"]["offsetY"].get<float>();
        area.width = settingsJson["area"]["width"].get<float>();
        area.height = settingsJson["area"]["height"].get<float>();
        pressure.minX = settingsJson["pressure"]["minX"].get<float>();
        pressure.minY = settingsJson["pressure"]["minY"].get<float>();
        pressure.maxX = settingsJson["pressure"]["maxX"].get<float>();
        pressure.maxY = settingsJson["pressure"]["maxY"].get<float>();
    }
    catch (std::exception const& error)
    {
        return liberror::make_error("Failed to properly parse settings file: {}", error.what());
    }

    spdlog::info("Loaded settings from file {}", APPLICATION_DEVICE_SETTINGS);

    return {};
}

void save_device_settings(libwacom::Area const& area, libwacom::Pressure const& pressure, bool forceFullArea, bool forceAspectRatio)
{
    nlohmann::ordered_json settingsJson {
        { "forceFullArea", forceFullArea },
        { "forceAspectRatio", forceAspectRatio },
        {
            "area", {
                { "offsetX", area.offsetX },
                { "offsetY", area.offsetY },
                { "width", area.width },
                { "height", area.height }
            }
        },
        {
            "pressure", {
                { "minX", pressure.minX },
                { "minY", pressure.minY },
                { "maxX", pressure.maxX },
                { "maxY", pressure.maxY },
            }
        }
    };

    std::ofstream file { APPLICATION_DEVICE_SETTINGS };
    file << std::setw(4) << settingsJson;

    spdlog::info("Saved settings to file {}", APPLICATION_DEVICE_SETTINGS);
}

liberror::Result<void> render_window(std::vector<libwacom::Device> const& devices, libwacom::Device const& selectedDevice, int& selectedDeviceIndex, libwacom::Area& deviceArea, libwacom::Pressure& devicePressure, bool& hasChangedDeviceArea, bool& hasChangedDevicePressure, bool& forceFullArea, bool& forceAspectRatio)
{
    ImGui::BeginGroup();
    {
        auto [cursorPosX, cursorPosY] = ImGui::GetCursorPos();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        static ImVec2 monitorAreaAnchors[4] { { 0, 0 }, { 0, 1 }, { 1, 0 }, { 1, 1 } };
        static const ImVec2 monitorMapperSize { 20 * 16, 20 * 9 };
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - monitorMapperSize.x)/2);
        static ImRect monitorMapperPosition {};
        area_mapper("Monitor", monitorAreaAnchors, monitorMapperSize, &monitorMapperPosition);
        ImGui::SetCursorPosX(cursorPosX);

        static ImVec2 deviceAreaAnchors[4] { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } };

        if (!devices.empty())
        {
            deviceAreaAnchors[0] = { deviceArea.offsetX / selectedDevice.area.width, deviceArea.offsetY / selectedDevice.area.height };
            deviceAreaAnchors[1] = { deviceArea.offsetX / selectedDevice.area.width, (deviceArea.height + deviceArea.offsetY) / selectedDevice.area.height };
            deviceAreaAnchors[2] = { (deviceArea.width + deviceArea.offsetX) / selectedDevice.area.width, deviceArea.offsetY / selectedDevice.area.height };
            deviceAreaAnchors[3] = { (deviceArea.width + deviceArea.offsetX) / selectedDevice.area.width, (deviceArea.height + deviceArea.offsetY) / selectedDevice.area.height };
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
        hasChangedDeviceArea = area_mapper("Tablet", deviceAreaAnchors, deviceMapperSize, &deviceMapperPosition, forceFullArea, forceAspectRatio);
        ImGui::SetCursorPosX(cursorPosX);

        if (hasChangedDeviceArea)
        {
            deviceArea = {
                .offsetX = deviceAreaAnchors[0].x * selectedDevice.area.width,
                .offsetY = deviceAreaAnchors[0].y * selectedDevice.area.height,
                .width   = (deviceAreaAnchors[2].x - deviceAreaAnchors[0].x) * selectedDevice.area.width,
                .height  = (deviceAreaAnchors[3].y - deviceAreaAnchors[2].y) * selectedDevice.area.height
            };
        }

        for (auto [monitorAnchor, deviceAnchor] : fplus::zip(std::span<ImVec2>(monitorAreaAnchors, 4), std::span<ImVec2>(deviceAreaAnchors, 4)))
        {
            auto p1 = monitorAnchor * (monitorMapperPosition.Max - monitorMapperPosition.Min) + monitorMapperPosition.Min;
            auto p2 = deviceAnchor * (deviceMapperPosition.Max - deviceMapperPosition.Min) + deviceMapperPosition.Min;
            drawList->AddLine(p1, p2, ImColor(255, 0, 0, 127), 2.f);
        }
    }
    ImGui::EndGroup();

    ImGui::Separator();

    ImGui::BeginGroup();
    {
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (250 + 308))/2);
        ImGui::BeginGroup();
        {
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Device");
                auto deviceNames = fplus::transform([] (libwacom::Device const& device) { return device.name.data(); }, devices);
                ImGui::SetNextItemWidth(308);
                ImGui::Combo("##Device", &selectedDeviceIndex, deviceNames.data(), static_cast<int>(deviceNames.size()));
            }
            {
                ImGui::BeginGroup();
                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Width");
                    ImGui::SetNextItemWidth(150);
                    hasChangedDeviceArea |= ImGui::InputFloat("##TabletWidth", &deviceArea.width, 0.f, 0.f, "%.0f");
                }
                ImGui::EndGroup();
                ImGui::SameLine();
                ImGui::BeginGroup();
                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Height");
                    ImGui::SetNextItemWidth(150);
                    hasChangedDeviceArea |= ImGui::InputFloat("##TabletHeight", &deviceArea.height, 0.f, 0.f, "%.0f");
                }
                ImGui::EndGroup();
            }
            {
                ImGui::BeginGroup();
                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Offset X");
                    ImGui::SetNextItemWidth(150);
                    hasChangedDeviceArea |= ImGui::InputFloat("##TabletOffsetX", &deviceArea.offsetX, 0.f, 0.f, "%.0f");
                }
                ImGui::EndGroup();
                ImGui::SameLine();
                ImGui::BeginGroup();
                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Offset Y");
                    ImGui::SetNextItemWidth(150);
                    hasChangedDeviceArea |= ImGui::InputFloat("##TabletOffsetY", &deviceArea.offsetY, 0.f, 0.f, "%.0f");
                }
                ImGui::EndGroup();
            }

            hasChangedDeviceArea |= ImGui::Checkbox("Full Area", &forceFullArea);
            ImGui::BeginDisabled();
            ImGui::Checkbox("Force Proportions", &forceAspectRatio);
            ImGui::EndDisabled();
        }
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginGroup();
        {
            static float devicePressureAnchors[4] = { devicePressure.minX, devicePressure.minY, devicePressure.maxX, devicePressure.maxY };

            if (devices.empty())
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
                devicePressure = { devicePressureAnchors[0], devicePressureAnchors[1], devicePressureAnchors[2], devicePressureAnchors[3] };
            }
        }
        ImGui::EndGroup();
    }
    ImGui::EndGroup();

    if (ImGui::Button("Save & Apply", { 200, 35 }))
    {
        libwacom::set_stylus_area(selectedDevice.id, deviceArea);
        libwacom::set_stylus_pressure_curve(selectedDevice.id, devicePressure);
        save_device_settings(deviceArea, devicePressure, forceFullArea, forceAspectRatio);
    }

    return {};
}

int main()
{
    InitWindow(WIDTH, HEIGHT, APPLICATION);
    SetTargetFPS(FPS);

    auto const window = static_cast<GLFWwindow*>(GetWindowHandle());
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    auto io = ImGui::GetIO();
    auto* font = io.Fonts->AddFontFromFileTTF(HOME"/resources/fonts/iosevka.ttf", 20.f, nullptr, io.Fonts->GetGlyphRangesDefault());

    std::vector<libwacom::Device> devices = MUST(libwacom::get_available_devices());
    devices = fplus::keep_if([] (auto&& device) { return device.kind == libwacom::Device::Kind::STYLUS; }, devices);
    int selectedDeviceIndex {};
    libwacom::Device const& selectedDevice = devices.empty() ? libwacom::Device {} : devices.at(static_cast<size_t>(selectedDeviceIndex));
    libwacom::Area deviceArea { -1, -1, -1, -1 };
    libwacom::Pressure devicePressure { -1, -1, -1, -1 };
    bool forceFullArea = false;
    bool forceAspectRatio = false;
    bool hasChangedDeviceArea = false;
    bool hasChangedDevicePressure = false;

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
            ImGui::Begin(APPLICATION, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            {
                ImGui::BeginDisabled(devices.empty());
                {
                    if (devices.empty() || forceFullArea) deviceArea = selectedDevice.area;
                    if (devices.empty()) devicePressure = { 0, 0, 1, 1 };

                    if (!devices.empty() && devicePressure.minX == -1 && devicePressure.minY == -1 && deviceArea.width == -1 && deviceArea.height == -1)
                    {
                        if (std::filesystem::exists(APPLICATION_DEVICE_SETTINGS))
                        {
                            MUST(load_device_settings(deviceArea, devicePressure, forceFullArea, forceAspectRatio));
                        }
                        else
                        {
                            spdlog::warn("Settings file not found, reading properties from xsetwacom");
                            deviceArea = MUST(libwacom::get_stylus_area(selectedDevice.id));
                            devicePressure = MUST(libwacom::get_stylus_pressure_curve(selectedDevice.id));
                        }
                    }

                    MUST(render_window(devices, selectedDevice, selectedDeviceIndex, deviceArea, devicePressure, hasChangedDeviceArea, hasChangedDevicePressure, forceFullArea, forceAspectRatio));
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
}
