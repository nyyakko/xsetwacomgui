#pragma once

#include <liberror/Result.hpp>
#include <liberror/Try.hpp>
#include <range/v3/view.hpp>

#include <map>
#include <string>

class Localisation
{
    enum class Message
    {
        Create,
        Delete,
        New_Profile,
        Save,

        Toast_Success,
        Toast_Warning,
        Toast_Error,

        Toast_Application_Settings_Saved,
        Toast_Device_Mappings_Saved,
        Toast_Device_Settings_Load_Failed,
        Toast_Device_Settings_Load_Success,
        Toast_Device_Settings_Migration_Failed,
        Toast_Device_Settings_Missing,
        Toast_Device_Settings_Outdated_Schema,
        Toast_Device_Settings_Overwritten,
        Toast_Device_Settings_Saved,
        Toast_Devices_Missing,
        Toast_Profile_Create_Failed,
        Toast_Profile_Create_Success,
        Toast_Profile_Delete_Success,
        Toast_Profile_Load_Failed,
        Toast_Profile_Missing,
        Toast_Profile_Name_Empty,
        Toast_Profile_Update_Success,

        Popup_Outdated_Device_Settings_Migrate,
        Popup_Outdated_Device_Settings_Overwrite,
        Popup_Outdated_Device_Settings_Text,
        Popup_Outdated_Device_Settings_Title,

        Window_Main_MenuBar_Other,
        Window_Main_MenuBar_Other_Goddess,
        Window_Main_MenuBar_Settings,
        Window_Main_MenuBar_Settings_Application,
        Window_Main_Tabs_Display_Display,
        Window_Main_Tabs_Display_ForceProportions,
        Window_Main_Tabs_Display_FullArea,
        Window_Main_Tabs_Display_Height,
        Window_Main_Tabs_Display_OffsetX,
        Window_Main_Tabs_Display_OffsetY,
        Window_Main_Tabs_Display_Title,
        Window_Main_Tabs_Display_Width,
        Window_Main_Tabs_Tablet_Device,
        Window_Main_Tabs_Tablet_ForceProportions,
        Window_Main_Tabs_Tablet_FullArea,
        Window_Main_Tabs_Tablet_Height,
        Window_Main_Tabs_Tablet_OffsetX,
        Window_Main_Tabs_Tablet_OffsetY,
        Window_Main_Tabs_Tablet_Orientation,
        Window_Main_Tabs_Tablet_Orientation_Left,
        Window_Main_Tabs_Tablet_Orientation_Right,
        Window_Main_Tabs_Tablet_PressureCurve,
        Window_Main_Tabs_Tablet_Title,
        Window_Main_Tabs_Tablet_Width,

        Window_Settings_Tabs_Appearance_Font,
        Window_Settings_Tabs_Appearance_FontStyle,
        Window_Settings_Tabs_Appearance_Theme,
        Window_Settings_Tabs_Appearance_Theme_Dark,
        Window_Settings_Tabs_Appearance_Theme_Light,
        Window_Settings_Tabs_Appearance_Title,
        Window_Settings_Tabs_Display_Scale,
        Window_Settings_Tabs_Display_Title,
        Window_Settings_Tabs_Language_Language,
        Window_Settings_Tabs_Language_Title,
        Window_Settings_Title,

        Window_Mappings_Tabs_Pad_Button,
        Window_Mappings_Tabs_Pad_Title,
        Window_Mappings_Tabs_Stylus_Button,
        Window_Mappings_Tabs_Stylus_Title,
        Window_Mappings_Title,

        Window_Profile_Tab_Name,
        Window_Profile_Title,
    };

public:
    using enum Message;

    Localisation()
        : data_{}
    {}

    Localisation(Localisation&& that)
        : data_{std::move(that.data_)}
    {}

    Localisation& operator=(Localisation&& that)
    {
        this->data_ = std::move(that.data_);
        return *this;
    }

    Localisation(Localisation const&) = delete;
    Localisation& operator=(Localisation const&) = delete;

public:
    static auto& the()
    {
        static auto localisation = MUST(Localisation::create());
        return localisation;
    }

    auto languages() const { return data_ | ranges::views::keys | ranges::to_vector; }

    auto const& translations() const { return data_; }

    static liberror::Result<char const*> get(std::string_view language, auto message)
    {
        return the().translations().at(language.data()).at(message).data();
    }

private:
    static liberror::Result<Localisation> create();

    std::map<std::string, std::map<Message, std::string>> data_;
};
