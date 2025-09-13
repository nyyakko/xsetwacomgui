#include "SettingsWindow.hpp"

#include "ui/Localisation.hpp"
#include "ui/Scaling.hpp"

#include <fplus/fplus.hpp>
#include <imgui/extensions/imgui_toast.hpp>
#include <liberror/Try.hpp>

#include <algorithm>

using namespace liberror;

static Result<void> render_appearance_tab(Context& context)
{
    ImGui::Text("%s", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Settings_Tabs_Appearance_Theme)));
    char const* themes[] = {
        TRY(Localisation::get(context.settings.application.language, Localisation::Window_Settings_Tabs_Appearance_Theme_Dark)),
        TRY(Localisation::get(context.settings.application.language, Localisation::Window_Settings_Tabs_Appearance_Theme_Light))
    };
    static int themeIndex = static_cast<int>(context.settings.application.theme);
    ImGui::SetNextItemWidth(300_scaled + ImGui::GetStyle().WindowPadding.x);
    context.hasChangedTheme = ImGui::Combo("##Theme", &themeIndex, themes, std::size(themes));

    if (context.hasChangedTheme)
    {
        context.settings.application.theme = ApplicationSettings::Theme(themeIndex);
    }

    static auto fonts = get_available_fonts();
    static auto fontsInfo = fplus::get_map_values(fonts);

    static auto fontsFamily = fplus::transform([] (std::vector<FontInfo> const& fontInfo) { return fontInfo.front().family.data(); }, fontsInfo);
    static auto fontFamilyIndex = static_cast<int>(std::distance(fonts.begin(), fonts.find(context.settings.application.font.family)));

    static auto fontStyles = fplus::transform([] (FontInfo const& fontInfo) { return fontInfo.style.data(); }, fontsInfo.at(static_cast<size_t>(fontFamilyIndex)));
    static auto fontStyleIndex = static_cast<int>(std::distance(fontStyles.begin(), std::ranges::find(fontStyles, context.settings.application.font.style)));

    ImGui::BeginGroup();
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Settings_Tabs_Appearance_Font)));
        ImGui::SetNextItemWidth(150_scaled);
        context.hasChangedFont = ImGui::Combo("##FontFamily", &fontFamilyIndex, fontsFamily.data(), static_cast<int>(fontsFamily.size()));
    }
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Settings_Tabs_Appearance_FontStyle)));
        ImGui::SetNextItemWidth(150_scaled);
        context.hasChangedFontStyle = ImGui::Combo("##FontStyle", &fontStyleIndex, fontStyles.data(), static_cast<int>(fontStyles.size()));
    }
    ImGui::EndGroup();

    if (context.hasChangedFont || context.hasChangedFontStyle)
    {
        if (context.hasChangedFont)
        {
            fontStyles = fplus::transform([] (FontInfo const& fontInfo) { return fontInfo.style.data(); }, fontsInfo.at(static_cast<size_t>(fontFamilyIndex)));
            fontStyleIndex = 0;
        }

        context.settings.application.font = fonts.at(fontsFamily.at(static_cast<size_t>(fontFamilyIndex))).at(static_cast<size_t>(fontStyleIndex));
    }

    return {};
}

static Result<void> render_display_tab(Context& context)
{
    ImGui::Text("%s", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Settings_Tabs_Display_Scale)));
    static float scale = context.settings.application.scale;
    context.hasChangedScale = ImGui::InputFloat("##UiScale", &scale, 0.1f);

    if (context.hasChangedScale)
    {
        scale = ImClamp(scale, 1.0f, 10.f);
        context.settings.application.scale = scale;
    }

    return {};
}

static Result<void> render_languages_tab(Context& context)
{
    ImGui::Text("%s", TRY(Localisation::get(context.settings.application.language, Localisation::Window_Settings_Tabs_Language_Language)));

    static auto languages = get_available_languages();
    static auto languagesData = fplus::transform([] (std::string const& language) { return language.data(); }, languages);

    static int languageIndex = static_cast<int>(
        std::distance(languages.begin(), std::ranges::find(languages, context.settings.application.language))
    );

    context.hasChangedLanguage = ImGui::Combo("##Language", &languageIndex, languagesData.data(), static_cast<int>(languages.size()));

    if (context.hasChangedLanguage)
    {
        context.settings.application.language = languages.at(static_cast<size_t>(languageIndex));
    }

    return {};
}

Result<void> render_settings_window(Context& context)
{
    if (ImGui::BeginTabBar("##Tabs_2"))
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
    if (ImGui::Button(TRY(Localisation::get(context.settings.application.language, Localisation::Save)), { 150_scaled, 25_scaled }))
    {
        ImGui::PushToast(
            TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Success)),
            TRY(Localisation::get(context.settings.application.language, Localisation::Toast_Application_Settings_Saved))
        );
        save_application_settings(context.settings.application);
    }
    ImGui::SetCursorPos(previousCursorPosition);

    return {};
}
