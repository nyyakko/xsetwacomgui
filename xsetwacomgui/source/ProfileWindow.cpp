#include "ProfileWindow.hpp"

#include "settings/TabletSettings.hpp"
#include "ui/Localisation.hpp"
#include "ui/Scaling.hpp"

#include <liberror/Try.hpp>
#include <imgui/imgui.hpp>
#include <imgui/extensions/imgui_toast.hpp>

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
            ImGui::PushToast(
                TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Success)),
                TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Profile_Created))
            );
            context.settings.tablet.profiles.emplace(profileName.data(), TRY(TabletSettings::Profile::make_default(context.tablet, context.display)));
            save_tablet_settings(context.settings.tablet);
        }
    }
    ImGui::SetCursorPos(previousCursorPosition);

    return {};
}

// cppcheck-suppress [constParameterReference]
Result<void> render_profile_window(Context& context, size_t profileId)
{
    (void)context; (void)profileId;
    assert(false && "UNIMPLEMENTED");
    return {};
}
