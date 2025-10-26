#include "app/ui/ProfileWindow.hpp"

#include "app/core/Localisation.hpp"
#include "app/core/Scaling.hpp"

#include <imgui/extensions/imgui_toast.hpp>
#include <imgui/imgui.hpp>
#include <liberror/Try.hpp>

using namespace liberror;
using namespace std::literals;

Result<void> render_profile_window(Context& context, TabletSettings& settings)
{
    ImGui::Text("%s", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Profile_Tab_Name)));
    static std::array<char, 256> profileName;
    ImGui::SetNextItemWidth(300_scaled + ImGui::GetStyle().WindowPadding.x);
    ImGui::InputText("##ProfileName", profileName.data(), profileName.size());

    auto previousCursorPosition = ImGui::GetCursorPos();
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - (25_scaled + ImGui::GetStyle().WindowPadding.x));
    static auto isCreateButtonDisabled = false;
    ImGui::BeginDisabled(isCreateButtonDisabled);
    if (ImGui::Button(TRY(Localisation::get(context.settings.application.language, Localisation::Create)), { 150_scaled, 25_scaled }))
    {
        if (profileName.data() == ""sv)
        {
            ImGui::PushToast(
                TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Error)),
                TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Profile_Name_Empty))
            );
        }
        else
        {
            isCreateButtonDisabled = true;
            asio::co_spawn(context.stExecutor, [] (Context& context, TabletSettings& settings) -> asio::awaitable<void> {
                auto maybeProfile = co_await asio::co_spawn(context.mtExecutor, [] (auto tablet, auto display) -> asio::awaitable<Result<TabletProfile>> {
                    co_return make_tablet_profile(profileName.data(), tablet, display);
                }(context.tablet, context.display));
                isCreateButtonDisabled = false;

                if (!maybeProfile.has_value())
                {
                    ImGui::PushToast(
                        MUST(Localisation::get(context.settings.application.language, Localisation::Toast_Error)),
                        MUST(Localisation::get(context.settings.application.language, Localisation::Toast_Profile_Create_Failed))
                    );
                    co_return;
                }

                context.settings.tablet.profiles().insert({ maybeProfile->name, *maybeProfile });

                context.hasChangedDeviceSettings = true;

                co_await asio::co_spawn(context.mtExecutor, [] (auto settings) -> asio::awaitable<void> {
                    save_tablet_settings(settings);
                    co_return;
                }(context.settings.tablet));

                ImGui::PushToast(
                    MUST(Localisation::get(context.settings.application.language, Localisation::Toast_Success)),
                    MUST(Localisation::get(context.settings.application.language, Localisation::Toast_Profile_Create_Success))
                );
            }(context, settings), asio::detached);
            context.stExecutor.restart();
        }
    }
    ImGui::EndDisabled();
    ImGui::SetCursorPos(previousCursorPosition);

    return {};
}

Result<bool> render_profile_window(bool isWindowVisible, Context& context, TabletSettings& settings, TabletProfile& profile)
{
    auto isWindowClosed = false;

    static TabletProfile* currentProfile = nullptr;

    ImGui::Text("%s", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Profile_Tab_Name)));
    static std::array<char, 256> profileName;
    ImGui::SetNextItemWidth(300_scaled + ImGui::GetStyle().WindowPadding.x);
    ImGui::InputText("##ProfileName", profileName.data(), profileName.size());

    if (currentProfile != &profile)
    {
        currentProfile = &profile;
        std::ranges::fill_n(profileName.data(), profileName.size(), 0);
        std::ranges::copy(currentProfile->name, profileName.data());
    }

    auto previousCursorPosition = ImGui::GetCursorPos();
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - (25_scaled + ImGui::GetStyle().WindowPadding.x));

    static auto isSaveButtonDisabled = false;
    ImGui::BeginDisabled(isSaveButtonDisabled);
    if (ImGui::Button(TRY(Localisation::get(context.settings.application.language, Localisation::Save)), { 150_scaled, 25_scaled }))
    {
        if (profileName.data() == ""sv)
        {
            ImGui::PushToast(
                TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Error)),
                TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Profile_Name_Empty))
            );
        }
        else
        {
            isWindowClosed = true;
            asio::co_spawn(context.stExecutor, [] (Context& context, TabletSettings& settings, TabletProfile profile) -> asio::awaitable<void> {
                auto previousNameOfTheProfileBeingEdited = profile.name;
                auto previousNameOfTheCurrentProfile = context.settings.tablet.profile()->first;

                std::erase_if(context.settings.tablet.profiles(), [&] (auto const& entry) { return entry.first == profile.name; });

                profile.name = profileName.data();
                auto [iterator, _] = context.settings.tablet.profiles().insert({ profile.name, profile });

                if (previousNameOfTheProfileBeingEdited == previousNameOfTheCurrentProfile)
                {
                    context.settings.tablet.profile(iterator);
                }

                context.hasChangedDeviceSettings = true;

                co_await asio::co_spawn(context.mtExecutor, [] (auto settings) -> asio::awaitable<void> {
                    save_tablet_settings(settings);
                    co_return;
                }(context.settings.tablet));

                ImGui::PushToast(
                    MUST(Localisation::get(context.settings.application.language, Localisation::Toast_Success)),
                    MUST(Localisation::get(context.settings.application.language, Localisation::Toast_Profile_Update_Success))
                );
            }(context, settings, profile), asio::detached);
            context.stExecutor.restart();
        }
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    ImGui::BeginDisabled(settings.profiles().size() <= 2);
    if (ImGui::Button(MUST(Localisation::get(context.settings.application.language, Localisation::Delete)), { 150_scaled, 25_scaled }))
    {
        isWindowClosed = true;
        asio::co_spawn(context.stExecutor, [] (Context& context, TabletSettings& settings, TabletProfile& profile) -> asio::awaitable<void> {
            std::erase_if(context.settings.tablet.profiles(), [&] (auto& entry) { return entry.first == profile.name; });

            context.settings.tablet.profile(std::ranges::find_if(context.settings.tablet.profiles(), [&] (auto const& entry) {
                return entry.first != "INVALID";
            }));

            context.hasChangedDeviceSettings = true;

            co_await asio::co_spawn(context.mtExecutor, [] (auto settings) -> asio::awaitable<void> {
                save_tablet_settings(settings);
                co_return;
            }(context.settings.tablet));

            ImGui::PushToast(
                MUST(Localisation::get(context.settings.application.language, Localisation::Toast_Success)),
                MUST(Localisation::get(context.settings.application.language, Localisation::Toast_Profile_Delete_Success))
            );
        }(context, settings, profile), asio::detached);
        context.stExecutor.restart();
    }
    ImGui::EndDisabled();

    ImGui::SetCursorPos(previousCursorPosition);

    if (!isWindowVisible)
    {
        std::ranges::fill_n(profileName.data(), profileName.size(), 0);
        std::ranges::copy(currentProfile->name, profileName.data());
    }

    return isWindowClosed;
}
