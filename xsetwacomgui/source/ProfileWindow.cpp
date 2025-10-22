#include "ProfileWindow.hpp"

#include "core/Scheduler.hpp"
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

            context.scheduler.run([] (auto tablet, auto display, auto settings, auto& context) -> coro::task<std::function<Result<void>()>> {
                auto maybeProfile = make_tablet_profile(profileName.data(), tablet, display);
                isCreateButtonDisabled = false;

                if (!maybeProfile.has_value())
                {
                    co_return [result = std::move(maybeProfile)] { return make_error(result.error()); };
                }

                settings.tablet.add_profile(*maybeProfile);
                save_tablet_settings(settings.tablet);

                co_return [&context, settings = std::move(settings)] -> Result<void> {
                    ImGui::PushToast(
                        TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Success)),
                        TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Profile_Created))
                    );
                    context.settings = settings;
                    return {};
                };
            }(context.tablet, context.display, context.settings, context));
        }
    }
    ImGui::EndDisabled();
    ImGui::SetCursorPos(previousCursorPosition);

    return {};
}
