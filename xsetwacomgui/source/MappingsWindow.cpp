#include "MappingsWindow.hpp"

#include "platform/hid/X11/Device.hpp"
#include "ui/Localisation.hpp"
#include "ui/Scaling.hpp"

#include <fplus/fplus.hpp>
#include <imgui/extensions/imgui_toast.hpp>
#include <imgui/imgui.hpp>
#include <liberror/Try.hpp>
#include <magic_enum/magic_enum.hpp>

using namespace liberror;

static std::vector<char const*>& the_action_names()
{
    static auto actions = magic_enum::enum_names<X11Action>();
    static auto actionNames = [] {
        std::vector<char const*> result {};
        std::transform(actions.begin(), actions.end(), std::back_inserter(result), [] (auto& action) {
            return action.data();
        });
        return result;
    }();
    return actionNames;
}

static Result<void> render_stylus_tab(Context& context)
{
    for (auto const& mapping : context.settings.tablet->stylus.mappings)
    {
        ImGui::Text("%s %d", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Mappings_Tabs_Stylus_Button)), mapping.first);
        ImGui::SameLine();

        static std::array<int, 9> actionIndexes {};

        actionIndexes[size_t(mapping.first)] = int(mapping.second)-1;

        ImGui::SetNextItemWidth(180_scaled);
        if (ImGui::Combo(fmt::format("##Actions##Stylus##{}", mapping.first).data(), &actionIndexes[size_t(mapping.first)], the_action_names().data(), int(the_action_names().size())))
        {
            context.settings.tablet->stylus.mappings.at(mapping.first) = *magic_enum::enum_cast<X11Action>(actionIndexes[size_t(mapping.first)]+1);
        }
    }

    return {};
}

static Result<void> render_pad_tab(Context& context)
{
    for (auto const& mapping : context.settings.tablet->pad.mappings)
    {
        ImGui::Text("%s %d", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Mappings_Tabs_Pad_Button)), mapping.first);
        ImGui::SameLine();

        static std::array<int, 9> actionIndexes {};

        actionIndexes[size_t(mapping.first)] = int(mapping.second)-1;

        ImGui::SetNextItemWidth(180_scaled);
        if (ImGui::Combo(fmt::format("##Actions##Pad##{}", mapping.first).data(), &actionIndexes[size_t(mapping.first)], the_action_names().data(), int(the_action_names().size())))
        {
            context.settings.tablet->pad.mappings.at(mapping.first) = *magic_enum::enum_cast<X11Action>(actionIndexes[size_t(mapping.first)]+1);
        }
    }

    return {};
}

Result<void> render_mappings_window(Context& context)
{
    if (ImGui::BeginTabBar("##Tabs_2"))
    {
        if (ImGui::BeginTabItem(TRY(Localisation::get(context.settings.application.language, Localisation::Window_Mappings_Tabs_Stylus_Title))))
        {
            TRY(render_stylus_tab(context));
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(TRY(Localisation::get(context.settings.application.language, Localisation::Window_Mappings_Tabs_Pad_Title))))
        {
            TRY(render_pad_tab(context));
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    auto previousCursorPosition = ImGui::GetCursorPos();
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - (25_scaled + ImGui::GetStyle().WindowPadding.x));
    if (ImGui::Button(TRY(Localisation::get(context.settings.application.language, Localisation::Save_Apply)), { 150_scaled, 25_scaled }))
    {
        ImGui::PushToast(
            TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Success)),
            TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Device_Settings_Saved))
        );
        save_tablet_settings(context.settings.tablet);
        TRY(TabletSettings::Profile::load_to_tablet(context.settings.tablet.get_current_profile(), context.tablet, context.display));
    }
    ImGui::SetCursorPos(previousCursorPosition);

    return {};
}
