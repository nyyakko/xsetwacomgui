#include "app/core/Localisation.hpp"

#include "platform/Environment.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <range/v3/view.hpp>

#include <sstream>
#include <fstream>

using namespace liberror;

Result<Localisation> Localisation::create()
{
    namespace fs = std::filesystem;

    Localisation localisation;

    for (auto const& language :
        ranges::subrange { fs::directory_iterator(get_application_languages_path()), fs::directory_iterator {} }
            | ranges::views::filter([] (auto& entry) { return entry.path().extension() == ".json"; })
            | ranges::views::transform([] (auto& entry) { return entry.path().stem().string(); }))
    {
        try
        {
            std::ifstream stream(get_application_languages_path() / fmt::format("{}.json", language));
            std::stringstream content;
            content << stream.rdbuf();

            auto json = nlohmann::json::parse(content.str());

            localisation.data_.insert({
                language.data(), {
                    { Localisation::About, json["about"].get<std::string>() },
                    { Localisation::Author, json["author"].get<std::string>() },
                    { Localisation::Create, json["create"].get<std::string>() },
                    { Localisation::Delete, json["delete"].get<std::string>() },
                    { Localisation::New_Profile, json["newProfile"].get<std::string>() },
                    { Localisation::Save, json["save"].get<std::string>() },
                    { Localisation::Version, json["version"].get<std::string>() },

                    { Localisation::Toast_Success, json["toast"]["title"]["success"].get<std::string>() },
                    { Localisation::Toast_Warning, json["toast"]["title"]["warning"].get<std::string>() },
                    { Localisation::Toast_Error, json["toast"]["title"]["error"].get<std::string>() },

                    { Localisation::Toast_Application_Settings_Saved, json["toast"]["content"]["applicationSettingsSaved"].get<std::string>() },
                    { Localisation::Toast_Device_Mappings_Saved, json["toast"]["content"]["deviceMappingsSaved"].get<std::string>() },
                    { Localisation::Toast_Device_Settings_Load_Failed, json["toast"]["content"]["deviceSettingsLoadFailed"].get<std::string>() },
                    { Localisation::Toast_Device_Settings_Load_Success, json["toast"]["content"]["deviceSettingsLoadSuccess"].get<std::string>() },
                    { Localisation::Toast_Device_Settings_Migration_Failed, json["toast"]["content"]["deviceSettingsMigrationFailed"].get<std::string>() },
                    { Localisation::Toast_Device_Settings_Missing, json["toast"]["content"]["deviceSettingsMissing"].get<std::string>() },
                    { Localisation::Toast_Device_Settings_Outdated_Schema, json["toast"]["content"]["deviceSettingsOutdatedSchema"].get<std::string>() },
                    { Localisation::Toast_Device_Settings_Overwritten, json["toast"]["content"]["deviceSettingsOverwritten"].get<std::string>() },
                    { Localisation::Toast_Device_Settings_Saved, json["toast"]["content"]["deviceSettingsSaved"].get<std::string>() },
                    { Localisation::Toast_Devices_Missing, json["toast"]["content"]["devicesMissing"].get<std::string>() },
                    { Localisation::Toast_Profile_Create_Failed, json["toast"]["content"]["profileCreateFailed"].get<std::string>() },
                    { Localisation::Toast_Profile_Create_Success, json["toast"]["content"]["profileCreateSuccess"].get<std::string>() },
                    { Localisation::Toast_Profile_Delete_Success, json["toast"]["content"]["profileDeleteSuccess"].get<std::string>() },
                    { Localisation::Toast_Profile_Load_Failed, json["toast"]["content"]["profileLoadFailed"].get<std::string>() },
                    { Localisation::Toast_Profile_Missing, json["toast"]["content"]["profileMissing"].get<std::string>() },
                    { Localisation::Toast_Profile_Name_Empty, json["toast"]["content"]["profileNameEmpty"].get<std::string>() },
                    { Localisation::Toast_Profile_Update_Success, json["toast"]["content"]["profileUpdateSuccess"].get<std::string>() },

                    { Localisation::Popup_Outdated_Device_Settings_Migrate, json["popup"]["outdatedDeviceSettings"]["migrate"].get<std::string>() },
                    { Localisation::Popup_Outdated_Device_Settings_Overwrite, json["popup"]["outdatedDeviceSettings"]["overwrite"].get<std::string>() },
                    { Localisation::Popup_Outdated_Device_Settings_Text, json["popup"]["outdatedDeviceSettings"]["text"].get<std::string>() },
                    { Localisation::Popup_Outdated_Device_Settings_Title, json["popup"]["outdatedDeviceSettings"]["title"].get<std::string>() },

                    { Localisation::Window_Main_MenuBar_Other_About, json["window"]["about"]["title"].get<std::string>() },
                    { Localisation::Window_Main_MenuBar_Other, json["window"]["main"]["menubar"]["other"]["title"].get<std::string>() },
                    { Localisation::Window_Main_MenuBar_Settings_Application, json["window"]["settings"]["title"].get<std::string>() },
                    { Localisation::Window_Main_MenuBar_Settings, json["window"]["main"]["menubar"]["settings"]["title"].get<std::string>() },
                    { Localisation::Window_Main_Tabs_Display_Display, json["window"]["main"]["tabs"]["display"]["display"].get<std::string>() },
                    { Localisation::Window_Main_Tabs_Display_ForceProportions, json["window"]["main"]["tabs"]["display"]["forceProportions"].get<std::string>() },
                    { Localisation::Window_Main_Tabs_Display_FullArea, json["window"]["main"]["tabs"]["display"]["fullArea"].get<std::string>() },
                    { Localisation::Window_Main_Tabs_Display_Height, json["window"]["main"]["tabs"]["display"]["height"].get<std::string>() },
                    { Localisation::Window_Main_Tabs_Display_OffsetX, json["window"]["main"]["tabs"]["display"]["offsetX"].get<std::string>() },
                    { Localisation::Window_Main_Tabs_Display_OffsetY, json["window"]["main"]["tabs"]["display"]["offsetY"].get<std::string>() },
                    { Localisation::Window_Main_Tabs_Display_Title, json["window"]["main"]["tabs"]["display"]["title"].get<std::string>() },
                    { Localisation::Window_Main_Tabs_Display_Width, json["window"]["main"]["tabs"]["display"]["width"].get<std::string>() },
                    { Localisation::Window_Main_Tabs_Tablet_Device, json["window"]["main"]["tabs"]["tablet"]["device"].get<std::string>() },
                    { Localisation::Window_Main_Tabs_Tablet_ForceProportions, json["window"]["main"]["tabs"]["tablet"]["forceProportions"].get<std::string>() },
                    { Localisation::Window_Main_Tabs_Tablet_FullArea, json["window"]["main"]["tabs"]["tablet"]["fullArea"].get<std::string>() },
                    { Localisation::Window_Main_Tabs_Tablet_Height, json["window"]["main"]["tabs"]["tablet"]["height"].get<std::string>() },
                    { Localisation::Window_Main_Tabs_Tablet_OffsetX, json["window"]["main"]["tabs"]["tablet"]["offsetX"].get<std::string>() },
                    { Localisation::Window_Main_Tabs_Tablet_OffsetY, json["window"]["main"]["tabs"]["tablet"]["offsetY"].get<std::string>() },
                    { Localisation::Window_Main_Tabs_Tablet_Orientation, json["window"]["main"]["tabs"]["tablet"]["orientation"].get<std::string>() },
                    { Localisation::Window_Main_Tabs_Tablet_Orientation_Left, json["window"]["main"]["tabs"]["tablet"]["orientationLeft"].get<std::string>() },
                    { Localisation::Window_Main_Tabs_Tablet_Orientation_Right, json["window"]["main"]["tabs"]["tablet"]["orientationRight"].get<std::string>() },
                    { Localisation::Window_Main_Tabs_Tablet_PressureCurve, json["window"]["main"]["tabs"]["tablet"]["pressureCurve"].get<std::string>() },
                    { Localisation::Window_Main_Tabs_Tablet_Title, json["window"]["main"]["tabs"]["tablet"]["title"].get<std::string>() },
                    { Localisation::Window_Main_Tabs_Tablet_Width, json["window"]["main"]["tabs"]["tablet"]["width"].get<std::string>() },

                    { Localisation::Window_Mappings_Tabs_Pad_Button, json["window"]["mappings"]["tabs"]["pad"]["button"].get<std::string>() },
                    { Localisation::Window_Mappings_Tabs_Pad_Title, json["window"]["mappings"]["tabs"]["pad"]["title"].get<std::string>() },
                    { Localisation::Window_Mappings_Tabs_Stylus_Button, json["window"]["mappings"]["tabs"]["stylus"]["button"].get<std::string>() },
                    { Localisation::Window_Mappings_Tabs_Stylus_Title, json["window"]["mappings"]["tabs"]["stylus"]["title"].get<std::string>() },
                    { Localisation::Window_Mappings_Title, json["window"]["mappings"]["title"].get<std::string>() },
                    { Localisation::Window_Settings_Tabs_Appearance_Font, json["window"]["settings"]["tabs"]["appearance"]["font"].get<std::string>() },
                    { Localisation::Window_Settings_Tabs_Appearance_FontStyle, json["window"]["settings"]["tabs"]["appearance"]["fontStyle"].get<std::string>() },
                    { Localisation::Window_Settings_Tabs_Appearance_Theme_Dark, json["window"]["settings"]["tabs"]["appearance"]["themeDark"].get<std::string>() },
                    { Localisation::Window_Settings_Tabs_Appearance_Theme, json["window"]["settings"]["tabs"]["appearance"]["theme"].get<std::string>() },
                    { Localisation::Window_Settings_Tabs_Appearance_Theme_Light, json["window"]["settings"]["tabs"]["appearance"]["themeLight"].get<std::string>() },
                    { Localisation::Window_Settings_Tabs_Appearance_Title, json["window"]["settings"]["tabs"]["appearance"]["title"].get<std::string>() },
                    { Localisation::Window_Settings_Tabs_Display_Scale, json["window"]["settings"]["tabs"]["display"]["scale"].get<std::string>() },
                    { Localisation::Window_Settings_Tabs_Display_Title, json["window"]["settings"]["tabs"]["display"]["title"].get<std::string>() },
                    { Localisation::Window_Settings_Tabs_Language_Language, json["window"]["settings"]["tabs"]["language"]["language"].get<std::string>() },
                    { Localisation::Window_Settings_Tabs_Language_Title, json["window"]["settings"]["tabs"]["language"]["title"].get<std::string>() },
                    { Localisation::Window_Settings_Title, json["window"]["settings"]["title"].get<std::string>() },

                    { Localisation::Window_Profile_Tab_Name, json["window"]["profile"]["tab"]["name"].get<std::string>() },
                    { Localisation::Window_Profile_Title, json["window"]["profile"]["title"].get<std::string>() },
                }
            });
        }
        catch (std::exception const& error)
        {
            return make_error("{}", error.what());
        }
    }

    return localisation;
}
