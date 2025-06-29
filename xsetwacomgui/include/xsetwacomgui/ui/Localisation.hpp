#pragma once

#include <liberror/Result.hpp>

#include <map>
#include <string>

class Localisation
{
private:
    std::map<std::string, std::map<int, std::string>> data;

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
        Popup_Settings_Tabs_Appearance_FontStyle,
        Popup_Settings_Tabs_Display_Title,
        Popup_Settings_Tabs_Display_Scale,
        Popup_Settings_Tabs_Language_Title,
        Popup_Settings_Tabs_Language_Language,

        Popup_Outdated_Device_Settings_Title,
        Popup_Outdated_Device_Settings_Text,
        Popup_Outdated_Device_Settings_Overwrite,
        Popup_Outdated_Device_Settings_Migrate,

        Tabs_Tablet_Title,
        Tabs_Tablet_Device,
        Tabs_Tablet_Orientation,
        Tabs_Tablet_Orientation_Left,
        Tabs_Tablet_Orientation_Right,
        Tabs_Tablet_PressureCurve,
        Tabs_Tablet_Width,
        Tabs_Tablet_Height,
        Tabs_Tablet_OffsetX,
        Tabs_Tablet_OffsetY,
        Tabs_Tablet_FullArea,
        Tabs_Tablet_ForceProportions,

        Tabs_Display_Title,
        Tabs_Display_Display,
        Tabs_Display_Width,
        Tabs_Display_Height,
        Tabs_Display_OffsetX,
        Tabs_Display_OffsetY,
        Tabs_Display_FullArea,
        Tabs_Display_ForceProportions,

        Toast_Application_Settings_Saved,
        Toast_Device_Settings_Saved,
        Toast_Device_Settings_Overwritten,
        Toast_Device_Settings_Load_Failed,
        Toast_Device_Settings_Load_Success,
        Toast_Device_Settings_Missing,
        Toast_Devices_Missing,
    };

    static auto& the()
    {
        static Localisation localisation;
        return localisation.data;
    }

    static liberror::Result<char const*> get(std::string_view language, LocalisedMessage id);
};

std::vector<std::string> get_available_languages();

