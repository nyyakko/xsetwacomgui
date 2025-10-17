#include "ProfileWindow.hpp"

#include "ui/Localisation.hpp"
#include "ui/Scaling.hpp"

#include <imgui/extensions/imgui_toast.hpp>
#include <imgui/imgui.hpp>
#include <libcoro/Task.hpp>
#include <liberror/Try.hpp>

using namespace liberror;
using namespace libcoro;
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

            context.tasks.push(Scheduler::the().schedule_with_result([] (Tablet tablet, Display display, Settings& settings) -> Task<std::function<Result<void>()>> {
                auto result = make_tablet_profile(profileName.data(), tablet, display);

                isCreateButtonDisabled = false;

                if (!result.has_value())
                {
                    co_return [result = std::move(result)] { return make_error(result.error()); };
                }

                co_return [&settings, result = std::move(result)] -> Result<void> {
                    ImGui::PushToast(
                        TRY(Localisation::get(settings.application.language, Localisation::Toast_Success)),
                        TRY(Localisation::get(settings.application.language, Localisation::Toast_Profile_Created))
                    );
                    settings.tablet.add_profile(*result);
                    save_tablet_settings(settings.tablet);
                    return {};
                };
            }(context.tablet, context.display, context.settings)));
        }
    }
    ImGui::EndDisabled();
    ImGui::SetCursorPos(previousCursorPosition);

    return {};
}
