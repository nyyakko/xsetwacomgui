#include "app/ui/SettingsWindow.hpp"

#include "app/core/Localisation.hpp"
#include "app/core/Scaling.hpp"
#include "utils/MakeAsync.hpp"

#include <imgui/extensions/imgui_toast.hpp>
#include <imgui/imgui.hpp>
#include <liberror/Try.hpp>
#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>

#include <algorithm>

using namespace liberror;

static Result<void> render_appearance_tab(Context& context)
{
    ImGui::Text("%s", TRY(Localisation::get(context.settings.language(), Localisation::Window_Settings_Tabs_Appearance_Theme)));
    char const* themes[] = {
        TRY(Localisation::get(context.settings.language(), Localisation::Window_Settings_Tabs_Appearance_Theme_Dark)),
        TRY(Localisation::get(context.settings.language(), Localisation::Window_Settings_Tabs_Appearance_Theme_Light))
    };
    static auto themeIndex = int(context.settings.theme());
    ImGui::SetNextItemWidth(300_scaled + ImGui::GetStyle().WindowPadding.x);
    context.hasChangedTheme = ImGui::Combo("##Theme", &themeIndex, themes, std::size(themes));

    if (context.hasChangedTheme)
    {
        context.settings.theme() = ApplicationSettings::Theme(themeIndex);
    }

    static auto fonts = get_available_fonts();
    static auto fontsInfo = ranges::views::values(fonts) | ranges::to_vector;

    static auto fontsFamily = fontsInfo | ranges::views::transform([] (auto const& fontInfo) { return fontInfo.front().family.data(); }) | ranges::to_vector;
    static auto fontFamilyIndex = int(std::distance(fonts.begin(), fonts.find(context.settings.font().family)));

    static auto fontStyles = fontsInfo.at(size_t(fontFamilyIndex)) | ranges::views::transform([] (auto const& fontInfo) { return fontInfo.style.data(); }) | ranges::to_vector;
    static auto fontStyleIndex = int(std::distance(fontStyles.begin(), ranges::find(fontStyles, context.settings.font().style)));

    ImGui::BeginGroup();
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", TRY(Localisation::get(context.settings.language(), Localisation::Window_Settings_Tabs_Appearance_Font)));
        ImGui::SetNextItemWidth(150_scaled);
        context.hasChangedFont = ImGui::Combo("##FontFamily", &fontFamilyIndex, fontsFamily.data(), int(fontsFamily.size()));
    }
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", TRY(Localisation::get(context.settings.language(), Localisation::Window_Settings_Tabs_Appearance_FontStyle)));
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

        context.settings.font() = fonts.at(fontsFamily.at(size_t(fontFamilyIndex))).at(size_t(fontStyleIndex));
    }

    return {};
}

static Result<void> render_display_tab(Context& context)
{
    ImGui::Text("%s", TRY(Localisation::get(context.settings.language(), Localisation::Window_Settings_Tabs_Display_Scale)));
    static auto scale = context.settings.scale();
    context.hasChangedScale = ImGui::InputFloat("##UiScale", &scale, 0.1f);

    if (context.hasChangedScale)
    {
        scale = std::clamp(scale, 1.0f, 10.f);
        context.settings.scale() = scale;
    }

    return {};
}

static Result<void> render_languages_tab(Context& context)
{
    ImGui::Text("%s", TRY(Localisation::get(context.settings.language(), Localisation::Window_Settings_Tabs_Language_Language)));

    static auto languages = Localisation::the().languages();
    static auto languagesData = languages | ranges::views::transform([] (auto const& language) { return language.data(); }) | ranges::to_vector;

    static int languageIndex = int(
        std::distance(languages.begin(), ranges::find(languages, context.settings.language()))
    );

    context.hasChangedLanguage = ImGui::Combo("##Language", &languageIndex, languagesData.data(), int(languages.size()));

    if (context.hasChangedLanguage)
    {
        context.settings.language() = languages.at(size_t(languageIndex));
    }

    return {};
}

Result<void> render_settings_window(Context& context)
{
    if (ImGui::BeginTabBar("##Tabs"))
    {
        if (ImGui::BeginTabItem(TRY(Localisation::get(context.settings.language(), Localisation::Window_Settings_Tabs_Appearance_Title))))
        {
            TRY(render_appearance_tab(context));
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(TRY(Localisation::get(context.settings.language(), Localisation::Window_Settings_Tabs_Display_Title))))
        {
            TRY(render_display_tab(context));
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(TRY(Localisation::get(context.settings.language(), Localisation::Window_Settings_Tabs_Language_Title))))
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
    if (ImGui::Button(TRY(Localisation::get(context.settings.language(), Localisation::Save)), { 150_scaled, 25_scaled }))
    {
        isSaveButtonDisabled = true;
        asio::co_spawn(context.stExecutor, [] (Context& context_) -> asio::awaitable<void> {
            co_await asio::co_spawn(context_.mtExecutor, make_async<save_application_settings>(auto(context_.settings)));
            isSaveButtonDisabled = false;
            ImGui::PushToast(
                MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Success)),
                MUST(Localisation::get(context_.settings.language(), Localisation::Toast_Application_Settings_Saved))
            );
        }(context), asio::detached);
    }
    ImGui::EndDisabled();
    ImGui::SetCursorPos(previousCursorPosition);

    return {};
}
