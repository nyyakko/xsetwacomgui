#pragma once

#include <map>
#include <string>

class Localisation
{
private:
    static std::map<std::string, std::map<int, std::string>> data;

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
        Popup_Settings_Tabs_Appearance_Scale,
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

    static char const* get(std::string_view language, auto id)
    {
        return the()[language.data()][id].data();
    }
};

inline std::map<std::string, std::map<int, std::string>> Localisation::data = {
    {
        "en_us", {
            { Localisation::Toast_Success, "Success" },
            { Localisation::Toast_Warning, "Warning" },
            { Localisation::Toast_Error, "Error" },
            { Localisation::Save, "Save" },
            { Localisation::Save_Apply, "Save & Apply" },
            { Localisation::MenuBar_Settings, "Settings" },
            { Localisation::MenuBar_Settings_Application, "Application Settings..." },
            { Localisation::MenuBar_Other, "Other" },
            { Localisation::MenuBar_Other_Goddess, "Goddess" },
            { Localisation::Popup_Settings_Tabs_Appearance_Title, "Appearance" },
            { Localisation::Popup_Settings_Tabs_Appearance_Theme, "Theme" },
            { Localisation::Popup_Settings_Tabs_Appearance_Theme_Dark, "Dark" },
            { Localisation::Popup_Settings_Tabs_Appearance_Theme_Light, "Light" },
            { Localisation::Popup_Settings_Tabs_Appearance_Scale, "Scale" },
            { Localisation::Popup_Settings_Tabs_Language_Title, "Language" },
            { Localisation::Popup_Settings_Tabs_Language_Language, "Language" },
            { Localisation::Tabs_Tablet_Title, "Tablet Settings" },
            { Localisation::Tabs_Tablet_Device, "Device" },
            { Localisation::Tabs_Tablet_PressureCurve, "Pressure Curve" },
            { Localisation::Tabs_Tablet_Width, "Width" },
            { Localisation::Tabs_Tablet_Height, "Height" },
            { Localisation::Tabs_Tablet_OffsetX, "Offset X" },
            { Localisation::Tabs_Tablet_OffsetY, "Offset Y" },
            { Localisation::Tabs_Tablet_FullArea, "Full Area" },
            { Localisation::Tabs_Tablet_ForceProportions, "Force Proportions" },
            { Localisation::Tabs_Monitor_Title, "Monitor Settings" },
            { Localisation::Tabs_Monitor_Monitor, "Monitor" },
            { Localisation::Tabs_Monitor_Width, "Width" },
            { Localisation::Tabs_Monitor_Height, "Height" },
            { Localisation::Tabs_Monitor_OffsetX, "Offset X" },
            { Localisation::Tabs_Monitor_OffsetY, "Offset Y" },
            { Localisation::Tabs_Monitor_FullArea, "Full Area" },
            { Localisation::Tabs_Monitor_ForceProportions, "Force Proportions" },
            { Localisation::Toast_Devices_Missing, "No devices were found" },
            { Localisation::Toast_Application_Settings_Saved, "Successfully saved application settings" },
            { Localisation::Toast_Device_Settings_Saved, "Successfully saved device settings" },
            { Localisation::Toast_Device_Settings_Load_Failed, "Failed to load device settings" },
            { Localisation::Toast_Device_Settings_Missing, "No saved device settings could be found, reading directly from xsetwacom instead" },
        }
    },
    {
        "pt_br", {
            { Localisation::Toast_Success, "Sucesso" },
            { Localisation::Toast_Warning, "Aviso" },
            { Localisation::Toast_Error, "Erro" },
            { Localisation::Save, "Salvar" },
            { Localisation::Save_Apply, "Salvar" },
            { Localisation::MenuBar_Settings, "Configurações" },
            { Localisation::MenuBar_Settings_Application, "Configurações da Aplicação..." },
            { Localisation::MenuBar_Other, "Outros" },
            { Localisation::MenuBar_Other_Goddess, "Deusa" },
            { Localisation::Popup_Settings_Tabs_Appearance_Title, "Aparência" },
            { Localisation::Popup_Settings_Tabs_Appearance_Theme, "Tema" },
            { Localisation::Popup_Settings_Tabs_Appearance_Theme_Dark, "Escuro" },
            { Localisation::Popup_Settings_Tabs_Appearance_Theme_Light, "Claro" },
            { Localisation::Popup_Settings_Tabs_Appearance_Scale, "Escala" },
            { Localisation::Popup_Settings_Tabs_Language_Title, "Língua" },
            { Localisation::Popup_Settings_Tabs_Language_Language, "Língua" },
            { Localisation::Tabs_Tablet_Title, "Configurações do Tablet" },
            { Localisation::Tabs_Tablet_Device, "Tablet" },
            { Localisation::Tabs_Tablet_PressureCurve, "Curva de Pressão" },
            { Localisation::Tabs_Tablet_Width, "Largura" },
            { Localisation::Tabs_Tablet_Height, "Altura" },
            { Localisation::Tabs_Tablet_OffsetX, "Offset X" },
            { Localisation::Tabs_Tablet_OffsetY, "Offset Y" },
            { Localisation::Tabs_Tablet_FullArea, "Área Completa" },
            { Localisation::Tabs_Tablet_ForceProportions, "Forçar Proporções" },
            { Localisation::Tabs_Monitor_Title, "Configurações do Monitor" },
            { Localisation::Tabs_Monitor_Monitor, "Monitor" },
            { Localisation::Tabs_Monitor_Width, "Largura" },
            { Localisation::Tabs_Monitor_Height, "Altura" },
            { Localisation::Tabs_Monitor_OffsetX, "Offset X" },
            { Localisation::Tabs_Monitor_OffsetY, "Offset Y" },
            { Localisation::Tabs_Monitor_FullArea, "Área Completa" },
            { Localisation::Tabs_Monitor_ForceProportions, "Forçar Proporções" },
            { Localisation::Toast_Devices_Missing, "Nenhum dispositivo encontrado" },
            { Localisation::Toast_Application_Settings_Saved, "As configurações da aplicação foram salvas com sucesso" },
            { Localisation::Toast_Device_Settings_Saved, "As configurações do tablet foram salvas com sucesso" },
            { Localisation::Toast_Device_Settings_Load_Failed, "Falha ao carregar configurações do tablet" },
            { Localisation::Toast_Device_Settings_Missing, "Não foi possível encontrar configurações para o dispositivo conectado, obtendo informações diretamente do xsetwacom" },
        }
    }
};
