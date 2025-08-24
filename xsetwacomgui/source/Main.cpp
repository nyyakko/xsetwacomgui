#define IMGUI_DEFINE_MATH_OPERATORS

#include <spdlog/spdlog.h>

#include "core/Context.hpp"
#include "platform/udev/UDevDevice.hpp"
#include "core/ipc/Client.hpp"
#include "core/ipc/Server.hpp"
#include "GoddessWindow.hpp"
#include "MainWindow.hpp"
#include "platform/Daemon.hpp"
#include "SettingsWindow.hpp"
#include "ui/Localisation.hpp"
#include "ui/Scaling.hpp"

#include <magic_enum/magic_enum.hpp>
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

#include <sys/poll.h>

#include <span>

using namespace liberror;

Result<void> run_gui(Context& context)
{
    TRY(IPCClient::the().configure(IPCClient::Mode::ASYNC));
    TRY(IPCClient::the().connect());

    if (!glfwInit())
    {
        return make_error("Failed to initialize glfw");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    set_scale(context.applicationSettings.scale);

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

    if (context.applicationSettings.font.family != "Default")
    {
        font = io.Fonts->AddFontFromFileTTF(context.applicationSettings.font.path.string().data(), 20_scaled, nullptr, ranges.Data);
    }

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            break;
        }

        if (context.applicationSettings.theme == ApplicationSettings::Theme::DARK)
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
                    if (ImGui::BeginMenu(TRY(Localisation::get(context.applicationSettings.language, Localisation::MenuBar_Settings_Title))))
                    {
                        if (ImGui::MenuItem(TRY(Localisation::get(context.applicationSettings.language, Localisation::MenuBar_Settings_Entry_Application))))
                        {
                            isApplicationSettingsOpen = true;
                        }

                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu(TRY(Localisation::get(context.applicationSettings.language, Localisation::MenuBar_Other_Title))))
                    {
                        if (ImGui::MenuItem(TRY(Localisation::get(context.applicationSettings.language, Localisation::MenuBar_Other_Entry_Goddess))))
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
                        TRY(Localisation::get(context.applicationSettings.language, Localisation::MenuBar_Settings_Entry_Application)),
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
                        TRY(Localisation::get(context.applicationSettings.language, Localisation::MenuBar_Other_Entry_Goddess)),
                        &isGoddessOpen,
                        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
                    );
                    {
                        TRY(render_goddess_window());
                    }
                    ImGui::End();
                }

                ImGui::BeginDisabled(context.devices.empty());
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

Result<void> run_no_gui(Context& context)
{
    TRY(daemonize(NAME"-client", QuitParent::TRUE));

    TRY(IPCClient::the().configure(IPCClient::Mode::SYNC));
    TRY(IPCClient::the().connect());

    using namespace std::literals;

    while (true)
    {
        auto message = TRY(IPCClient::the().receive_message());

        auto action = magic_enum::enum_cast<UDevDevice::Action>(message.data());
        assert(action && "INVALID ACTION");

        switch (*action)
        {
            case UDevDevice::Action::UNBIND: {
                auto devicesFiltered = fplus::keep_if([] (auto&& device) { return device.kind == Device::Kind::STYLUS; }, context.devices);
                auto hadMoreThanOneDevice = devicesFiltered.size() > 1;
                context.devices = TRY(get_available_devices());

                if (hadMoreThanOneDevice) break;

                auto maybeDevice = std::ranges::find(context.devices, context.tabletSettings.stylus.name, &Device::name);

                if (maybeDevice == context.devices.end())
                {
                    context.display = {};
                    context.stylus = {};
                    context.pad = {};
                    context.tabletSettings = {};
                }

                break;
            }
            case UDevDevice::Action::BIND: {
                auto devicesFiltered = fplus::keep_if([] (auto&& device) { return device.kind == Device::Kind::STYLUS; }, context.devices);
                auto hadAtleastOneDevice = !devicesFiltered.empty();
                context.devices = TRY(get_available_devices());

                if (hadAtleastOneDevice) break;

                auto result = load_tablet_settings();

                if (!result.has_value())
                {
                    TRY(apply_settings_from_driver_to_context(context));

                    switch (result.error().message())
                    {
                    case SettingsError::Type::WRITE_FAILURE: break;
                    case SettingsError::Type::FILE_NOT_FOUND: {
                        return make_error("No saved device settings could be found, reading directly from xsetwacom instead");
                    }
                    case SettingsError::Type::READ_FAILURE: {
                        return make_error("Failed to load device settings");
                    }
                    case SettingsError::Type::OUTDATED_SCHEMA: {
                        return make_error("Outdated tablet settings file");
                    }
                    }
                }
                else
                {
                    context.tabletSettings = *result;

                    auto stylus = fplus::keep_if([] (auto&& device) { return device.kind == Device::Kind::STYLUS; }, context.devices).back();
                    assert(stylus.name == context.tabletSettings.stylus.name && "FIXME: assuming device connected is the same as the one saved in the settings file");
                    context.stylus = stylus;

                    auto pad = fplus::keep_if([] (auto&& device) { return device.kind == Device::Kind::PAD; }, context.devices).back();
                    assert(pad.name == context.tabletSettings.pad.name && "FIXME: assuming device connected is the same as the one saved in the settings file");
                    context.pad = pad;

                    context.hasChangedDevice = true;
                    context.hasChangedDeviceHandedness = true;
                    context.display = *std::ranges::find(context.displays, context.tabletSettings.display.name, &Display::name);
                    context.hasChangedDisplay = true;

                    TRY(apply_settings_from_context_to_device(context));

                    spdlog::info("Device settings loaded successfully");
                }

                break;
            }
            case UDevDevice::Action::REMOVE: break;
            case UDevDevice::Action::ADD: break;
            case UDevDevice::Action::NONE: break;
        }
    }

    return {};
}

Result<void> safe_main(std::span<char const*> const& arguments)
{
    argparse::ArgumentParser cli(NAME, "", argparse::default_arguments::help);
    cli.add_description("A graphical xsetwacom wrapper for ease of use.");

    cli.add_argument("--no-gui").help("starts only the server daemon").flag();

    argparse::ArgumentParser config("config", "", argparse::default_arguments::help);
    config.add_description("manages device related configuration");
    config.add_argument("--load").help("loads the tablet configuration without loading the UI").flag();

    cli.add_subparser(config);

    try
    {
        cli.parse_args(static_cast<int>(arguments.size()), arguments.data());
    }
    catch (std::exception const& exception)
    {
        return make_error(exception.what());
    }

    if (!(std::filesystem::exists(get_application_config_path()) || std::filesystem::create_directory(get_application_config_path())))
    {
        return make_error("Failed to create settings directory");
    }

    ApplicationSettings applicationSettings {};
    TabletSettings tabletSettings {};
    auto displays = MUST(get_available_displays());
    auto devices = TRY(get_available_devices());

    Context context { applicationSettings, tabletSettings, devices, displays };

    if (config.is_used("--load"))
    {
        auto result = load_tablet_settings();

        if (!result) return make_error("Failed to load device settings");
        if (context.devices.empty()) return make_error("Failed to load devices");

        context.tabletSettings = *result;

        auto stylus = std::ranges::find(context.devices, context.tabletSettings.stylus.name, &Device::name);
        assert(stylus != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
        context.stylus = *stylus;

        auto pad = std::ranges::find(context.devices, context.tabletSettings.pad.name, &Device::name);
        assert(pad != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
        context.pad = *pad;

        context.display = *std::ranges::find(context.displays, context.tabletSettings.display.name, &Display::name);

        TRY(apply_settings_from_context_to_device(context));

        fmt::println("Device settings loaded successfully");

        return {};
    }

    if (!std::filesystem::exists(APPLICATION_SETTINGS_FILE))
    {
        save_application_settings(applicationSettings);
    }
    else
    {
        auto result = load_application_settings();

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
        else
        {
            context.applicationSettings = *result;
        }
    }

    if (TRY(daemonize(NAME"-server")) == IsDaemon::TRUE)
    {
        IPCServer::the().start();
    }
    else
    {
        if (!cli.is_used("--no-gui"))
            TRY(run_gui(context));
        else
            TRY(run_no_gui(context));
    }

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
