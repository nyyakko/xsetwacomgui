#define IMGUI_DEFINE_MATH_OPERATORS

#include <spdlog/spdlog.h>

#include "GoddessWindow.hpp"
#include "MainWindow.hpp"
#include "SettingsWindow.hpp"
#include "ui/Localisation.hpp"
#include "ui/Scaling.hpp"

#include <argparse/argparse.hpp>
#include <fplus/fplus.hpp>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <imgui/extensions/imgui_toast.hpp>
#include <imgui/imgui.hpp>
#include <imgui/imgui_impl_glfw.hpp>
#include <imgui/imgui_impl_opengl3.hpp>
#include <liberror/Try.hpp>
#include <scn/scan.h>

#include <span>

using namespace liberror;

Result<void> safe_main(std::span<char const*> const& arguments)
{
    argparse::ArgumentParser parser(NAME, "", argparse::default_arguments::help);
    parser.add_description("A graphical xsetwacom wrapper for ease of use.");

    argparse::ArgumentParser configCommand("config", "", argparse::default_arguments::help);
    configCommand.add_description("manages device related configuration");
    configCommand.add_argument("--load").help("loads the tablet configuration without loading the UI").flag();
    parser.add_subparser(configCommand);

    try
    {
        parser.parse_args(static_cast<int>(arguments.size()), arguments.data());
    }
    catch (std::exception const& exception)
    {
        return make_error(exception.what());
    }

    std::vector<Display> displays = TRY(get_available_displays());
    std::vector<Device> devices = TRY(get_available_devices());
    devices = fplus::keep_if([] (auto&& device) { return device.kind == Device::Kind::STYLUS; }, devices);

    if (!(std::filesystem::exists(get_application_config_path()) || std::filesystem::create_directory(get_application_config_path())))
    {
        return make_error("Failed to create settings directory");
    }

    if (configCommand["--load"] != false)
    {
        TabletSettings tabletSettings {};

        if (!load_tablet_settings(tabletSettings))
        {
            return make_error("Failed to load device settings");
        }

        if (devices.empty() || displays.empty())
        {
            return make_error("Failed to load devices");
        }

        auto device  = devices.front();
        auto display = TRY(get_primary_display());

        TRY(set_stylus_area(device.id, tabletSettings.device.area));
        TRY(set_stylus_handedness(device.id, tabletSettings.device.handedness));
        TRY(set_stylus_pressure_curve(device.id, tabletSettings.device.pressure));
        auto displayArea = tabletSettings.display.area;
        displayArea.offsetX += display.area.offsetX;
        displayArea.offsetY += display.area.offsetY;
        TRY(set_stylus_output_from_display_area(device.id, displayArea));

        fmt::println("Device settings loaded successfully");

        return {};
    }

    ApplicationSettings applicationSettings {};
    TabletSettings tabletSettings {};

    if (!std::filesystem::exists(APPLICATION_SETTINGS_FILE))
    {
        save_application_settings(applicationSettings);
    }
    else
    {
        auto result = load_application_settings(applicationSettings);
        if (!result.has_value())
        {
            fmt::println("The currently saved application settings differs from");
            fmt::println("the expected format. You can:\n");

            fmt::println("1. Overwrite Everything");
            fmt::println("2. Migrate Manually\n");

            auto choice = scn::prompt<int>("How would you like to proceed? (choose a value) ", "{}");

            if (choice)
            {
                if (choice->value() == 1) save_application_settings(applicationSettings);
                else if (choice->value() == 2) migrate_application_settings(applicationSettings);
            }

            fmt::println("Done. Restart the application.");

            return {};
        }
    }

    if (!glfwInit())
    {
        return make_error("Failed to initialize glfw");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    set_scale(applicationSettings.scale);

#ifdef DEBUG
    auto window = glfwCreateWindow(static_cast<int>(800_scaled), static_cast<int>(815_scaled), NAME " - DEBUG BUILD", nullptr, nullptr);
#else
    auto window = glfwCreateWindow(static_cast<int>(800_scaled), static_cast<int>(815_scaled), NAME, nullptr, nullptr);
#endif

    glfwMakeContextCurrent(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    auto& io = ImGui::GetIO();

    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    ImFont* font = nullptr;

    ImVector<ImWchar> ranges {};
    ImFontGlyphRangesBuilder rangeBuilder {};

    static const ImWchar rangesData[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
        0,
    };

    rangeBuilder.AddRanges(rangesData);
    rangeBuilder.BuildRanges(&ranges);

    if (applicationSettings.font.family != "Default")
    {
        font = io.Fonts->AddFontFromFileTTF(applicationSettings.font.path.string().data(), 20_scaled, nullptr, ranges.Data);
    }

    Context context { applicationSettings, tabletSettings, devices, displays };

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
                    if (ImGui::BeginMenu(TRY(Localisation::get(applicationSettings.language, Localisation::MenuBar_Settings_Title))))
                    {
                        if (ImGui::MenuItem(TRY(Localisation::get(applicationSettings.language, Localisation::MenuBar_Settings_Application))))
                        {
                            isApplicationSettingsOpen = true;
                        }

                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu(TRY(Localisation::get(applicationSettings.language, Localisation::MenuBar_Other_Title))))
                    {
                        if (ImGui::MenuItem(TRY(Localisation::get(applicationSettings.language, Localisation::MenuBar_Other_Goddess))))
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
                        TRY(Localisation::get(applicationSettings.language, Localisation::MenuBar_Settings_Application)),
                        &isApplicationSettingsOpen,
                        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
                    );
                    {
                        TRY(render_settings_window(context));
                    }
                    ImGui::End();
                }

                if (isGoddessOpen)
                {
                    float goddessWidth = static_cast<float>(windowWidth)/1.5f, goddessHeight = static_cast<float>(windowHeight)/1.5f;
                    ImGui::SetNextWindowSize({ goddessWidth, goddessHeight });
                    ImGui::SetNextWindowPos({ (static_cast<float>(windowWidth) - goddessWidth)/2, (static_cast<float>(windowHeight) - goddessHeight)/2 });
                    ImGui::Begin(
                        TRY(Localisation::get(applicationSettings.language, Localisation::MenuBar_Other_Goddess)),
                        &isGoddessOpen,
                        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
                    );
                    {
                        TRY(render_goddess_window());
                    }
                    ImGui::End();
                }

                ImGui::BeginDisabled(devices.empty());
                {
                    TRY(render_main_window(context));
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

    return {};
}

int main(int argc, char const** argv)
{
    auto result = safe_main(std::span<char const*>(argv, size_t(argc)));

    if (!result.has_value())
    {
        spdlog::error("{}", result.error().message());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
