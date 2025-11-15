#include "app/ui/ProfileWindow.hpp"

#include "app/core/Localisation.hpp"
#include "app/core/Scaling.hpp"
#include "utils/MakeAsync.hpp"

#include <imgui/extensions/imgui_toast.hpp>
#include <imgui/imgui.hpp>
#include <liberror/Result.hpp>
#include <liberror/Try.hpp>
#include <range/v3/algorithm.hpp>

using namespace liberror;
using namespace std::literals;

Result<void> render_profile_window(Context& context)
{
    ImGui::Text("%s", TRY(Localisation::get(context.settings.language(), Localisation::Window_Profile_Tab_Name)));
    static std::array<char, 256> profileName;
    ImGui::SetNextItemWidth(300_scaled + ImGui::GetStyle().WindowPadding.x);
    ImGui::InputText("##ProfileName", profileName.data(), profileName.size());

    auto previousCursorPosition = ImGui::GetCursorPos();
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - (25_scaled + ImGui::GetStyle().WindowPadding.x));
    static auto isCreateButtonDisabled = false;
    ImGui::BeginDisabled(isCreateButtonDisabled);
    if (ImGui::Button(TRY(Localisation::get(context.settings.language(), Localisation::Create)), { 150_scaled, 25_scaled }))
    {
        if (profileName.data() == ""sv)
        {
            ImGui::PushToast(
                TRY(Localisation::get(context.settings.language(), Localisation::Toast_Error)),
                TRY(Localisation::get(context.settings.language(), Localisation::Toast_Profile_Name_Empty))
            );
        }
        else
        {
            isCreateButtonDisabled = true;
            asio::co_spawn(context.stExecutor, [] (Context& context_) -> asio::awaitable<void> {
                auto maybeProfile = co_await asio::co_spawn(context_.mtExecutor, make_async<make_tablet_profile>(profileName.data(), auto(context_.tablet.stylus), auto(context_.tablet.pad), auto(context_.display)));
                isCreateButtonDisabled = false;
                if (!maybeProfile.has_value())
                {
                    ImGui::PushToast(
                        MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Error)),
                        MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Profile_Create_Failed))
                    );
                    co_return;
                }

                context_.tablet.settings.profiles().insert({ maybeProfile->name, *maybeProfile });

                MUST(co_await asio::co_spawn(context_.mtExecutor, make_async<save_tablet_settings>(auto(context_.tablet.settings))));

                ImGui::PushToast(
                    MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Success)),
                    MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Profile_Create_Success))
                );
            }(context), asio::detached);
        }
    }
    ImGui::EndDisabled();
    ImGui::SetCursorPos(previousCursorPosition);

    return {};
}

Result<bool> render_profile_window(bool isWindowVisible, Context& context, TabletProfile& profile)
{
    static TabletProfile* currentProfile = nullptr;

    ImGui::Text("%s", TRY(Localisation::get(context.settings.language(), Localisation::Window_Profile_Tab_Name)));
    static std::array<char, 256> profileName;
    ImGui::SetNextItemWidth(300_scaled + ImGui::GetStyle().WindowPadding.x);
    ImGui::InputText("##ProfileName", profileName.data(), profileName.size());

    if (currentProfile != &profile)
    {
        currentProfile = &profile;
        ranges::fill_n(profileName.data(), profileName.size(), 0);
        ranges::copy(currentProfile->name, profileName.data());
    }

    auto previousCursorPosition = ImGui::GetCursorPos();
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - (25_scaled + ImGui::GetStyle().WindowPadding.x));

    auto isWindowClosed = false;

    static auto isSaveButtonDisabled = false;
    ImGui::BeginDisabled(isSaveButtonDisabled);
    if (ImGui::Button(TRY(Localisation::get(context.settings.language(), Localisation::Save)), { 150_scaled, 25_scaled }))
    {
        if (profileName.data() == ""sv)
        {
            ImGui::PushToast(
                TRY(Localisation::get(context.settings.language(), Localisation::Toast_Error)),
                TRY(Localisation::get(context.settings.language(), Localisation::Toast_Profile_Name_Empty))
            );
        }
        else
        {
            isWindowClosed = true;
            asio::co_spawn(context.stExecutor, [] (Context& context_, TabletProfile profile_) -> asio::awaitable<void> {
                auto nameOfTheProfileBeingEdited = profile_.name;
                auto nameOfTheCurrentProfile = context_.tablet.settings.profile()->first;

                std::erase_if(context_.tablet.settings.profiles(), [&] (auto const& entry) { return entry.first == profile_.name; });

                profile_.name = profileName.data();
                auto [iterator, _] = context_.tablet.settings.profiles().insert({ profile_.name, profile_ });

                if (nameOfTheProfileBeingEdited == nameOfTheCurrentProfile)
                {
                    context_.tablet.settings.profile(iterator);
                }

                context_.hasChangedDeviceSettings = true;

                co_await asio::co_spawn(context_.mtExecutor, make_async<save_tablet_settings>(auto(context_.tablet.settings)));

                ImGui::PushToast(
                    MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Success)),
                    MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Profile_Update_Success))
                );
            }(context, profile), asio::detached);
        }
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    ImGui::BeginDisabled(context.tablet.settings.profiles().size() <= 2);
    if (ImGui::Button(MUST(Localisation::get(context.settings.language(), Localisation::Delete)), { 150_scaled, 25_scaled }))
    {
        isWindowClosed = true;
        asio::co_spawn(context.stExecutor, [] (Context& context_, TabletProfile& profile_) -> asio::awaitable<void> {
            std::erase_if(context_.tablet.settings.profiles(), [&] (auto const& entry) { return entry.first == profile_.name; });

            context_.tablet.settings.profile(ranges::find_if(context_.tablet.settings.profiles(), [&] (auto const& entry) {
                return entry.first != "INVALID";
            }));

            context_.hasChangedDeviceSettings = true;

            co_await asio::co_spawn(context_.mtExecutor, make_async<save_tablet_settings>(auto(context_.tablet.settings)));

            ImGui::PushToast(
                MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Success)),
                MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Profile_Delete_Success))
            );
        }(context, profile), asio::detached);
    }
    ImGui::EndDisabled();

    ImGui::SetCursorPos(previousCursorPosition);

    if (!isWindowVisible)
    {
        ranges::fill_n(profileName.data(), profileName.size(), 0);
        ranges::copy(currentProfile->name, profileName.data());
    }

    return isWindowClosed;
}
