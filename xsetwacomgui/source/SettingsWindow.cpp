#include "SettingsWindow.hpp"

#include "ui/Localisation.hpp"
#include "ui/Scaling.hpp"

#include <imgui/extensions/imgui_toast.hpp>
#include <imgui/imgui.hpp>
#include <libcoro/Task.hpp>
#include <liberror/Try.hpp>
#include <range/v3/view.hpp>

#include <algorithm>

using namespace liberror;
using namespace libcoro;

static Result<void> render_appearance_tab(Context& context)
{
    ImGui::Text("%s", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Settings_Tabs_Appearance_Theme)));
    char const* themes[] = {
        TRY(Localisation::get(context.settings.application.language, Localisation::Window_Settings_Tabs_Appearance_Theme_Dark)),
        TRY(Localisation::get(context.settings.application.language, Localisation::Window_Settings_Tabs_Appearance_Theme_Light))
    };
    static auto themeIndex = int(context.settings.application.theme);
    ImGui::SetNextItemWidth(300_scaled + ImGui::GetStyle().WindowPadding.x);
    context.hasChangedTheme = ImGui::Combo("##Theme", &themeIndex, themes, std::size(themes));

    if (context.hasChangedTheme)
    {
        context.settings.application.theme = SettingsApplication::Theme(themeIndex);
    }

    static auto fonts = get_available_fonts();
    static auto fontsInfo = ranges::views::values(fonts) | ranges::to_vector;

    static auto fontsFamily = fontsInfo | ranges::views::transform([] (auto const& fontInfo) { return fontInfo.front().family.data(); }) | ranges::to_vector;
    static auto fontFamilyIndex = int(std::distance(fonts.begin(), fonts.find(context.settings.application.font.family)));

    static auto fontStyles = fontsInfo.at(size_t(fontFamilyIndex)) | ranges::views::transform([] (auto const& fontInfo) { return fontInfo.style.data(); }) | ranges::to_vector;
    static auto fontStyleIndex = int(std::distance(fontStyles.begin(), std::ranges::find(fontStyles, context.settings.application.font.style)));

    ImGui::BeginGroup();
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Settings_Tabs_Appearance_Font)));
        ImGui::SetNextItemWidth(150_scaled);
        context.hasChangedFont = ImGui::Combo("##FontFamily", &fontFamilyIndex, fontsFamily.data(), int(fontsFamily.size()));
    }
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Settings_Tabs_Appearance_FontStyle)));
        ImGui::SetNextItemWidth(150_scaled);
        context.hasChangedFontStyle = ImGui::Combo("##FontStyle", &fontStyleIndex, fontStyles.data(), int(fontStyles.size()));
    }
    ImGui::EndGroup();

    if (context.hasChangedFont || context.hasChangedFontStyle)
    {
        if (context.hasChangedFont)
        {
            fontStyles = fontsInfo.at(size_t(fontFamilyIndex)) | ranges::views::transform([] (auto const& fontInfo) { return fontInfo.style.data(); }) | ranges::to_vector;
            fontStyleIndex = 0;
        }

        context.settings.application.font = fonts.at(fontsFamily.at(size_t(fontFamilyIndex))).at(size_t(fontStyleIndex));
    }

    return {};
}

static Result<void> render_display_tab(Context& context)
{
    ImGui::Text("%s", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Settings_Tabs_Display_Scale)));
    static auto scale = context.settings.application.scale;
    context.hasChangedScale = ImGui::InputFloat("##UiScale", &scale, 0.1f);

    if (context.hasChangedScale)
    {
        scale = std::clamp(scale, 1.0f, 10.f);
        context.settings.application.scale = scale;
    }

    return {};
}

static Result<void> render_languages_tab(Context& context)
{
    ImGui::Text("%s", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Settings_Tabs_Language_Language)));

    static auto languages = get_available_languages();
    static auto languagesData = languages | ranges::views::transform([] (auto const& language) { return language.data(); }) | ranges::to_vector;

    static int languageIndex = int(
        std::distance(languages.begin(), std::ranges::find(languages, context.settings.application.language))
    );

    context.hasChangedLanguage = ImGui::Combo("##Language", &languageIndex, languagesData.data(), int(languages.size()));

    if (context.hasChangedLanguage)
    {
        context.settings.application.language = languages.at(size_t(languageIndex));
    }

    return {};
}

Result<void> render_settings_window(Context& context)
{
    if (ImGui::BeginTabBar("##Tabs"))
    {
        if (ImGui::BeginTabItem(TRY(Localisation::get(context.settings.application.language, Localisation::Window_Settings_Tabs_Appearance_Title))))
        {
            TRY(render_appearance_tab(context));
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(TRY(Localisation::get(context.settings.application.language, Localisation::Window_Settings_Tabs_Display_Title))))
        {
            TRY(render_display_tab(context));
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(TRY(Localisation::get(context.settings.application.language, Localisation::Window_Settings_Tabs_Language_Title))))
        {
            TRY(render_languages_tab(context));
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    auto previousCursorPosition = ImGui::GetCursorPos();
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - (25_scaled + ImGui::GetStyle().WindowPadding.x));
    static auto isSaveButtonDisabled = false;
    ImGui::BeginDisabled(isSaveButtonDisabled);
    if (ImGui::Button(TRY(Localisation::get(context.settings.application.language, Localisation::Save)), { 150_scaled, 25_scaled }))
    {
        isSaveButtonDisabled = true;

        context.tasks.push(Scheduler::the().schedule_with_result([] (Settings settings, Context& context) -> Task<std::function<Result<void>()>> {
            isSaveButtonDisabled = false;
            save_application_settings(settings.application);
            co_return [&context] -> Result<void> {
                ImGui::PushToast(
                    TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Success)),
                    TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Application_Settings_Saved))
                );
                return {};
            };
        }(context.settings, context)));
    }
    ImGui::EndDisabled();
    ImGui::SetCursorPos(previousCursorPosition);

    return {};
}
