#define IMGUI_DEFINE_MATH_OPERATORS

#include "Widgets.hpp"
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

static constexpr auto WIDTH = 800;
static constexpr auto HEIGHT = 785;
static constexpr auto FPS = 60;

liberror::Result<void> render_window(DeviceSetttings settings, std::vector<libwacom::Device>& devices)
{
    static libwacom::Device device = devices.empty() ? libwacom::Device {} : devices.front();
    static libwacom::Area deviceDefaultArea = devices.empty() ? libwacom::Area {} : TRY(libwacom::get_stylus_default_area(device.id));

    static bool hasChangedDevice; // cppcheck-suppress variableScope
    static bool hasChangedDeviceArea; // cppcheck-suppress variableScope
    static bool hasChangedDevicePressure; // cppcheck-suppress variableScope

    if (devices.empty() && settings.pressure.minX == -1 && settings.pressure.minY == -1 && settings.area.width == -1 && settings.area.height == -1)
    {
        settings.area = { 0, 0, 0, 0 };
        settings.pressure = { 0, 0, 1, 1 };
        ImGui::PushToast("Warning", "No devices were found");
    }

    if (!devices.empty() && settings.pressure.minX == -1 && settings.pressure.minY == -1 && settings.area.width == -1 && settings.area.height == -1)
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
            settings.area = MUST(libwacom::get_stylus_area(device.id));
            settings.pressure = MUST(libwacom::get_stylus_pressure_curve(device.id));
        }
    }

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
            deviceAreaAnchors[0] = { settings.area.offsetX / deviceDefaultArea.width, settings.area.offsetY / deviceDefaultArea.height };
            deviceAreaAnchors[1] = { settings.area.offsetX / deviceDefaultArea.width, (settings.area.height + settings.area.offsetY) / deviceDefaultArea.height };
            deviceAreaAnchors[2] = { (settings.area.width + settings.area.offsetX) / deviceDefaultArea.width, settings.area.offsetY / deviceDefaultArea.height };
            deviceAreaAnchors[3] = { (settings.area.width + settings.area.offsetX) / deviceDefaultArea.width, (settings.area.height + settings.area.offsetY) / deviceDefaultArea.height };
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
        hasChangedDeviceArea |= area_mapper("Tablet", deviceAreaAnchors, deviceMapperSize, &deviceMapperPosition, settings.forceFullArea, settings.forceAspectRatio);
        ImGui::SetCursorPosX(cursorPosX);

        if (hasChangedDeviceArea)
        {
            settings.area = {
                .offsetX = deviceAreaAnchors[0].x * deviceDefaultArea.width,
                .offsetY = deviceAreaAnchors[0].y * deviceDefaultArea.height,
                .width   = (deviceAreaAnchors[2].x - deviceAreaAnchors[0].x) * deviceDefaultArea.width,
                .height  = (deviceAreaAnchors[3].y - deviceAreaAnchors[2].y) * deviceDefaultArea.height
            };
        }

        if (hasChangedDeviceArea && settings.forceFullArea)
        {
            settings.area = deviceDefaultArea;
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
            }

            {
                ImGui::BeginGroup();
                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Width");
                    ImGui::SetNextItemWidth(150);
                    hasChangedDeviceArea |= ImGui::InputFloat("##TabletWidth", &settings.area.width, 0.f, 0.f, "%.0f");
                }
                ImGui::EndGroup();
                ImGui::SameLine();
                ImGui::BeginGroup();
                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Height");
                    ImGui::SetNextItemWidth(150);
                    hasChangedDeviceArea |= ImGui::InputFloat("##TabletHeight", &settings.area.height, 0.f, 0.f, "%.0f");
                }
                ImGui::EndGroup();
            }
            {
                ImGui::BeginGroup();
                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Offset X");
                    ImGui::SetNextItemWidth(150);
                    hasChangedDeviceArea |= ImGui::InputFloat("##TabletOffsetX", &settings.area.offsetX, 0.f, 0.f, "%.0f");
                }
                ImGui::EndGroup();
                ImGui::SameLine();
                ImGui::BeginGroup();
                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Offset Y");
                    ImGui::SetNextItemWidth(150);
                    hasChangedDeviceArea |= ImGui::InputFloat("##TabletOffsetY", &settings.area.offsetY, 0.f, 0.f, "%.0f");
                }
                ImGui::EndGroup();
            }

            hasChangedDeviceArea |= ImGui::Checkbox("Full Area", &settings.forceFullArea);
            ImGui::BeginDisabled();
            ImGui::Checkbox("Force Proportions", &settings.forceAspectRatio);
            ImGui::EndDisabled();
        }
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginGroup();
        {
            static float devicePressureAnchors[4] = { settings.pressure.minX, settings.pressure.minY, settings.pressure.maxX, settings.pressure.maxY };

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
                settings.pressure = { devicePressureAnchors[0], devicePressureAnchors[1], devicePressureAnchors[2], devicePressureAnchors[3] };
            }
        }
        ImGui::EndGroup();
    }
    ImGui::EndGroup();

    if (ImGui::Button("Save & Apply", { 200, 35 }))
    {
        if (save_device_settings(settings))
        {
            ImGui::PushToast("Success", "Successfully saved device settings");
        }

        libwacom::set_stylus_area(device.id, settings.area);
        libwacom::set_stylus_pressure_curve(device.id, settings.pressure);
    }

    return {};
}

int main()
{
    InitWindow(WIDTH, HEIGHT, NAME);
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

    libwacom::Area deviceArea { -1, -1, -1, -1 };
    libwacom::Pressure devicePressure { -1, -1, -1, -1 };
    bool forceFullArea = false;
    bool forceAspectRatio = false;

    DeviceSetttings ctx { deviceArea, devicePressure, forceFullArea, forceAspectRatio };

    if (!std::filesystem::exists(SETTINGS_PATH))
    {
        std::filesystem::create_directory(SETTINGS_PATH);
    }

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
            ImGui::Begin(NAME, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);
            ImGui::RenderToasts();
            {
                ImGui::BeginDisabled(devices.empty());
                {
                    MUST(render_window(ctx, devices));
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
