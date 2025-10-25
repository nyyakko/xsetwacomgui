#include "ProfileWindow.hpp"

#include "ui/Localisation.hpp"
#include "ui/Scaling.hpp"

#include <imgui/extensions/imgui_toast.hpp>
#include <imgui/imgui.hpp>
#include <liberror/Try.hpp>

using namespace liberror;
using namespace std::literals;

Result<void> render_profile_window(Context& context)
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
            asio::co_spawn(context.stExecutor, [] (Context& context) -> asio::awaitable<void> {
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

                context.settings.tablet.add_profile(*maybeProfile);

                co_await asio::co_spawn(context.mtExecutor, [] (auto settings) -> asio::awaitable<void> {
                    save_tablet_settings(settings);
                    co_return;
                }(context.settings.tablet));

                ImGui::PushToast(
                    MUST(Localisation::get(context.settings.application.language, Localisation::Toast_Success)),
                    MUST(Localisation::get(context.settings.application.language, Localisation::Toast_Profile_Create_Success))
                );
            }(context), asio::detached);
            context.stExecutor.restart();
        }
    }
    ImGui::EndDisabled();
    ImGui::SetCursorPos(previousCursorPosition);

    return {};
}
