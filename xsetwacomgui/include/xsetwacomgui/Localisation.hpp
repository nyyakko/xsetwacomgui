#pragma once

#include "Settings.hpp"

#include <liberror/Result.hpp>

#include <map>
#include <string>

class Localisation
{
private:
    std::map<ApplicationSettings::Language, std::map<int, std::string>> data;

    using LocalisedMessage = int;
public:
    enum
    {
        Toast_Success,
        Toast_Warning,
        Toast_Error,

        Save,
        Save_Apply,

        MenuBar_Settings,
        MenuBar_Settings_Application,
        MenuBar_Other,
        MenuBar_Other_Goddess,

        Popup_Settings_Tabs_Appearance_Title,
        Popup_Settings_Tabs_Appearance_Theme,
        Popup_Settings_Tabs_Appearance_Theme_Dark,
        Popup_Settings_Tabs_Appearance_Theme_Light,
        Popup_Settings_Tabs_Appearance_Font,
        Popup_Settings_Tabs_Display_Title,
        Popup_Settings_Tabs_Display_Scale,
        Popup_Settings_Tabs_Language_Title,
        Popup_Settings_Tabs_Language_Language,

        Tabs_Tablet_Title,
        Tabs_Tablet_Device,
        Tabs_Tablet_PressureCurve,
        Tabs_Tablet_Width,
        Tabs_Tablet_Height,
        Tabs_Tablet_OffsetX,
        Tabs_Tablet_OffsetY,
        Tabs_Tablet_FullArea,
        Tabs_Tablet_ForceProportions,
        Toast_Devices_Missing,

        Tabs_Monitor_Title,
        Tabs_Monitor_Monitor,
        Tabs_Monitor_Width,
        Tabs_Monitor_Height,
        Tabs_Monitor_OffsetX,
        Tabs_Monitor_OffsetY,
        Tabs_Monitor_FullArea,
        Tabs_Monitor_ForceProportions,

        Toast_Application_Settings_Saved,
        Toast_Device_Settings_Saved,
        Toast_Device_Settings_Load_Failed,
        Toast_Device_Settings_Missing,
    };

    static auto& the()
    {
        static Localisation localisation;
        return localisation.data;
    }

    static liberror::Result<char const*> get(ApplicationSettings::Language language, LocalisedMessage id);
};

