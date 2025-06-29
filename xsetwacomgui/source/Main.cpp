#define IMGUI_DEFINE_MATH_OPERATORS

#include <spdlog/spdlog.h>

#include "platform/Environment.hpp"
#include "platform/events/USBEvent.hpp"
#include "platform/Display.hpp"
#include "settings/ApplicationSettings.hpp"
#include "settings/TabletSettings.hpp"
#include "ui/FreeType.hpp"
#include "ui/Localisation.hpp"
#include "ui/Scaling.hpp"
#include "ui/widgets/AreaMapper.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image/stb_image.h"

#include <argparse/argparse.hpp>
#include <imgui/extensions/imgui_text.hpp>
#include <imgui/extensions/imgui_toast.hpp>
#include <imgui/extensions/imgui_bezier.hpp>
#include <imgui/imgui.hpp>
#include <imgui/imgui_impl_glfw.hpp>
#include <imgui/imgui_impl_opengl3.hpp>
#include <libwacom/Device.hpp>
#include <liberror/Try.hpp>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <fplus/fplus.hpp>
#include <scn/scan.h>

#include <ranges>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <span>

struct Context
{
    ApplicationSettings& applicationSettings;
    TabletSettings& tabletSettings;

    libwacom::Device device {};
    Display display {};

    bool handleOutdatedDeviceSettings = false;

    uint8_t hasChangedDevice = false;
    uint8_t hasChangedDeviceHandedness = false;
    bool hasChangedDeviceArea = false;
    bool hasChangedDevicePressure = false;
    uint8_t hasChangedDisplay = false;
    bool hasChangedDisplayArea = false;
    bool hasTriedToInitializeDeviceSettings = false;
};

static constexpr auto USB_ACTION_MAGIC = 3;

liberror::Result<void> render_settings_popup_appearance_tab(Context const& context)
{
    ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Popup_Settings_Tabs_Appearance_Theme)));
    char const* themes[] = {
        TRY(Localisation::get(context.applicationSettings.language, Localisation::Popup_Settings_Tabs_Appearance_Theme_Dark)),
        TRY(Localisation::get(context.applicationSettings.language, Localisation::Popup_Settings_Tabs_Appearance_Theme_Light))
    };
    static int themeIndex = static_cast<int>(context.applicationSettings.theme);
    ImGui::SetNextItemWidth(300_scaled + ImGui::GetStyle().WindowPadding.x);
    auto hasChangedUITheme = ImGui::Combo("##Theme", &themeIndex, themes, std::size(themes));

    if (hasChangedUITheme)
    {
        context.applicationSettings.theme = ApplicationSettings::Theme::from_int(themeIndex);
    }

    static auto fonts = get_available_fonts();
    static auto fontsInfoView = fonts | std::views::values;
    static std::vector<std::vector<FontInfo>> fontsInfo(fontsInfoView.begin(), fontsInfoView.end());

    static auto fontsFamily = fplus::transform([] (std::vector<FontInfo> const& fontInfo) { return fontInfo.front().family.data(); }, fontsInfo);
    static auto fontFamilyIndex = static_cast<int>(std::distance(fonts.begin(), fonts.find(context.applicationSettings.font.family)));

    static auto fontStyles = fplus::transform([] (FontInfo const& fontInfo) { return fontInfo.style.data(); }, fontsInfo.at(static_cast<size_t>(fontFamilyIndex)));
    static auto fontStyleIndex = static_cast<int>(std::distance(fontStyles.begin(), std::ranges::find(fontStyles, context.applicationSettings.font.style)));

    auto hasChangedUIFont = false;
    auto hasChangedUIFontStyle = false;

    ImGui::BeginGroup();
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Popup_Settings_Tabs_Appearance_Font)));
        ImGui::SetNextItemWidth(150_scaled);
        hasChangedUIFont = ImGui::Combo("##FontFamily", &fontFamilyIndex, fontsFamily.data(), static_cast<int>(fontsFamily.size()));
    }
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Popup_Settings_Tabs_Appearance_FontStyle)));
        ImGui::SetNextItemWidth(150_scaled);
        hasChangedUIFontStyle = ImGui::Combo("##FontStyle", &fontStyleIndex, fontStyles.data(), static_cast<int>(fontStyles.size()));
    }
    ImGui::EndGroup();

    if (hasChangedUIFont || hasChangedUIFontStyle)
    {
        if (hasChangedUIFont)
        {
            fontStyles = fplus::transform([] (FontInfo const& fontInfo) { return fontInfo.style.data(); }, fontsInfo.at(static_cast<size_t>(fontFamilyIndex)));
            fontStyleIndex = 0;
        }

        context.applicationSettings.font = fonts.at(fontsFamily.at(static_cast<size_t>(fontFamilyIndex))).at(static_cast<size_t>(fontStyleIndex));
    }

    return {};
}

liberror::Result<void> render_settings_popup_display_tab(Context const& context)
{
    ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Popup_Settings_Tabs_Display_Scale)));
    static float scale = context.applicationSettings.scale;
    auto hasChangedUIScale = ImGui::InputFloat("##UiScale", &scale, 0.1f);

    if (hasChangedUIScale)
    {
        scale = ImClamp(scale, 1.0f, 10.f);
        context.applicationSettings.scale = scale;
    }

    return {};
}

liberror::Result<void> render_settings_popup_language_tab(Context const& context)
{
    ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Popup_Settings_Tabs_Language_Language)));

    static auto languages = get_available_languages();
    static auto languagesData = fplus::transform([] (std::string const& language) { return language.data(); }, languages);

    static int languageIndex = static_cast<int>(
        std::distance(languages.begin(), std::ranges::find(languages, context.applicationSettings.language))
    );

    auto hasChangedUILanguage = ImGui::Combo("##Language", &languageIndex, languagesData.data(), static_cast<int>(languages.size()));

    if (hasChangedUILanguage)
    {
        context.applicationSettings.language = languages.at(static_cast<size_t>(languageIndex));
    }

    return {};
}

liberror::Result<void> render_settings_popup(Context const& context)
{
    if (ImGui::BeginTabBar("##Tabs_2"))
    {
        if (ImGui::BeginTabItem(TRY(Localisation::get(context.applicationSettings.language, Localisation::Popup_Settings_Tabs_Appearance_Title))))
        {
            TRY(render_settings_popup_appearance_tab(context));
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(TRY(Localisation::get(context.applicationSettings.language, Localisation::Popup_Settings_Tabs_Display_Title))))
        {
            TRY(render_settings_popup_display_tab(context));
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(TRY(Localisation::get(context.applicationSettings.language, Localisation::Popup_Settings_Tabs_Language_Title))))
        {
            TRY(render_settings_popup_language_tab(context));
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    auto previousCursorPosition = ImGui::GetCursorPos();
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - (25_scaled + ImGui::GetStyle().WindowPadding.x));
    if (ImGui::Button(TRY(Localisation::get(context.applicationSettings.language, Localisation::Save)), { 100_scaled, 25_scaled }))
    {
        ImGui::PushToast(
            TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Success)),
            TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Application_Settings_Saved))
        );
        save_application_settings(context.applicationSettings);
    }
    ImGui::SetCursorPos(previousCursorPosition);

    return {};
}

void render_goddess_popup()
{
    static auto width = 0, height = 0;
    static auto channels = 0;
    static auto image = stbi_load((get_application_data_path() / "images" / "jahy.png").c_str(), &width, &height, &channels, STBI_rgb_alpha);
    static GLuint imageTexture;

    if (image != nullptr)
    {
        glGenTextures(1, &imageTexture);
        glBindTexture(GL_TEXTURE_2D, imageTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        stbi_image_free(image);
        image = nullptr;
    }

    static ImVec2 frameDimensions { static_cast<float>(width) * 70/100, static_cast<float>(height) * 70/100 };
    ImGui::SetCursorPos({ (ImGui::GetWindowWidth() - frameDimensions.x) / 2, (ImGui::GetWindowHeight() - frameDimensions.y) / 2 });
    ImGui::Image(imageTexture, frameDimensions);
}

liberror::Result<void> apply_settings_to_device(Context const& context)
{
    TRY(libwacom::set_stylus_area(context.device.id, context.tabletSettings.device.area));
    TRY(libwacom::set_stylus_handedness(context.device.id, context.tabletSettings.device.handedness));
    TRY(libwacom::set_stylus_pressure_curve(context.device.id, context.tabletSettings.device.pressure));
    auto displayArea = context.tabletSettings.display.area;
    displayArea.offsetX += context.display.area.offsetX;
    displayArea.offsetY += context.display.area.offsetY;
    TRY(libwacom::set_stylus_output_from_display_area(context.device.id, displayArea));
    return {};
}

liberror::Result<void> render_region_mappers(Context& context, std::vector<libwacom::Device> const& devices, std::vector<Display> const& displays)
{
    auto [cursorX, cursorY] = ImGui::GetCursorPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    static ImVec2 displayAreaAnchors[4] { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } };
    static libwacom::Area displayDefaultArea = displays.empty() ? libwacom::Area {} : libwacom::Area { 0, 0, context.display.area.width, context.display.area.height };

    if (context.hasChangedDisplayArea && context.tabletSettings.display.forceFullArea && context.tabletSettings.display.name != "INVALID")
    {
        context.tabletSettings.display.area = displayDefaultArea;
    }

    if (context.hasChangedDisplay && context.tabletSettings.display.name != "INVALID")
    {
        displayDefaultArea = libwacom::Area { 0, 0, context.display.area.width, context.display.area.height };
    }

    if (!displays.empty() && context.tabletSettings.display.name != "INVALID")
    {
        displayAreaAnchors[0] = {
            context.tabletSettings.display.area.offsetX / displayDefaultArea.width,
            context.tabletSettings.display.area.offsetY / displayDefaultArea.height
        };
        displayAreaAnchors[1] = {
            context.tabletSettings.display.area.offsetX / displayDefaultArea.width,
            (context.tabletSettings.display.area.height + context.tabletSettings.display.area.offsetY) / displayDefaultArea.height
        };
        displayAreaAnchors[2] = {
            (context.tabletSettings.display.area.width + context.tabletSettings.display.area.offsetX) / displayDefaultArea.width,
            context.tabletSettings.display.area.offsetY / displayDefaultArea.height
        };
        displayAreaAnchors[3] = {
            (context.tabletSettings.display.area.width + context.tabletSettings.display.area.offsetX) / displayDefaultArea.width,
            (context.tabletSettings.display.area.height + context.tabletSettings.display.area.offsetY) / displayDefaultArea.height
        };
    }
    else
    {
        displayAreaAnchors[0] = { 0, 0 };
        displayAreaAnchors[1] = { 0, 1 };
        displayAreaAnchors[2] = { 1, 0 };
        displayAreaAnchors[3] = { 1, 1 };
    }

    static const ImVec2 displayMapperSize { 20 * 16_scaled, 20 * 9_scaled };
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - displayMapperSize.x)/2);
    static ImRect displayMapperPosition {};
    context.hasChangedDisplayArea = area_mapper(TRY(Localisation::get(context.applicationSettings.language, Localisation::Tabs_Display_Display)), displayAreaAnchors, displayMapperSize, &displayMapperPosition, context.tabletSettings.display.forceFullArea, context.tabletSettings.display.forceAspectRatio);
    ImGui::SetCursorPosX(cursorX);

    if (context.hasChangedDisplayArea && context.tabletSettings.display.name != "INVALID")
    {
        context.tabletSettings.display.area = {
            .offsetX = displayAreaAnchors[0].x * displayDefaultArea.width,
            .offsetY = displayAreaAnchors[0].y * displayDefaultArea.height,
            .width   = (displayAreaAnchors[2].x - displayAreaAnchors[0].x) * displayDefaultArea.width,
            .height  = (displayAreaAnchors[3].y - displayAreaAnchors[2].y) * displayDefaultArea.height
        };
    }

    static ImVec2 deviceAreaAnchors[4] { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } };
    static libwacom::Area deviceDefaultArea = devices.empty() ? libwacom::Area {} : TRY(libwacom::get_stylus_default_area(context.device.id));

    if (context.hasChangedDeviceArea && context.tabletSettings.device.forceFullArea && context.tabletSettings.device.name != "INVALID")
    {
        context.tabletSettings.device.area = deviceDefaultArea;
    }

    if (context.hasChangedDevice && context.tabletSettings.device.name != "INVALID")
    {
        deviceDefaultArea = TRY(libwacom::get_stylus_default_area(context.device.id));
    }

    if (!devices.empty() && context.tabletSettings.device.name != "INVALID")
    {
        deviceAreaAnchors[0] = {
            context.tabletSettings.device.area.offsetX / deviceDefaultArea.width,
            context.tabletSettings.device.area.offsetY / deviceDefaultArea.height
        };
        deviceAreaAnchors[1] = {
            context.tabletSettings.device.area.offsetX / deviceDefaultArea.width,
            (context.tabletSettings.device.area.height + context.tabletSettings.device.area.offsetY) / deviceDefaultArea.height
        };
        deviceAreaAnchors[2] = {
            (context.tabletSettings.device.area.width + context.tabletSettings.device.area.offsetX) / deviceDefaultArea.width,
            context.tabletSettings.device.area.offsetY / deviceDefaultArea.height
        };
        deviceAreaAnchors[3] = {
            (context.tabletSettings.device.area.width + context.tabletSettings.device.area.offsetX) / deviceDefaultArea.width,
            (context.tabletSettings.device.area.height + context.tabletSettings.device.area.offsetY) / deviceDefaultArea.height
        };
    }
    else
    {
        deviceAreaAnchors[0] = { 0, 0 };
        deviceAreaAnchors[1] = { 0, 1 };
        deviceAreaAnchors[2] = { 1, 0 };
        deviceAreaAnchors[3] = { 1, 1 };
    }

    static const ImVec2 deviceMapperSize { 15 * 16_scaled, 15 * 9_scaled };
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - deviceMapperSize.x)/2);
    static ImRect deviceMapperPosition {};
    context.hasChangedDeviceArea = area_mapper(TRY(Localisation::get(context.applicationSettings.language, Localisation::Tabs_Tablet_Device)), deviceAreaAnchors, deviceMapperSize, &deviceMapperPosition, context.tabletSettings.device.forceFullArea, context.tabletSettings.device.forceAspectRatio);
    ImGui::SetCursorPosX(cursorX);

    if (context.hasChangedDeviceArea && context.tabletSettings.device.name != "INVALID")
    {
        context.tabletSettings.device.area = {
            .offsetX = deviceAreaAnchors[0].x * deviceDefaultArea.width,
            .offsetY = deviceAreaAnchors[0].y * deviceDefaultArea.height,
            .width   = (deviceAreaAnchors[2].x - deviceAreaAnchors[0].x) * deviceDefaultArea.width,
            .height  = (deviceAreaAnchors[3].y - deviceAreaAnchors[2].y) * deviceDefaultArea.height
        };
    }

    for (auto [displayAnchor, deviceAnchor] : fplus::zip(std::span<ImVec2>(displayAreaAnchors, 4), std::span<ImVec2>(deviceAreaAnchors, 4)))
    {
        auto p1 = displayAnchor * (displayMapperPosition.Max - displayMapperPosition.Min) + displayMapperPosition.Min;
        auto p2 = deviceAnchor * (deviceMapperPosition.Max - deviceMapperPosition.Min) + deviceMapperPosition.Min;
        drawList->AddLine(p1, p2, ImColor(255, 0, 0, 127), 2.f);
    }

    return {};
}

liberror::Result<void> render_tablet_settings_tab(Context& context, std::vector<libwacom::Device> const& devices)
{
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (250_scaled + 300_scaled + ImGui::GetStyle().WindowPadding.x))/2);

    static libwacom::Area deviceDefaultArea = devices.empty() ? libwacom::Area {} : TRY(libwacom::get_stylus_default_area(context.device.id));

    if (context.hasChangedDevice && context.tabletSettings.device.name != "INVALID")
    {
        deviceDefaultArea = TRY(libwacom::get_stylus_default_area(context.device.id));
    }

    ImGui::BeginGroup();
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Tabs_Tablet_Device)));
        auto deviceNames = fplus::transform([] (libwacom::Device const& device) { return device.name.data(); }, devices);
        ImGui::SetNextItemWidth(300_scaled + ImGui::GetStyle().WindowPadding.x);
        static int deviceIndex = context.tabletSettings.device.name == "INVALID" ? 0 : static_cast<int>(
            std::distance(devices.begin(), std::ranges::find(devices, context.tabletSettings.device.name, &libwacom::Device::name))
        );

        if (context.hasChangedDevice && (context.hasChangedDevice & USB_ACTION_MAGIC) == 0)
        {
            deviceIndex = context.tabletSettings.device.name == "INVALID" ? 0 : static_cast<int>(
                std::distance(devices.begin(), std::ranges::find(devices, context.tabletSettings.device.name, &libwacom::Device::name))
            );
        }

        context.hasChangedDevice = ImGui::Combo("##Device", &deviceIndex, deviceNames.data(), static_cast<int>(deviceNames.size()));

        if (context.hasChangedDevice)
        {
            context.device = devices.at(static_cast<size_t>(deviceIndex));
            context.tabletSettings.device.name = context.device.name;
            context.tabletSettings.device.area = TRY(libwacom::get_stylus_default_area(context.device.id));
            context.tabletSettings.device.pressure = { 0, 0, 1, 1 };
            context.tabletSettings.device.forceFullArea = false;
            context.tabletSettings.device.forceAspectRatio = false;
        }

        ImGui::BeginDisabled(context.tabletSettings.device.forceFullArea);
        {
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Tabs_Tablet_Width)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletWidth", &context.tabletSettings.device.area.width, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Tabs_Tablet_Height)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletHeight", &context.tabletSettings.device.area.height, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
        }
        {
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Tabs_Tablet_OffsetX)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletOffsetX", &context.tabletSettings.device.area.offsetX, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Tabs_Tablet_OffsetY)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDeviceArea |= ImGui::InputFloat("##TabletOffsetY", &context.tabletSettings.device.area.offsetY, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
        }
        ImGui::EndDisabled();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Tabs_Tablet_Orientation)));
        char const* orientations[] = {
            TRY(Localisation::get(context.applicationSettings.language, Localisation::Tabs_Tablet_Orientation_Left)),
            TRY(Localisation::get(context.applicationSettings.language, Localisation::Tabs_Tablet_Orientation_Right)),
        };
        ImGui::SetNextItemWidth(150_scaled);
        static int orientationIndex = static_cast<int>(context.tabletSettings.device.handedness.to_int());

        if (context.hasChangedDeviceHandedness && (context.hasChangedDeviceHandedness & USB_ACTION_MAGIC) == 0)
        {
            orientationIndex = static_cast<int>(context.tabletSettings.device.handedness.to_int());
        }

        context.hasChangedDeviceHandedness = ImGui::Combo("##Orientations", &orientationIndex, orientations, std::size(orientations));

        if (context.hasChangedDeviceHandedness)
        {
            context.tabletSettings.device.handedness = libwacom::Handedness::from_int(orientationIndex);
        }

        ImGui::BeginGroup();
        {
            context.hasChangedDeviceArea |= ImGui::Checkbox(TRY(Localisation::get(context.applicationSettings.language, Localisation::Tabs_Tablet_FullArea)), &context.tabletSettings.device.forceFullArea);
            ImGui::BeginDisabled();
            ImGui::Checkbox(TRY(Localisation::get(context.applicationSettings.language, Localisation::Tabs_Tablet_ForceProportions)), &context.tabletSettings.device.forceAspectRatio);
            ImGui::EndDisabled();
        }
        ImGui::EndGroup();
    }
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
    {
        static float devicePressureAnchors[4] = {};

        if (!devices.empty() && context.tabletSettings.device.name != "INVALID")
        {
            devicePressureAnchors[0] = context.tabletSettings.device.pressure.minX;
            devicePressureAnchors[1] = context.tabletSettings.device.pressure.minY;
            devicePressureAnchors[2] = context.tabletSettings.device.pressure.maxX;
            devicePressureAnchors[3] = context.tabletSettings.device.pressure.maxY;
        }
        else
        {
            devicePressureAnchors[0] = 0;
            devicePressureAnchors[1] = 0;
            devicePressureAnchors[2] = 1;
            devicePressureAnchors[3] = 1;
        }

        ImGui::AlignTextToFramePadding();
        context.hasChangedDevicePressure = ImGui::BezierEditor(TRY(Localisation::get(context.applicationSettings.language, Localisation::Tabs_Tablet_PressureCurve)), devicePressureAnchors, { 250_scaled, 250_scaled });

        if (context.hasChangedDevicePressure)
        {
            context.tabletSettings.device.pressure = { devicePressureAnchors[0], devicePressureAnchors[1], devicePressureAnchors[2], devicePressureAnchors[3] };
        }
    }
    ImGui::EndGroup();

    if (context.hasChangedDeviceArea && context.tabletSettings.device.name != "INVALID")
    {
        context.tabletSettings.device.area = {
            .offsetX = std::clamp(context.tabletSettings.device.area.offsetX, 0.f, deviceDefaultArea.width),
            .offsetY = std::clamp(context.tabletSettings.device.area.offsetY, 0.f, deviceDefaultArea.height),
            .width   = std::clamp(context.tabletSettings.device.area.width, 0.f, deviceDefaultArea.width),
            .height  = std::clamp(context.tabletSettings.device.area.height, 0.f, deviceDefaultArea.height)
        };
    }

    return {};
}

liberror::Result<void> render_display_settings_tab(Context& context, std::vector<Display> const& displays)
{
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (300_scaled + ImGui::GetStyle().WindowPadding.x))/2);

    static libwacom::Area displayDefaultArea = displays.empty() ? libwacom::Area {} : libwacom::Area { 0, 0, context.display.area.width, context.display.area.height };

    if (context.hasChangedDisplay && context.tabletSettings.display.name != "INVALID")
    {
        displayDefaultArea = libwacom::Area { 0, 0, context.display.area.width, context.display.area.height };
    }

    ImGui::BeginGroup();
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Tabs_Display_Display)));
        auto displayNames = fplus::transform([] (Display const& display) { return fmt::format("{} ({}x{})", display.name, display.area.width, display.area.height); }, displays);
        auto displayNamesData = fplus::transform([] (std::string const& name) { return name.data(); }, displayNames);
        ImGui::SetNextItemWidth(300_scaled + ImGui::GetStyle().WindowPadding.x);
        static int displayIndex = context.tabletSettings.display.name == "INVALID" ? 0 : static_cast<int>(
            std::distance(displays.begin(), std::ranges::find(displays, context.tabletSettings.display.name, &Display::name))
        );

        if (context.hasChangedDisplay && (context.hasChangedDisplay & USB_ACTION_MAGIC) == 0)
        {
            displayIndex = context.tabletSettings.display.name == "INVALID" ? 0 : static_cast<int>(
                std::distance(displays.begin(), std::ranges::find(displays, context.tabletSettings.display.name, &Display::name))
            );
        }

        context.hasChangedDisplay = ImGui::Combo("##Displays", &displayIndex, displayNamesData.data(), static_cast<int>(displayNamesData.size()));

        if (context.hasChangedDisplay)
        {
            context.display = displays.at(static_cast<size_t>(displayIndex));
            context.tabletSettings.display.name = context.display.name;
            context.tabletSettings.display.area = libwacom::Area { 0, 0, context.display.area.width, context.display.area.height };
            context.tabletSettings.display.forceFullArea = false;
            context.tabletSettings.display.forceAspectRatio = false;
        }

        ImGui::BeginDisabled(context.tabletSettings.display.forceFullArea);
        {
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Tabs_Display_Width)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDisplayArea |= ImGui::InputFloat("##DisplayWidth", &context.tabletSettings.display.area.width, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Tabs_Display_Height)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDisplayArea |= ImGui::InputFloat("##DisplayHeight", &context.tabletSettings.display.area.height, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
        }
        {
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Tabs_Display_OffsetX)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDisplayArea |= ImGui::InputFloat("##DisplayOffsetX", &context.tabletSettings.display.area.offsetX, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", TRY(Localisation::get(context.applicationSettings.language, Localisation::Tabs_Display_OffsetY)));
                ImGui::SetNextItemWidth(150_scaled);
                context.hasChangedDisplayArea |= ImGui::InputFloat("##DisplayOffsetY", &context.tabletSettings.display.area.offsetY, 0.f, 0.f, "%.0f");
            }
            ImGui::EndGroup();
        }
        ImGui::EndDisabled();

        ImGui::BeginGroup();
        {
            context.hasChangedDisplayArea |= ImGui::Checkbox(TRY(Localisation::get(context.applicationSettings.language, Localisation::Tabs_Display_FullArea)), &context.tabletSettings.display.forceFullArea);
            ImGui::BeginDisabled();
            ImGui::Checkbox(TRY(Localisation::get(context.applicationSettings.language, Localisation::Tabs_Display_ForceProportions)), &context.tabletSettings.display.forceAspectRatio);
            ImGui::EndDisabled();
        }
        ImGui::EndGroup();
    }
    ImGui::EndGroup();

    if (context.hasChangedDisplayArea && context.tabletSettings.display.name != "INVALID")
    {
        context.tabletSettings.display.area = {
            .offsetX = std::clamp(context.tabletSettings.display.area.offsetX, 0.f, displayDefaultArea.width),
            .offsetY = std::clamp(context.tabletSettings.display.area.offsetY, 0.f, displayDefaultArea.height),
            .width   = std::clamp(context.tabletSettings.display.area.width, 0.f, displayDefaultArea.width),
            .height  = std::clamp(context.tabletSettings.display.area.height, 0.f, displayDefaultArea.height)
        };
    }

    return {};
}

liberror::Result<void> render_window(Context& context, std::vector<libwacom::Device> const& devices, std::vector<Display> const& displays)
{
#ifdef DEBUG
    static std::once_flag debugWarningFlag {};
    std::call_once(debugWarningFlag, [&] () {
        ImGui::PushToast("Debug", "You are running a debug build!");
    });
#endif

    if (context.handleOutdatedDeviceSettings)
    {
        auto [windowWidth, windowHeight] = ImGui::GetWindowSize();

        float deviceSettingsMigrationWidth = 400_scaled, deviceSettingsMigrationHeight = 150_scaled;
        ImGui::SetNextWindowSize({ deviceSettingsMigrationWidth, deviceSettingsMigrationHeight });
        ImGui::SetNextWindowPos({ (static_cast<float>(windowWidth) - deviceSettingsMigrationWidth)/2, (static_cast<float>(windowHeight) - deviceSettingsMigrationHeight)/2 });
        ImGui::Begin(
            TRY(Localisation::get(context.applicationSettings.language, Localisation::Popup_Outdated_Device_Settings_Title)),
            nullptr,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
        );
        {
            auto [popupWidth, popupHeight] = ImGui::GetWindowSize();

            ImGui::BeginGroup();
            {
                for (auto messageLine :
                    ImGui::SplitToWidth(TRY(Localisation::get(context.applicationSettings.language, Localisation::Popup_Outdated_Device_Settings_Text)), static_cast<int>(popupWidth)))
                {
                    ImGui::Text("%s", messageLine.data());
                }
            }
            ImGui::EndGroup();

            ImGui::SetCursorPosY(popupHeight - (25_scaled + ImGui::GetStyle().WindowPadding.y));
            if (ImGui::Button(TRY(Localisation::get(context.applicationSettings.language, Localisation::Popup_Outdated_Device_Settings_Overwrite)), { 0, 25_scaled }))
            {
                ImGui::PushToast(
                    TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Success)),
                    TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Device_Settings_Overwritten))
                );

                save_tablet_settings(context.tabletSettings);
                context.handleOutdatedDeviceSettings = false;
            }

            ImGui::SameLine();

            if (ImGui::Button(TRY(Localisation::get(context.applicationSettings.language, Localisation::Popup_Outdated_Device_Settings_Migrate)), { 0, 25_scaled }))
            {
                migrate_tablet_settings(context.tabletSettings);
            }
        }
        ImGui::End();
    }

    if (devices.empty() && !context.hasTriedToInitializeDeviceSettings)
    {
        context.hasTriedToInitializeDeviceSettings = true;
        ImGui::PushToast(
            TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Warning)),
            TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Devices_Missing))
        );
    }

    if (!(devices.empty() || context.hasTriedToInitializeDeviceSettings))
    {
        context.hasTriedToInitializeDeviceSettings = true;
        auto result = load_tablet_settings(context.tabletSettings);
        if (!result.has_value())
        {
            context.device = devices.back();
            context.hasChangedDevice = true;
            context.hasChangedDeviceHandedness = true;
            context.display = TRY(get_primary_display());
            context.hasChangedDisplay = true;
            context.tabletSettings.device.name = context.device.name;
            context.tabletSettings.device.area = TRY(libwacom::get_stylus_area(context.device.id));
            context.tabletSettings.device.pressure = TRY(libwacom::get_stylus_pressure_curve(context.device.id));
            context.tabletSettings.device.forceFullArea = false;
            context.tabletSettings.device.forceAspectRatio = false;
            context.tabletSettings.display.name = context.display.name;
            context.tabletSettings.display.area = { 0, 0, context.display.area.width, context.display.area.height };
            context.tabletSettings.display.forceFullArea = false;
            context.tabletSettings.display.forceAspectRatio = false;

            switch (result.error().message())
            {
            case SettingsError::Type::WRITE_FAILURE: break;
            case SettingsError::Type::FILE_NOT_FOUND: {
                ImGui::PushToast(
                    TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Warning)),
                    TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Device_Settings_Missing))
                );
                save_tablet_settings(context.tabletSettings);
                break;
            }
            case SettingsError::Type::READ_FAILURE: {
                ImGui::PushToast(
                    TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Warning)),
                    TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Device_Settings_Load_Failed))
                );
                break;
            }
            case SettingsError::Type::OUTDATED_SCHEMA: {
                context.handleOutdatedDeviceSettings = true;
                break;
            }
            }
        }
        else
        {
            context.display = *std::ranges::find(displays, context.tabletSettings.display.name, &Display::name);
            context.device  = *std::ranges::find(devices, context.tabletSettings.device.name, &libwacom::Device::name);
        }
    }

    ImGui::BeginDisabled(context.handleOutdatedDeviceSettings);
    ImGui::BeginGroup();
    {
        render_region_mappers(context, devices, displays);
    }
    ImGui::EndGroup();

    if (ImGui::BeginTabBar("##Tabs_1"))
    {
        if (ImGui::BeginTabItem(TRY(Localisation::get(context.applicationSettings.language, Localisation::Tabs_Tablet_Title))))
        {
            TRY(render_tablet_settings_tab(context, devices));
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(TRY(Localisation::get(context.applicationSettings.language, Localisation::Tabs_Display_Title))))
        {
            TRY(render_display_settings_tab(context, displays));
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    auto previousCursorPosition = ImGui::GetCursorPos();
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - (35_scaled + ImGui::GetStyle().WindowPadding.x));
    if (ImGui::Button(TRY(Localisation::get(context.applicationSettings.language, Localisation::Save_Apply)), { 200_scaled, 35_scaled }))
    {
        save_tablet_settings(context.tabletSettings);
        ImGui::PushToast(
            TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Success)),
            TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Device_Settings_Saved))
        );
        TRY(apply_settings_to_device(context));
    }
    ImGui::SetCursorPos(previousCursorPosition);
    ImGui::EndDisabled();

    if (context.hasChangedDevice && (context.hasChangedDevice & USB_ACTION_MAGIC) == 0) context.hasChangedDevice = false;
    if (context.hasChangedDisplay && (context.hasChangedDisplay & USB_ACTION_MAGIC) == 0) context.hasChangedDisplay = false;

    return {};
}

liberror::Result<void> safe_main(std::span<char const*> const& arguments)
{
    argparse::ArgumentParser parser(NAME, "", argparse::default_arguments::help);
    parser.add_description("A graphical xsetwacom wrapper for ease of use.");

    argparse::ArgumentParser configCommand("config", "", argparse::default_arguments::help);
    configCommand.add_description("manages device related configuration");
    configCommand.add_argument("--load").help("loads the tablet configuration without loading the UI").flag();
    parser.add_subparser(configCommand);

    try
    {
        parser.parse_args(static_cast<int>(arguments.size()), arguments.data());
    }
    catch (std::exception const& exception)
    {
        return liberror::make_error(exception.what());
    }

    std::vector<Display> displays = TRY(get_available_displays());
    std::vector<libwacom::Device> devices = TRY(libwacom::get_available_devices());
    devices = fplus::keep_if([] (auto&& device) { return device.kind == libwacom::Device::Kind::STYLUS; }, devices);

    if (!(std::filesystem::exists(get_application_config_path()) || std::filesystem::create_directory(get_application_config_path())))
    {
        return liberror::make_error("Failed to create settings directory");
    }

    if (configCommand["--load"] != false)
    {
        TabletSettings tabletSettings {};

        if (!load_tablet_settings(tabletSettings))
        {
            return liberror::make_error("Failed to load device settings");
        }

        if (devices.empty() || displays.empty())
        {
            return liberror::make_error("Failed to load devices");
        }

        auto device  = devices.front();
        auto display = TRY(get_primary_display());

        TRY(libwacom::set_stylus_area(device.id, tabletSettings.device.area));
        TRY(libwacom::set_stylus_handedness(device.id, tabletSettings.device.handedness));
        TRY(libwacom::set_stylus_pressure_curve(device.id, tabletSettings.device.pressure));
        auto displayArea = tabletSettings.display.area;
        displayArea.offsetX += display.area.offsetX;
        displayArea.offsetY += display.area.offsetY;
        TRY(libwacom::set_stylus_output_from_display_area(device.id, displayArea));

        fmt::println("Device settings loaded successfully");

        return {};
    }

    ApplicationSettings applicationSettings {};
    TabletSettings tabletSettings {};

    if (!std::filesystem::exists(APPLICATION_SETTINGS_FILE))
    {
        save_application_settings(applicationSettings);
    }
    else
    {
        auto result = load_application_settings(applicationSettings);
        if (!result.has_value())
        {
            fmt::println("The currently saved application settings differs from");
            fmt::println("the expected format. You can:\n");

            fmt::println("1. Overwrite Everything");
            fmt::println("2. Migrate Manually\n");

            auto choice = scn::prompt<int>("How would you like to proceed? (choose a value) ", "{}");

            if (choice)
            {
                if (choice->value() == 1) save_application_settings(applicationSettings);
                else if (choice->value() == 2) migrate_application_settings(applicationSettings);
            }

            fmt::println("Done. Restart the application.");

            return {};
        }
    }

    if (!glfwInit())
    {
        return liberror::make_error("Failed to initialize glfw");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    set_scale(applicationSettings.scale);

#ifdef DEBUG
    auto window = glfwCreateWindow(static_cast<int>(800_scaled), static_cast<int>(815_scaled), NAME " - DEBUG BUILD", nullptr, nullptr);
#else
    auto window = glfwCreateWindow(static_cast<int>(800_scaled), static_cast<int>(815_scaled), NAME, nullptr, nullptr);
#endif

    glfwMakeContextCurrent(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    auto& io = ImGui::GetIO();

    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    ImFont* font = nullptr;

    ImVector<ImWchar> ranges {};
    ImFontGlyphRangesBuilder rangeBuilder {};

    static const ImWchar rangesData[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
        0,
    };

    rangeBuilder.AddRanges(rangesData);
    rangeBuilder.BuildRanges(&ranges);

    if (applicationSettings.font.family != "Default")
    {
        font = io.Fonts->AddFontFromFileTTF(applicationSettings.font.path.string().data(), 20_scaled, nullptr, ranges.Data);
    }

    Context context { applicationSettings, tabletSettings };

    USBEvent usbAction {};

    usbAction.subscribe([&] (std::string_view, USBEvent::Action event) -> liberror::Result<void> {
        if (event != USBEvent::Action::UNBIND) return {};

        auto hadMoreThanOneDevice = devices.size() > 1;
        devices = fplus::keep_if([] (auto&& device) { return device.kind == libwacom::Device::Kind::STYLUS; }, TRY(libwacom::get_available_devices()));
        if (hadMoreThanOneDevice) return {};

        auto maybeDevice = std::ranges::find(devices, context.tabletSettings.device.name, &libwacom::Device::name);
        if (maybeDevice == devices.end())
        {
            context.display = Display {};
            context.device = libwacom::Device {};
            context.tabletSettings = {};
        }

        return {};
    });

    usbAction.subscribe([&] (std::string_view, USBEvent::Action event) -> liberror::Result<void> {
        if (event != USBEvent::Action::BIND) return {};

        auto hadAtleastOneDevice = !devices.empty();
        devices = fplus::keep_if([] (auto&& device) { return device.kind == libwacom::Device::Kind::STYLUS; }, TRY(libwacom::get_available_devices()));
        if (hadAtleastOneDevice) return {};

        auto result = load_tablet_settings(context.tabletSettings);
        if (!result.has_value())
        {
            context.device = devices.back();
            context.hasChangedDevice = 0xFF ^ USB_ACTION_MAGIC;
            context.hasChangedDeviceHandedness = 0xFF ^ USB_ACTION_MAGIC;
            context.display = TRY(get_primary_display());
            context.hasChangedDisplay = 0xFF ^ USB_ACTION_MAGIC;
            context.tabletSettings.device.name = context.device.name;
            context.tabletSettings.device.area = TRY(libwacom::get_stylus_area(context.device.id));
            context.tabletSettings.device.pressure = TRY(libwacom::get_stylus_pressure_curve(context.device.id));
            context.tabletSettings.display.name = context.display.name;
            context.tabletSettings.display.area = { 0, 0, context.display.area.width, context.display.area.height };

            switch (result.error().message())
            {
            case SettingsError::Type::WRITE_FAILURE: break;
            case SettingsError::Type::FILE_NOT_FOUND: {
                ImGui::PushToast(
                    TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Warning)),
                    TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Device_Settings_Missing))
                );
                save_tablet_settings(context.tabletSettings);
                break;
            }
            case SettingsError::Type::READ_FAILURE: {
                ImGui::PushToast(
                    TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Warning)),
                    TRY(Localisation::get(context.applicationSettings.language, Localisation::Toast_Device_Settings_Load_Failed))
                );
                break;
            }
            case SettingsError::Type::OUTDATED_SCHEMA: {
                context.handleOutdatedDeviceSettings = true;
                break;
            }
            }
        }
        else if (devices.back().name == context.tabletSettings.device.name)
        {
            ImGui::PushToast(
                TRY(Localisation::get(applicationSettings.language, Localisation::Toast_Success)),
                TRY(Localisation::get(applicationSettings.language, Localisation::Toast_Device_Settings_Load_Success))
            );
            context.device = devices.back();
            context.hasChangedDevice = 0xFF ^ USB_ACTION_MAGIC;
            context.hasChangedDeviceHandedness = 0xFF ^ USB_ACTION_MAGIC;
            context.display = *std::ranges::find(displays, context.tabletSettings.display.name, &Display::name);
            context.hasChangedDisplay = 0xFF ^ USB_ACTION_MAGIC;
            TRY(apply_settings_to_device(context));
        }

        return {};
    });

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            break;
        }

        if (applicationSettings.theme == ApplicationSettings::Theme::DARK)
        {
            ImGui::StyleColorsDark();
        }
        else
        {
            ImGui::StyleColorsLight();
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::PushFont(font);
        {
            int windowWidth, windowHeight;
            glfwGetWindowSize(window, &windowWidth, &windowHeight);
            ImGui::SetNextWindowPos({});
            ImGui::SetNextWindowSize({ static_cast<float>(windowWidth), static_cast<float>(windowHeight) });
            ImGui::Begin(NAME, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);
            ImGui::RenderToasts();
            {
                static bool isApplicationSettingsOpen = false;
                static bool isGoddessOpen = false;

                TRY(usbAction.update());

                if (ImGui::BeginMenuBar())
                {
                    if (ImGui::BeginMenu(TRY(Localisation::get(applicationSettings.language, Localisation::MenuBar_Settings))))
                    {
                        if (ImGui::MenuItem(TRY(Localisation::get(applicationSettings.language, Localisation::MenuBar_Settings_Application))))
                        {
                            isApplicationSettingsOpen = true;
                        }

                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu(TRY(Localisation::get(applicationSettings.language, Localisation::MenuBar_Other))))
                    {
                        if (ImGui::MenuItem(TRY(Localisation::get(applicationSettings.language, Localisation::MenuBar_Other_Goddess))))
                        {
                            isGoddessOpen = true;
                        }

                        ImGui::EndMenu();
                    }

                    ImGui::EndMenuBar();
                }

                if (isApplicationSettingsOpen)
                {
                    float applicationSettingsWidth = static_cast<float>(windowWidth)/1.5f, applicationSettingsHeight = static_cast<float>(windowHeight)/1.5f;
                    ImGui::SetNextWindowSize({ applicationSettingsWidth, applicationSettingsHeight });
                    ImGui::SetNextWindowPos({ (static_cast<float>(windowWidth) - applicationSettingsWidth)/2, (static_cast<float>(windowHeight) - applicationSettingsHeight)/2 });
                    ImGui::Begin(
                        TRY(Localisation::get(applicationSettings.language, Localisation::MenuBar_Settings_Application)),
                        &isApplicationSettingsOpen,
                        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
                    );
                    {
                        render_settings_popup(context);
                    }
                    ImGui::End();
                }

                if (isGoddessOpen)
                {
                    float goddessWidth = static_cast<float>(windowWidth)/1.5f, goddessHeight = static_cast<float>(windowHeight)/1.5f;
                    ImGui::SetNextWindowSize({ goddessWidth, goddessHeight });
                    ImGui::SetNextWindowPos({ (static_cast<float>(windowWidth) - goddessWidth)/2, (static_cast<float>(windowHeight) - goddessHeight)/2 });
                    ImGui::Begin(
                        TRY(Localisation::get(applicationSettings.language, Localisation::MenuBar_Other_Goddess)),
                        &isGoddessOpen,
                        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
                    );
                    {
                        render_goddess_popup();
                    }
                    ImGui::End();
                }

                ImGui::BeginDisabled(devices.empty());
                {
                    TRY(render_window(context, devices, displays));
                }
                ImGui::EndDisabled();
            }
            ImGui::End();
        }
        ImGui::PopFont();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);

    glfwTerminate();

    return {};
}

int main(int argc, char const** argv)
{
    auto result = safe_main(std::span<char const*>(argv, size_t(argc)));

    if (!result.has_value())
    {
        spdlog::error("{}", result.error().message());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
