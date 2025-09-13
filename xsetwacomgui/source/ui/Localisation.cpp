#include "ui/Localisation.hpp"

#include "platform/Environment.hpp"

#include <fmt/format.h>
#include <fplus/fplus.hpp>
#include <nlohmann/json.hpp>

#include <sstream>
#include <fstream>
#include <ranges>

using namespace liberror;

Result<char const*> Localisation::get(std::string_view language, Message id)
{
    if (!the().contains(language.data()))
    {
        auto languageLowercase = std::string_view(language.data()) | std::views::transform(tolower);
        std::ifstream stream(
            get_application_data_path() / "languages" / fmt::format("{}.json", std::string(languageLowercase.begin(), languageLowercase.end()))
        );
        std::stringstream content;
        content << stream.rdbuf();

        try
        {
            auto json = nlohmann::json::parse(content.str());
            the()[language.data()] = {
                { Localisation::Save, json["save"].get<std::string>() },
                { Localisation::Save_Apply, json["saveApply"].get<std::string>() },

                { Localisation::Toast_Success, json["toast"]["success"].get<std::string>() },
                { Localisation::Toast_Warning, json["toast"]["warning"].get<std::string>() },
                { Localisation::Toast_Error, json["toast"]["error"].get<std::string>() },
                { Localisation::Toast_Devices_Missing, json["toast"]["devicesMissing"].get<std::string>() },
                { Localisation::Toast_Application_Settings_Saved, json["toast"]["applicationSettingsSaved"].get<std::string>() },
                { Localisation::Toast_Device_Settings_Saved, json["toast"]["deviceSettingsSaved"].get<std::string>() },
                { Localisation::Toast_Device_Settings_Overwritten, json["toast"]["deviceSettingsOverwritten"].get<std::string>() },
                { Localisation::Toast_Device_Settings_Load_Failed, json["toast"]["deviceSettingsLoadFailed"].get<std::string>() },
                { Localisation::Toast_Device_Settings_Load_Success, json["toast"]["deviceSettingsLoadSuccess"].get<std::string>() },
                { Localisation::Toast_Device_Settings_Missing, json["toast"]["deviceSettingsMissing"].get<std::string>() },
                { Localisation::Toast_Device_Settings_Outdated_Schema, json["toast"]["deviceSettingsOutdatedSchema"].get<std::string>() },

                { Localisation::MenuBar_Settings_Title, json["menubar"]["settings"]["title"].get<std::string>() },
                { Localisation::MenuBar_Settings_Entry_Application, json["window"]["settings"]["title"].get<std::string>() },

                { Localisation::MenuBar_Other_Title, json["menubar"]["other"]["title"].get<std::string>() },
                { Localisation::MenuBar_Other_Entry_Goddess, json["window"]["goddess"]["title"].get<std::string>() },

                { Localisation::Popup_Outdated_Device_Settings_Title, json["popup"]["outdatedDeviceSettings"]["title"].get<std::string>() },
                { Localisation::Popup_Outdated_Device_Settings_Text, json["popup"]["outdatedDeviceSettings"]["text"].get<std::string>() },
                { Localisation::Popup_Outdated_Device_Settings_Overwrite, json["popup"]["outdatedDeviceSettings"]["overwrite"].get<std::string>() },
                { Localisation::Popup_Outdated_Device_Settings_Migrate, json["popup"]["outdatedDeviceSettings"]["migrate"].get<std::string>() },

                { Localisation::Window_Main_Tabs_Tablet_Title, json["window"]["main"]["tabs"]["tablet"]["title"].get<std::string>() },
                { Localisation::Window_Main_Tabs_Tablet_Device, json["window"]["main"]["tabs"]["tablet"]["device"].get<std::string>() },
                { Localisation::Window_Main_Tabs_Tablet_Orientation, json["window"]["main"]["tabs"]["tablet"]["orientation"].get<std::string>() },
                { Localisation::Window_Main_Tabs_Tablet_Orientation_Left, json["window"]["main"]["tabs"]["tablet"]["orientationLeft"].get<std::string>() },
                { Localisation::Window_Main_Tabs_Tablet_Orientation_Right, json["window"]["main"]["tabs"]["tablet"]["orientationRight"].get<std::string>() },
                { Localisation::Window_Main_Tabs_Tablet_PressureCurve, json["window"]["main"]["tabs"]["tablet"]["pressureCurve"].get<std::string>() },
                { Localisation::Window_Main_Tabs_Tablet_Width, json["window"]["main"]["tabs"]["tablet"]["width"].get<std::string>() },
                { Localisation::Window_Main_Tabs_Tablet_Height, json["window"]["main"]["tabs"]["tablet"]["height"].get<std::string>() },
                { Localisation::Window_Main_Tabs_Tablet_OffsetX, json["window"]["main"]["tabs"]["tablet"]["offsetX"].get<std::string>() },
                { Localisation::Window_Main_Tabs_Tablet_OffsetY, json["window"]["main"]["tabs"]["tablet"]["offsetY"].get<std::string>() },
                { Localisation::Window_Main_Tabs_Tablet_FullArea, json["window"]["main"]["tabs"]["tablet"]["fullArea"].get<std::string>() },
                { Localisation::Window_Main_Tabs_Tablet_ForceProportions, json["window"]["main"]["tabs"]["tablet"]["forceProportions"].get<std::string>() },
                { Localisation::Window_Main_Tabs_Display_Title, json["window"]["main"]["tabs"]["display"]["title"].get<std::string>() },
                { Localisation::Window_Main_Tabs_Display_Display, json["window"]["main"]["tabs"]["display"]["display"].get<std::string>() },
                { Localisation::Window_Main_Tabs_Display_Width, json["window"]["main"]["tabs"]["display"]["width"].get<std::string>() },
                { Localisation::Window_Main_Tabs_Display_Height, json["window"]["main"]["tabs"]["display"]["height"].get<std::string>() },
                { Localisation::Window_Main_Tabs_Display_OffsetX, json["window"]["main"]["tabs"]["display"]["offsetX"].get<std::string>() },
                { Localisation::Window_Main_Tabs_Display_OffsetY, json["window"]["main"]["tabs"]["display"]["offsetY"].get<std::string>() },
                { Localisation::Window_Main_Tabs_Display_FullArea, json["window"]["main"]["tabs"]["display"]["fullArea"].get<std::string>() },
                { Localisation::Window_Main_Tabs_Display_ForceProportions, json["window"]["main"]["tabs"]["display"]["forceProportions"].get<std::string>() },

                { Localisation::Window_Settings_Title, json["window"]["settings"]["title"].get<std::string>() },
                { Localisation::Window_Settings_Tabs_Appearance_Title, json["window"]["settings"]["tabs"]["appearance"]["title"].get<std::string>() },
                { Localisation::Window_Settings_Tabs_Appearance_Theme, json["window"]["settings"]["tabs"]["appearance"]["theme"].get<std::string>() },
                { Localisation::Window_Settings_Tabs_Appearance_Theme_Dark, json["window"]["settings"]["tabs"]["appearance"]["themeDark"].get<std::string>() },
                { Localisation::Window_Settings_Tabs_Appearance_Theme_Light, json["window"]["settings"]["tabs"]["appearance"]["themeLight"].get<std::string>() },
                { Localisation::Window_Settings_Tabs_Appearance_Font, json["window"]["settings"]["tabs"]["appearance"]["font"].get<std::string>() },
                { Localisation::Window_Settings_Tabs_Appearance_FontStyle, json["window"]["settings"]["tabs"]["appearance"]["fontStyle"].get<std::string>() },
                { Localisation::Window_Settings_Tabs_Display_Title, json["window"]["settings"]["tabs"]["display"]["title"].get<std::string>() },
                { Localisation::Window_Settings_Tabs_Display_Scale, json["window"]["settings"]["tabs"]["display"]["scale"].get<std::string>() },
                { Localisation::Window_Settings_Tabs_Language_Title, json["window"]["settings"]["tabs"]["language"]["title"].get<std::string>() },
                { Localisation::Window_Settings_Tabs_Language_Language, json["window"]["settings"]["tabs"]["language"]["language"].get<std::string>() },

                { Localisation::Window_Mappings_Title, json["window"]["mappings"]["title"].get<std::string>() },
                { Localisation::Window_Mappings_Tabs_Stylus_Title, json["window"]["mappings"]["tabs"]["stylus"]["title"].get<std::string>() },
                { Localisation::Window_Mappings_Tabs_Stylus_Button, json["window"]["mappings"]["tabs"]["stylus"]["button"].get<std::string>() },
                { Localisation::Window_Mappings_Tabs_Pad_Title, json["window"]["mappings"]["tabs"]["pad"]["title"].get<std::string>() },
                { Localisation::Window_Mappings_Tabs_Pad_Button, json["window"]["mappings"]["tabs"]["pad"]["button"].get<std::string>() },
            };
        }
        catch (std::exception const& error)
        {
            return make_error("{}", error.what());
        }
    }

    return the()[language.data()][id].data();
}

std::vector<std::string> get_available_languages()
{
    std::vector<std::string> languages {};

    for (auto const& entry : std::filesystem::directory_iterator(get_application_data_path() / "languages"))
    {
        if (entry.path().extension() == ".json")
            languages.push_back(entry.path().stem().string());
    }

    return languages;
}
