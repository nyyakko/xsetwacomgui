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
#include "platform/Notify.hpp"
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

enum class Headless { FALSE, TRUE };

static asio::awaitable<void> ipc_message_handler(IPCClient& client, Context& context, Headless headless)
{
    static auto fnGetAvailableDevices = [] (auto shouldRetry) -> asio::awaitable<std::vector<Device>> {
        std::vector<Device> devices {};

        for (auto i = 0; i < 3; i += 1)
        {
            devices = MUST(get_available_devices());
            if (!(devices.empty() && shouldRetry)) break;
            std::this_thread::sleep_for(500ms);
        }

        co_return devices;
    };

    while (true)
    {
        auto message = co_await client.receive_message_async();

        auto action = magic_enum::enum_cast<UDevDevice::Action>(message.data());
        assert(action);

        if (*action == UDevDevice::Action::BIND && ranges::count(context.devices, Device::Kind::STYLUS, &Device::kind) < 1)
        {
            context.devices = co_await asio::co_spawn(context.mtExecutor, fnGetAvailableDevices(true));

            if (context.devices.empty())
            {
                if (headless == Headless::FALSE)
                {
                    ImGui::PushToast(
                        MUST(Localisation::get(context.settings.language(), Localisation::Toast_Error)),
                        MUST(Localisation::get(context.settings.language(), Localisation::Toast_Devices_Missing))
                    );
                }
                else
                {
                    MUST(notify_send("XSetWacomGUI", MUST(Localisation::get(context.settings.language(), Localisation::Toast_Devices_Missing))));
                }
                continue;
            }

            auto maybeSettings = co_await asio::co_spawn(context.mtExecutor, [] -> asio::awaitable<Result<TabletSettings, SettingsError>> {
                co_return load_tablet_settings();
            });
            assert(maybeSettings.has_value() && "how did you even manage to make this happen?");

            context.tablet.settings = *maybeSettings;

            auto stylus = ranges::find(context.devices, context.tablet.settings.profile()->second.stylus.name, &Device::name);
            assert(stylus != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
            context.tablet.stylus = *stylus;

            auto pad = ranges::find(context.devices, context.tablet.settings.profile()->second.pad.name, &Device::name);
            assert(pad != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
            context.tablet.pad = *pad;

            context.display = *ranges::find(context.displays, context.tablet.settings.profile()->second.display.name, &Display::name);

            context.hasChangedDeviceSettings = true;

            auto maybeLoaded = co_await asio::co_spawn(context.mtExecutor, [] (auto settings_, auto tablet_, auto display_) -> asio::awaitable<Result<void>> {
                co_return load_tablet_profile(settings_.profile()->second, tablet_.stylus, tablet_.pad, display_);
            }(context.tablet.settings, context.tablet, context.display));

            if (!maybeLoaded.has_value())
            {
                if (headless == Headless::FALSE)
                {
                    ImGui::PushToast(
                        MUST(Localisation::get(context.settings.language(), Localisation::Toast_Error)),
                        MUST(Localisation::get(context.settings.language(), Localisation::Toast_Profile_Load_Failed))
                    );
                }
                else
                {
                    MUST(notify_send("XSetWacomGUI", MUST(Localisation::get(context.settings.language(), Localisation::Toast_Profile_Load_Failed))));
                }
                continue;
            }

            if (headless == Headless::TRUE)
            {
                MUST(notify_send("XSetWacomGUI", MUST(Localisation::get(context.settings.language(), Localisation::Toast_Device_Settings_Load_Success))));
            }
        }

        if (*action == UDevDevice::Action::UNBIND && ranges::count(context.devices, Device::Kind::STYLUS, &Device::kind) <= 1)
        {
            context.devices = co_await asio::co_spawn(context.mtExecutor, fnGetAvailableDevices(false));

            if (ranges::find(context.devices, context.tablet.settings.profile()->second.stylus.name, &Device::name) != context.devices.end())
            {
                continue;
            }

            context.display = {};
            context.tablet = {};

            context.hasChangedDeviceSettings = true;
        }
    }

    co_return;
}

static Result<void> run_gui()
{
    static Context context {
        .devices  = TRY(get_available_devices()),
        .displays = TRY(get_available_displays()),
    };

    static auto client = TRY(IPCClient::create(context.stExecutor));
    TRY(client.connect());

    struct sigaction action;
    action.sa_handler = [] (int) { client.~IPCClient(); _exit(0); };
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);

    if (!glfwInit()) return make_error("Failed to initialize glfw");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    if (!std::filesystem::exists(APPLICATION_SETTINGS_FILE))
    {
        TRY(save_application_settings(context.settings));
    }
    else
    {
        auto result = load_application_settings();

        if (!result.has_value())
        {
            fmt::println("The currently saved application settings");
            fmt::println("differs from the expected format. You can:\n");

            fmt::println("1. Overwrite Everything");
            fmt::println("2. Migrate Manually\n");

            while (true)
            {
                fmt::print("How would you like to proceed? (choose a value): ");

                auto choice = 0; std::cin >> choice;
                if (choice == 1 || choice == 2)
                {
                    if (choice == 1) TRY(save_application_settings(context.settings));
                    if (choice == 2) TRY(migrate_application_settings(context.settings));
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

    auto guard = asio::make_work_guard(context.stExecutor);
    asio::co_spawn(context.stExecutor, ipc_message_handler(client, context, Headless::FALSE), asio::detached);

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
            ImGui::Begin(NAME, nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBringToFrontOnFocus);
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

static Result<void> run_no_gui()
{
    TRY(daemonize(NAME"-client", Detached::TRUE));

    static Context context {
        .devices  = TRY(get_available_devices()),
        .displays = TRY(get_available_displays()),
    };

    static auto client = TRY(IPCClient::create(context.stExecutor));
    TRY(client.connect());

    struct sigaction action;
    action.sa_handler = [] (int) { client.~IPCClient(); _exit(0); };
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);

    if (!std::filesystem::exists(APPLICATION_SETTINGS_FILE))
    {
        TRY(save_application_settings(context.settings));
    }
    else
    {
        auto result = load_application_settings();

        if (!result.has_value())
        {
            fmt::println("The currently saved application settings");
            fmt::println("differs from the expected format. You can:\n");

            fmt::println("1. Overwrite Everything");
            fmt::println("2. Migrate Manually\n");

            while (true)
            {
                fmt::print("How would you like to proceed? (choose a value): ");

                auto choice = 0; std::cin >> choice;
                if (choice == 1 || choice == 2)
                {
                    if (choice == 1) TRY(save_application_settings(context.settings));
                    if (choice == 2) TRY(migrate_application_settings(context.settings));
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

    if (!context.devices.empty())
    {
        auto result = load_tablet_settings();
        if (!result) return make_error("Failed to load device settings");

        context.tablet.settings = *result;

        auto stylus = ranges::find(context.devices, context.tablet.settings.profile()->second.stylus.name, &Device::name);
        assert(stylus != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
        context.tablet.stylus = *stylus;

        auto pad = ranges::find(context.devices, context.tablet.settings.profile()->second.pad.name, &Device::name);
        assert(pad != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
        context.tablet.pad = *pad;

        context.display = *ranges::find(context.displays, context.tablet.settings.profile()->second.display.name, &Display::name);

        TRY(load_tablet_profile(context.tablet.settings.profile()->second, context.tablet.stylus, context.tablet.pad, context.display));

        spdlog::info("Device settings (profile: {}) loaded successfully", context.tablet.settings.profile()->second.name);
    }

    auto guard = asio::make_work_guard(context.stExecutor);
    asio::co_spawn(context.stExecutor, ipc_message_handler(client, context, Headless::TRUE), asio::detached);
    context.stExecutor.run();

    return {};
}

static Result<void> safe_main(std::span<char const*> const& arguments)
{
    auto mainHelp = [] {
        fmt::println("Usage: " NAME " [--help] [--no-gui] {{config}}");
        fmt::println("\na graphical xsetwacom wrapper for ease of use");
        fmt::println("\nOptional arguments:");
        fmt::println("  --help {:>21}", "shows help message");
        fmt::println("  --no-gui {:>35}", "runs the program in the background");
    };

    if (auto posHelp = ranges::find(arguments, "--help"sv); posHelp != arguments.end())
    {
        if (std::distance(arguments.begin(), posHelp) == 1) mainHelp();
        return {};
    }

    if (!(std::filesystem::exists(get_application_config_path()) || std::filesystem::create_directory(get_application_config_path())))
    {
        return make_error("Failed to create settings directory");
    }

    if (TRY(daemonize(NAME"-server")) == IsDaemon::TRUE)
    {
        asio::io_context executor;
        auto guard = asio::make_work_guard(executor);

        static auto server = TRY(IPCServer::create(executor));

        struct sigaction action;
        action.sa_handler = [] (int) { server.~IPCServer(); _exit(0); };
        sigaction(SIGINT, &action, NULL);
        sigaction(SIGTERM, &action, NULL);

        server.start();

        return {};
    }
    else
    {
        if (ranges::find(arguments, "--no-gui"sv) != arguments.end())
        {
            TRY(run_no_gui());
        }
        else
        {
            TRY(run_gui());
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
