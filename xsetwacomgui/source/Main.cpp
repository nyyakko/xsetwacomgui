#define IMGUI_DEFINE_MATH_OPERATORS

#include <spdlog/spdlog.h>

#include "app/Context.hpp"
#include "app/core/ipc/IPCClient.hpp"
#include "app/core/ipc/IPCServer.hpp"
#include "app/core/Localisation.hpp"
#include "app/core/Scaling.hpp"
#include "app/ui/GoddessWindow.hpp"
#include "app/ui/MainWindow.hpp"
#include "app/ui/SettingsWindow.hpp"
#include "platform/Daemon.hpp"
#include "platform/Environment.hpp"
#include "platform/udev/UDevDevice.hpp"

#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <imgui/extensions/imgui_toast.hpp>
#include <imgui/imgui.hpp>
#include <imgui/imgui_impl_glfw.hpp>
#include <imgui/imgui_impl_opengl3.hpp>
#include <liberror/Try.hpp>
#include <libexec/Execute.hpp>
#include <magic_enum/magic_enum.hpp>
#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>

#include <sys/poll.h>

#include <iostream>
#include <limits>
#include <span>

using namespace liberror;
using namespace std::literals;

static void configure_signal_handler(void(*handler)(int))
{
    struct sigaction action;

    action.sa_handler = handler;

    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
}

static Result<void> run_gui(Context& context)
{
    TRY(IPCClient::the().configure(IPCClient::Mode::ASYNC));
    TRY(IPCClient::the().connect());

    configure_signal_handler([] (int) {
        IPCClient::the().~IPCClient();
        _exit(0);
    });

    if (!glfwInit()) return make_error("Failed to initialize glfw");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    set_scale(context.settings.scale());

#ifdef DEBUG
    auto window = glfwCreateWindow(int(800_scaled), int(815_scaled), NAME " - DEBUG BUILD", nullptr, nullptr);
#else
    auto window = glfwCreateWindow(int(800_scaled), int(815_scaled), NAME, nullptr, nullptr);
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

    ImVector<ImWchar> glyphRanges {};
    ImFontGlyphRangesBuilder glyphRangesBuilder {};
    static const ImWchar glyphRangesData[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
        0,
    };
    glyphRangesBuilder.AddRanges(glyphRangesData);
    glyphRangesBuilder.BuildRanges(&glyphRanges);

    if (context.settings.font().family != "Default")
    {
        font = io.Fonts->AddFontFromFileTTF(context.settings.font().path.string().data(), 20_scaled, nullptr, glyphRanges.Data);
    }

    auto executorGuard = asio::make_work_guard(context.stExecutor);

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;

        if (context.settings.theme() == ApplicationSettings::Theme::DARK)
        {
            ImGui::StyleColorsDark();
        }
        else
        {
            ImGui::StyleColorsLight();
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        if (context.hasChangedFont || context.hasChangedFontStyle)
        {
            font = io.Fonts->AddFontFromFileTTF(context.settings.font().path.string().data(), 20_scaled, nullptr, glyphRanges.Data);
            ImGui_ImplOpenGL3_CreateFontsTexture();
        }

        ImGui::NewFrame();

        context.stExecutor.poll();

        ImGui::PushFont(font);
        {
            int windowWidth, windowHeight;
            glfwGetWindowSize(window, &windowWidth, &windowHeight);
            ImGui::SetNextWindowPos({});
            ImGui::SetNextWindowSize({ float(windowWidth), float(windowHeight) });
            ImGui::Begin(NAME, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);
            {
                ImGui::RenderToasts();
#ifdef DEBUG
                static auto warnDebugBuild = true;

                if (warnDebugBuild)
                {
                    ImGui::PushToast("Debug", "You are running a DEBUG build!");
                    warnDebugBuild = false;
                }
#endif

                static auto isSettingsWindowOpen = false;
                static auto isGoddessWindowOpen = false;

                if (ImGui::BeginMenuBar())
                {
                    if (ImGui::BeginMenu(TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_MenuBar_Settings))))
                    {
                        if (ImGui::MenuItem(TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_MenuBar_Settings_Application))))
                        {
                            isSettingsWindowOpen = true;
                        }

                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu(TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_MenuBar_Other))))
                    {
                        if (ImGui::MenuItem(TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_MenuBar_Other_Goddess))))
                        {
                            isGoddessWindowOpen = true;
                        }

                        ImGui::EndMenu();
                    }

                    ImGui::EndMenuBar();
                }

                if (isSettingsWindowOpen)
                {
                    float applicationSettingsWidth = float(windowWidth)/1.5f, applicationSettingsHeight = float(windowHeight)/1.5f;
                    ImGui::SetNextWindowSize({ applicationSettingsWidth, applicationSettingsHeight });
                    ImGui::SetNextWindowPos({ (float(windowWidth) - applicationSettingsWidth)/2, (float(windowHeight) - applicationSettingsHeight)/2 });
                    ImGui::Begin(
                        TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_MenuBar_Settings_Application)),
                        &isSettingsWindowOpen,
                        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
                    );
                    {
                        TRY(render_settings_window(context));
                    }
                    ImGui::End();
                }

                if (isGoddessWindowOpen)
                {
                    float goddessWidth = float(windowWidth)/1.5f, goddessHeight = float(windowHeight)/1.5f;
                    ImGui::SetNextWindowSize({ goddessWidth, goddessHeight });
                    ImGui::SetNextWindowPos({ (float(windowWidth) - goddessWidth)/2, (float(windowHeight) - goddessHeight)/2 });
                    ImGui::Begin(
                        TRY(Localisation::get(context.settings.language(), Localisation::Window_Main_MenuBar_Other_Goddess)),
                        &isGoddessWindowOpen,
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

static Result<void> run_no_gui(Context& context)
{
    TRY(daemonize(NAME"-client", Detached::TRUE));

    TRY(IPCClient::the().configure(IPCClient::Mode::SYNC));
    TRY(IPCClient::the().connect());

    configure_signal_handler([] (int) {
        IPCClient::the().~IPCClient();
        _exit(0);
    });

    if (!context.devices.empty())
    {
        auto result = load_tablet_settings();

        if (!result) return make_error("Failed to load device settings");
        if (context.devices.empty()) return make_error("Failed to load devices");

        context.tablet.settings = *result;

        auto stylus = ranges::find(context.devices, context.tablet.settings.profile()->second.stylus.name, &Device::name);
        assert(stylus != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
        context.tablet.stylus = *stylus;

        auto pad = ranges::find(context.devices, context.tablet.settings.profile()->second.pad.name, &Device::name);
        assert(pad != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
        context.tablet.pad = *pad;

        context.display = *ranges::find(context.displays, context.tablet.settings.profile()->second.display.name, &Display::name);

        TRY(load_tablet_profile(context.tablet.settings.profile()->second, context.tablet, context.display));

        spdlog::info("Device settings (profile: {}) loaded successfully", context.tablet.settings.profile()->second.name);
    }

    while (true)
    {
        auto message = TRY(IPCClient::the().receive_message());

        auto action = magic_enum::enum_cast<UDevDevice::Action>(message.data());
        assert(action && "INVALID ACTION");

        if (*action == UDevDevice::Action::BIND && ranges::count(context.devices, Device::Kind::STYLUS, &Device::kind) < 1)
        {
            while (context.devices = TRY(get_available_devices()), context.devices.empty())
            {
                if (static auto retry = 0; retry++ == 3) break;
                spdlog::info("No devices were found, retrying...");
                std::this_thread::sleep_for(250ms);
            }

            if (context.devices.empty())
            {
                static auto icon = get_application_icon_path() / "64x64" / "apps" / NAME".png";
                auto [out, err] = TRY(libexec::execute("notify-send", {
                    "XSetWacomGUI", TRY(Localisation::get(context.settings.language(), Localisation::Toast_Devices_Missing)), "--icon", icon.string()
                }));
                if (!err.empty()) return make_error(err);
                continue;
            }

            auto result = load_tablet_settings();
            assert(result.has_value() && "how did you even manage to make this happen?");

            context.tablet.settings = *result;

            auto stylus = ranges::find(context.devices, context.tablet.settings.profile()->second.stylus.name, &Device::name);
            assert(stylus != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
            context.tablet.stylus = *stylus;

            auto pad = ranges::find(context.devices, context.tablet.settings.profile()->second.pad.name, &Device::name);
            assert(pad != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
            context.tablet.pad = *pad;

            context.display = *ranges::find(context.displays, context.tablet.settings.profile()->second.display.name, &Display::name);

            TRY(load_tablet_profile(context.tablet.settings.profile()->second, context.tablet, context.display));

            spdlog::info("Device settings (profile: {}) loaded successfully", context.tablet.settings.profile()->second.name);
        }

        if (*action == UDevDevice::Action::UNBIND && ranges::count(context.devices, Device::Kind::STYLUS, &Device::kind) <= 1)
        {
            context.devices = TRY(get_available_devices());

            auto maybeDevice = ranges::find(context.devices, context.tablet.settings.profile()->second.stylus.name, &Device::name);

            if (maybeDevice == context.devices.end())
            {
                context.display = {};
                context.tablet = {};
                context.tablet.settings = {};
            }
        }
    }

    return {};
}

Result<void> safe_main(std::span<char const*> const& arguments)
{
    auto mainHelp = [] {
        fmt::println("Usage: " NAME " [--help] [--no-gui] {{config}}");
        fmt::println("\na graphical xsetwacom wrapper for ease of use");
        fmt::println("\nOptional arguments:");
        fmt::println("  --help {:>21}", "shows help message");
        fmt::println("  --no-gui {:>35}", "runs the program in the background");
        fmt::println("\nSubcommands:");
        fmt::println("  config {:>39}", "manages device related configuration");
    };

    auto configHelp = [] {
        fmt::println("Usage: " NAME " config [--help] [--load]");
        fmt::println("\nmanages device related configuration");
        fmt::println("\nOptional arguments:");
        fmt::println("  --help {:>21}", "shows help message");
        fmt::println("  --load {:>39}", "loads the saved tablet configuration");
    };

    if (auto posHelp = ranges::find(arguments, "--help"sv); posHelp != arguments.end())
    {
        if (std::distance(arguments.begin(), posHelp) == 1) mainHelp();

        if (auto posConfig = ranges::find(arguments, "config"sv); posConfig != arguments.end())
        {
            auto commandArguments = arguments.subspan(size_t(std::distance(arguments.begin(), posConfig)));

            if (posHelp = ranges::find(commandArguments, "--help"sv); posHelp != arguments.end())
            {
                configHelp();
            }
        }

        return {};
    }

    if (!(std::filesystem::exists(get_application_config_path()) || std::filesystem::create_directory(get_application_config_path())))
    {
        return make_error("Failed to create settings directory");
    }

    Context context {
        .devices = TRY(get_available_devices()),
        .displays = TRY(get_available_displays()),
    };

    if (auto posConfig = ranges::find(arguments, "config"sv); posConfig != arguments.end())
    {
        auto commandArguments = arguments.subspan(size_t(std::distance(arguments.begin(), posConfig)));

        if (auto posLoad = ranges::find(commandArguments, "--load"sv); posLoad != arguments.end())
        {
            auto result = load_tablet_settings();

            if (!result) return make_error("Failed to load device settings");
            if (context.devices.empty()) return make_error("Failed to load devices");

            context.tablet.settings = *result;

            auto stylus = ranges::find(context.devices, context.tablet.settings.profile()->second.stylus.name, &Device::name);
            assert(stylus != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
            context.tablet.stylus = *stylus;

            auto pad = ranges::find(context.devices, context.tablet.settings.profile()->second.pad.name, &Device::name);
            assert(pad != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
            context.tablet.pad = *pad;

            context.display = *ranges::find(context.displays, context.tablet.settings.profile()->second.display.name, &Display::name);

            TRY(load_tablet_profile(context.tablet.settings.profile()->second, context.tablet, context.display));

            fmt::println("Device settings loaded successfully");

            return {};
        }
    }

    if (!std::filesystem::exists(APPLICATION_SETTINGS_FILE))
    {
        save_application_settings(context.settings);
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

            while (true)
            {
                fmt::print("How would you like to proceed? (choose a value): ");

                auto choice = 0; std::cin >> choice;
                if (choice == 1 || choice == 2)
                {
                    if (choice == 1) save_application_settings(context.settings);
                    if (choice == 2) migrate_application_settings(context.settings);
                    break;
                }
                else
                {
                    fmt::println("\nInvalid option. Choose either 1 or 2.\n");
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                }
            }

            fmt::println("Done. Restart the application.");

            return {};
        }
        else
        {
            context.settings = *result;
        }
    }

    if (TRY(daemonize(NAME"-server")) == IsDaemon::TRUE)
    {
        configure_signal_handler([] (int) {
            IPCServer::the().~IPCServer();
            _exit(0);
        });

        IPCServer::the().start();
    }
    else
    {
        if (ranges::find(arguments, "--no-gui"sv) != arguments.end())
        {
            TRY(run_no_gui(context));
        }
        else
        {
            TRY(run_gui(context));
        }
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
