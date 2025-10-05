#define IMGUI_DEFINE_MATH_OPERATORS

#include <spdlog/spdlog.h>

#include "core/Context.hpp"
#include "core/Help.hpp"
#include "core/ipc/Client.hpp"
#include "core/ipc/Server.hpp"
#include "GoddessWindow.hpp"
#include "MainWindow.hpp"
#include "platform/Daemon.hpp"
#include "platform/Environment.hpp"
#include "platform/udev/UDevDevice.hpp"
#include "SettingsWindow.hpp"
#include "ui/Localisation.hpp"
#include "ui/Scaling.hpp"

#include <fplus/container_common.hpp>
#include <fplus/fplus.hpp>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <imgui/extensions/imgui_toast.hpp>
#include <imgui/imgui.hpp>
#include <imgui/imgui_impl_glfw.hpp>
#include <imgui/imgui_impl_opengl3.hpp>
#include <liberror/Try.hpp>
#include <libexec/Execute.hpp>
#include <magic_enum/magic_enum.hpp>
#include <scn/scan.h>

#include <sys/poll.h>

#include <span>

using namespace liberror;
using namespace std::literals;

Result<void> push_system_toast(std::string_view message)
{
    static auto icon = get_application_icon_path() / "64x64" / "apps" / NAME".png";
    auto [out, err] = TRY(libexec::execute("notify-send", { "XSetWacomGUI", message.data(), "--icon", icon.string() }));
    if (!err.empty()) return make_error(err);
    return {};
}

Result<void> run_gui(Context& context)
{
    TRY(IPCClient::the().configure(IPCClient::Mode::ASYNC));
    TRY(IPCClient::the().connect());

    if (!glfwInit()) return make_error("Failed to initialize glfw");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    set_scale(context.settings.application.scale);

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

    if (context.settings.application.font.family != "Default")
    {
        font = io.Fonts->AddFontFromFileTTF(context.settings.application.font.path.string().data(), 20_scaled, nullptr, ranges.Data);
    }

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;

        if (context.settings.application.theme == ApplicationSettings::Theme::DARK)
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
                static bool isSettingsWindowOpen = false;
                static bool isGoddessWindowOpen = false;

                if (ImGui::BeginMenuBar())
                {
                    if (ImGui::BeginMenu(TRY(Localisation::get(context.settings.application.language, Localisation::MenuBar_Settings))))
                    {
                        if (ImGui::MenuItem(TRY(Localisation::get(context.settings.application.language, Localisation::MenuBar_Settings_Application))))
                        {
                            isSettingsWindowOpen = true;
                        }

                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu(TRY(Localisation::get(context.settings.application.language, Localisation::MenuBar_Other))))
                    {
                        if (ImGui::MenuItem(TRY(Localisation::get(context.settings.application.language, Localisation::MenuBar_Other_Goddess))))
                        {
                            isGoddessWindowOpen = true;
                        }

                        ImGui::EndMenu();
                    }

                    ImGui::EndMenuBar();
                }

                if (isSettingsWindowOpen)
                {
                    float applicationSettingsWidth = static_cast<float>(windowWidth)/1.5f, applicationSettingsHeight = static_cast<float>(windowHeight)/1.5f;
                    ImGui::SetNextWindowSize({ applicationSettingsWidth, applicationSettingsHeight });
                    ImGui::SetNextWindowPos({ (static_cast<float>(windowWidth) - applicationSettingsWidth)/2, (static_cast<float>(windowHeight) - applicationSettingsHeight)/2 });
                    ImGui::Begin(
                        TRY(Localisation::get(context.settings.application.language, Localisation::MenuBar_Settings_Application)),
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
                    float goddessWidth = static_cast<float>(windowWidth)/1.5f, goddessHeight = static_cast<float>(windowHeight)/1.5f;
                    ImGui::SetNextWindowSize({ goddessWidth, goddessHeight });
                    ImGui::SetNextWindowPos({ (static_cast<float>(windowWidth) - goddessWidth)/2, (static_cast<float>(windowHeight) - goddessHeight)/2 });
                    ImGui::Begin(
                        TRY(Localisation::get(context.settings.application.language, Localisation::MenuBar_Other_Goddess)),
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

Result<void> run_no_gui(Context& context)
{
    TRY(daemonize(NAME"-client", QuitParent::TRUE));

    TRY(IPCClient::the().configure(IPCClient::Mode::SYNC));
    TRY(IPCClient::the().connect());

    if (!context.devices.empty())
    {
        auto result = load_tablet_settings();

        if (!result) return make_error("Failed to load device settings");
        if (context.devices.empty()) return make_error("Failed to load devices");

        context.settings.tablet = *result;

        auto stylus = std::ranges::find(context.devices, context.settings.tablet->stylus.name, &Device::name);
        assert(stylus != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
        context.tablet.stylus = *stylus;

        auto pad = std::ranges::find(context.devices, context.settings.tablet->pad.name, &Device::name);
        assert(pad != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
        context.tablet.pad = *pad;

        context.display = *std::ranges::find(context.displays, context.settings.tablet->display.name, &Display::name);

        TRY(TabletSettings::Profile::load_to_tablet(context.settings.tablet.get_current_profile(), context.tablet, context.display));

        fmt::println("Device settings loaded successfully");
    }

    while (true)
    {
        auto message = TRY(IPCClient::the().receive_message());

        auto action = magic_enum::enum_cast<UDevDevice::Action>(message.data());
        assert(action && "INVALID ACTION");

        switch (*action)
        {
        case UDevDevice::Action::BIND: {
            if (std::ranges::count(context.devices, Device::Kind::STYLUS, &Device::kind) >= 1) break;

            while (context.devices = TRY(get_available_devices()), context.devices.empty())
            {
                if (static auto retry = 0; retry++ == 3) break;
                spdlog::info("No devices were found, retrying...");
                std::this_thread::sleep_for(250ms);
            }

            if (context.devices.empty())
            {
                push_system_toast(TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Devices_Missing)));
                break;
            }

            auto result = load_tablet_settings();

            if (!result.has_value())
            {
                context.tablet.stylus = fplus::keep_if([] (auto&& device) { return device.kind == Device::Kind::STYLUS; }, context.devices).back();
                context.tablet.pad = fplus::keep_if([] (auto&& device) { return device.kind == Device::Kind::PAD; }, context.devices).back();
                context.display = TRY(get_primary_display());

                context.settings.tablet.profiles.emplace("Default", TRY(TabletSettings::Profile::make_default(context.tablet, context.display)));
                context.settings.tablet.profile = "Default";

                TRY(TabletSettings::Profile::load_to_tablet(context.settings.tablet.get_current_profile(), context.tablet, context.display));

                switch (result.error().message())
                {
                case SettingsError::Type::WRITE_FAILURE: break;
                case SettingsError::Type::FILE_NOT_FOUND: {
                    TRY(push_system_toast(TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Device_Settings_Missing))));
                    break;
                }
                case SettingsError::Type::PROFILE_NOT_FOUND: {
                    TRY(push_system_toast("Could not find previously selected profile"));
                    break;
                }
                case SettingsError::Type::READ_FAILURE: {
                    TRY(push_system_toast(TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Device_Settings_Load_Failed))));
                    break;
                }
                case SettingsError::Type::OUTDATED_SCHEMA: {
                    TRY(push_system_toast(TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Device_Settings_Outdated_Schema))));
                    break;
                }
                }
            }
            else
            {
                context.settings.tablet = *result;

                auto stylus = std::ranges::find(context.devices, context.settings.tablet->stylus.name, &Device::name);
                assert(stylus != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
                context.tablet.stylus = *stylus;

                auto pad = std::ranges::find(context.devices, context.settings.tablet->pad.name, &Device::name);
                assert(pad != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
                context.tablet.pad = *pad;

                context.display = *std::ranges::find(context.displays, context.settings.tablet->display.name, &Display::name);

                TRY(TabletSettings::Profile::load_to_tablet(context.settings.tablet.get_current_profile(), context.tablet, context.display));

                spdlog::info("Device settings loaded successfully");
            }

            break;
        }
        case UDevDevice::Action::UNBIND: {
            if (std::ranges::count(context.devices, Device::Kind::STYLUS, &Device::kind) > 1) break;

            context.devices = TRY(get_available_devices());

            auto maybeDevice = std::ranges::find(context.devices, context.settings.tablet->stylus.name, &Device::name);

            if (maybeDevice == context.devices.end())
            {
                context.display = {};
                context.tablet = {};
                context.settings.tablet = {};
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
    if (std::ranges::find(arguments, "--help"sv) != arguments.end())
    {
        get_help(arguments);
        return {};
    }

    if (!(std::filesystem::exists(get_application_config_path()) || std::filesystem::create_directory(get_application_config_path())))
    {
        return make_error("Failed to create settings directory");
    }

    Context context {
        {},
        TRY(get_available_devices()),
        TRY(get_available_displays())
    };

    if (auto posConfig = std::ranges::find(arguments, "config"sv); posConfig != arguments.end())
    {
        auto commandArguments = arguments.subspan(size_t(std::distance(arguments.begin(), posConfig)));

        if (auto posLoad = std::ranges::find(commandArguments, "--load"sv); posLoad != arguments.end())
        {
            auto result = load_tablet_settings();

            if (!result) return make_error("Failed to load device settings");
            if (context.devices.empty()) return make_error("Failed to load devices");

            context.settings.tablet = *result;

            auto stylus = std::ranges::find(context.devices, context.settings.tablet->stylus.name, &Device::name);
            assert(stylus != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
            context.tablet.stylus = *stylus;

            auto pad = std::ranges::find(context.devices, context.settings.tablet->pad.name, &Device::name);
            assert(pad != context.devices.end() && "FIXME: assuming device connected is the same as the one saved in the settings file");
            context.tablet.pad = *pad;

            context.display = *std::ranges::find(context.displays, context.settings.tablet->display.name, &Display::name);

            TRY(TabletSettings::Profile::load_to_tablet(context.settings.tablet.get_current_profile(), context.tablet, context.display));

            fmt::println("Device settings loaded successfully");

            return {};
        }
    }

    if (!std::filesystem::exists(APPLICATION_SETTINGS_FILE))
    {
        save_application_settings(context.settings.application);
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
                if (choice->value() == 1) save_application_settings(context.settings.application);
                else if (choice->value() == 2) migrate_application_settings(context.settings.application);
            }

            fmt::println("Done. Restart the application.");

            return {};
        }
        else
        {
            context.settings.application = *result;
        }
    }

    if (TRY(daemonize(NAME"-server")) == IsDaemon::TRUE)
    {
        IPCServer::the().start();
    }
    else
    {
        if (std::ranges::find(arguments, "--no-gui"sv) != arguments.end())
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
