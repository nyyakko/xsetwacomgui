#include <spdlog/spdlog.h>

#include "app/ui/MappingsWindow.hpp"

#include "app/core/Localisation.hpp"
#include "app/core/Scaling.hpp"
#include "platform/hid/X11/Device.hpp"

#include <imgui/extensions/imgui_toast.hpp>
#include <imgui/imgui.hpp>
#include <liberror/Try.hpp>
#include <magic_enum/magic_enum.hpp>
#include <range/v3/view.hpp>

using namespace liberror;

static Result<void> render_stylus_tab(Context& context, TabletSettings& settings)
{
    static auto actionNames =
        magic_enum::enum_names<X11Action>()
            | ranges::views::transform([] (auto& action) { return action.data(); })
            | ranges::to_vector;

    for (auto const& mapping : settings.profile()->second.stylus.mappings)
    {
        ImGui::Text("%s %d", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Mappings_Tabs_Stylus_Button)), mapping.first);
        ImGui::SameLine();

        static std::array<int, 9> actionIndexes {};

        actionIndexes[size_t(mapping.first)-1] = int(mapping.second)-1;

        ImGui::SetNextItemWidth(180_scaled);
        if (ImGui::Combo(fmt::format("##Actions##Stylus##{}", mapping.first).data(), &actionIndexes[size_t(mapping.first)-1], actionNames.data(), int(actionNames.size())))
        {
            settings.profile()->second.stylus.mappings.at(mapping.first) = *magic_enum::enum_cast<X11Action>(actionIndexes[size_t(mapping.first)-1]+1);
        }
    }

    return {};
}

static Result<void> render_pad_tab(Context& context, TabletSettings& settings)
{
    static auto actionNames =
        magic_enum::enum_names<X11Action>()
            | ranges::views::transform([] (auto& action) { return action.data(); })
            | ranges::to_vector;

    for (auto const& mapping : settings.profile()->second.pad.mappings)
    {
        ImGui::Text("%s %d", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Mappings_Tabs_Pad_Button)), mapping.first);
        ImGui::SameLine();

        static std::array<int, 9> actionIndexes {};

        actionIndexes[size_t(mapping.first)-1] = int(mapping.second)-1;

        ImGui::SetNextItemWidth(180_scaled);
        if (ImGui::Combo(fmt::format("##Actions##Pad##{}", mapping.first).data(), &actionIndexes[size_t(mapping.first)-1], actionNames.data(), int(actionNames.size())))
        {
            settings.profile()->second.pad.mappings.at(mapping.first) = *magic_enum::enum_cast<X11Action>(actionIndexes[size_t(mapping.first)-1]+1);
        }
    }

    return {};
}

Result<void> render_mappings_window(bool isWindowVisible, Context& context)
{
    static auto settings = context.settings.tablet;

    if (ImGui::BeginTabBar("##Tabs"))
    {
        if (ImGui::BeginTabItem(TRY(Localisation::get(context.settings.application.language, Localisation::Window_Mappings_Tabs_Stylus_Title))))
        {
            TRY(render_stylus_tab(context, settings));
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(TRY(Localisation::get(context.settings.application.language, Localisation::Window_Mappings_Tabs_Pad_Title))))
        {
            TRY(render_pad_tab(context, settings));
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    auto previousCursorPosition = ImGui::GetCursorPos();
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - (25_scaled + ImGui::GetStyle().WindowPadding.x));
    static auto isSaveApplyButtonDisabled = false;
    ImGui::BeginDisabled(isSaveApplyButtonDisabled);
    if (ImGui::Button(TRY(Localisation::get(context.settings.application.language, Localisation::Save)), { 150_scaled, 25_scaled }))
    {
        isSaveApplyButtonDisabled = true;
        asio::co_spawn(context.stExecutor, [] (Context& context) -> asio::awaitable<void> {
            isSaveApplyButtonDisabled = false;

            context.settings.tablet.profile(settings.profile());

            ImGui::PushToast(
                MUST(Localisation::get(context.settings.application.language, Localisation::Toast_Success)),
                MUST(Localisation::get(context.settings.application.language, Localisation::Toast_Device_Settings_Saved))
            );
            co_return;
        }(context), asio::detached);
        context.stExecutor.restart();
    }
    ImGui::EndDisabled();
    ImGui::SetCursorPos(previousCursorPosition);

    if (!isWindowVisible)
    {
        settings = context.settings.tablet;
    }

    return {};
}
