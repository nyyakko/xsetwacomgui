#include "app/ui/AboutWindow.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "app/core/Localisation.hpp"
#include "app/core/Scaling.hpp"
#include "external/stb_image/stb_image.h"
#include "platform/Environment.hpp"

#include <fmt/core.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <imgui/imgui.hpp>
#include <imgui/imgui_impl_glfw.hpp>
#include <imgui/imgui_impl_opengl3.hpp>
#include <range/v3/algorithm.hpp>

#include <limits>
#include <random>

using namespace liberror;

static std::filesystem::path get_random_jahy()
{
    auto count = 0;

    ranges::for_each(std::filesystem::directory_iterator(get_application_images_path()), [&] (auto) {
        count += 1;
    });

    std::random_device device;
    std::mt19937 generator(device());
    std::uniform_int_distribution<> distribution(0, count-1);

    return get_application_images_path() / fmt::format("Jahy-{}.png", distribution(generator));
}

Result<void> render_about_window(Context& context)
{
    static auto width = 0, height = 0;
    static auto channels = 0;
    static auto imagePath = get_random_jahy();
    static auto image = stbi_load(imagePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    static GLuint imageTexture = std::numeric_limits<GLuint>::max();

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

    if (imageTexture == std::numeric_limits<GLuint>::max())
    {
        return make_error("Failed to load {}", imagePath.string());
    }

    static ImVec2 frameDimensions { float(width) * 70/100, float(height) * 70/100 };
    ImGui::SetCursorPos({ (ImGui::GetWindowWidth() - frameDimensions.x) / 2, (ImGui::GetWindowHeight() - frameDimensions.y) / 2 });
    ImGui::Image(imageTexture, frameDimensions);

    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - (45_scaled + ImGui::GetStyle().WindowPadding.x));

    ImGui::Text("%s: nyakonyns@gmail.com", TRY(Localisation::get(context.settings.language(), Localisation::Author)));
    ImGui::Text("%s: %s", TRY(Localisation::get(context.settings.language(), Localisation::Version)), VERSION);

    return {};
}
