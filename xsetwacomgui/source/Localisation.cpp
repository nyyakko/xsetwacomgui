#include "Localisation.hpp"

#include "Environment.hpp"

#include <fmt/format.h>
#include <fplus/fplus.hpp>
#include <nlohmann/json.hpp>

#include <sstream>
#include <fstream>

liberror::Result<char const*> Localisation::get(ApplicationSettings::Language language, LocalisedMessage id)
{
    if (!the().contains(language))
    {
        auto languageLowercase = std::string_view(language.to_string()) | std::views::transform(tolower);
        std::ifstream stream(
            get_application_data_path() / "languages" / fmt::format("{}.json", std::string(languageLowercase.begin(), languageLowercase.end()))
        );
        std::stringstream content;
        content << stream.rdbuf();

        try
        {
            auto json = nlohmann::json::parse(content.str());
            the()[language] = {
                { Localisation::Toast_Success, json["toastSuccess"].get<std::string>() },
                { Localisation::Toast_Warning, json["toastWarning"].get<std::string>() },
                { Localisation::Toast_Error, json["toastError"].get<std::string>() },
                { Localisation::Save, json["save"].get<std::string>() },
                { Localisation::Save_Apply, json["saveApply"].get<std::string>() },
                { Localisation::MenuBar_Settings, json["menuBarSettings"].get<std::string>() },
                { Localisation::MenuBar_Settings_Application, json["menuBarSettingsApplication"].get<std::string>() },
                { Localisation::MenuBar_Other, json["menuBarOther"].get<std::string>() },
                { Localisation::MenuBar_Other_Goddess, json["menuBarOtherGoddess"].get<std::string>() },
                { Localisation::Popup_Settings_Tabs_Appearance_Title, json["popupSettingsTabsAppearanceTitle"].get<std::string>() },
                { Localisation::Popup_Settings_Tabs_Appearance_Theme, json["popupSettingsTabsAppearanceTheme"].get<std::string>() },
                { Localisation::Popup_Settings_Tabs_Appearance_Theme_Dark, json["popupSettingsTabsAppearanceThemeDark"].get<std::string>() },
                { Localisation::Popup_Settings_Tabs_Appearance_Theme_Light, json["popupSettingsTabsAppearanceThemeLight"].get<std::string>() },
                { Localisation::Popup_Settings_Tabs_Display_Title, json["popupSettingsTabsDisplayTitle"].get<std::string>() },
                { Localisation::Popup_Settings_Tabs_Display_Scale, json["popupSettingsTabsDisplayScale"].get<std::string>() },
                { Localisation::Popup_Settings_Tabs_Language_Title, json["popupSettingsTabsLanguageTitle"].get<std::string>() },
                { Localisation::Popup_Settings_Tabs_Language_Language, json["popupSettingsTabsLanguageLanguage"].get<std::string>() },
                { Localisation::Popup_Settings_Tabs_Appearance_Font, json["popupSettingsTabsAppearanceFont"].get<std::string>() },
                { Localisation::Tabs_Tablet_Title, json["tabsTabletTitle"].get<std::string>() },
                { Localisation::Tabs_Tablet_Device, json["tabsTabletDevice"].get<std::string>() },
                { Localisation::Tabs_Tablet_PressureCurve, json["tabsTabletPressureCurve"].get<std::string>() },
                { Localisation::Tabs_Tablet_Width, json["tabsTabletWidth"].get<std::string>() },
                { Localisation::Tabs_Tablet_Height, json["tabsTabletHeight"].get<std::string>() },
                { Localisation::Tabs_Tablet_OffsetX, json["tabsTabletOffsetX"].get<std::string>() },
                { Localisation::Tabs_Tablet_OffsetY, json["tabsTabletOffsetY"].get<std::string>() },
                { Localisation::Tabs_Tablet_FullArea, json["tabsTabletFullArea"].get<std::string>() },
                { Localisation::Tabs_Tablet_ForceProportions, json["tabsTabletForceProportions"].get<std::string>() },
                { Localisation::Tabs_Monitor_Title, json["tabsMonitorTitle"].get<std::string>() },
                { Localisation::Tabs_Monitor_Monitor, json["tabsMonitorMonitor"].get<std::string>() },
                { Localisation::Tabs_Monitor_Width, json["tabsMonitorWidth"].get<std::string>() },
                { Localisation::Tabs_Monitor_Height, json["tabsMonitorHeight"].get<std::string>() },
                { Localisation::Tabs_Monitor_OffsetX, json["tabsMonitorOffsetX"].get<std::string>() },
                { Localisation::Tabs_Monitor_OffsetY, json["tabsMonitorOffsetY"].get<std::string>() },
                { Localisation::Tabs_Monitor_FullArea, json["tabsMonitorFullArea"].get<std::string>() },
                { Localisation::Tabs_Monitor_ForceProportions, json["tabsMonitorForceProportions"].get<std::string>() },
                { Localisation::Toast_Devices_Missing, json["toastDevicesMissing"].get<std::string>() },
                { Localisation::Toast_Application_Settings_Saved, json["toastApplicationSettingsSaved"].get<std::string>() },
                { Localisation::Toast_Device_Settings_Saved, json["toastDeviceSettingsSaved"].get<std::string>() },
                { Localisation::Toast_Device_Settings_Load_Failed, json["toastDeviceSettingsLoadFailed"].get<std::string>() },
                { Localisation::Toast_Device_Settings_Missing, json["toastDeviceSettingsMissing"].get<std::string>() },
            };
        }
        catch (std::exception const& error)
        {
            return liberror::make_error("{}", error.what());
        }
    }

    return the()[language][id].data();
}
