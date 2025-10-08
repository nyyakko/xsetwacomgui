#include "GoddessWindow.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image/stb_image.h"
#include "platform/Environment.hpp"

#include <fmt/core.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <imgui/imgui.hpp>
#include <imgui/imgui_impl_glfw.hpp>
#include <imgui/imgui_impl_opengl3.hpp>

#include <algorithm>
#include <limits>
#include <random>

using namespace liberror;

static std::filesystem::path get_random_jahy()
{
    auto count = 0;

    std::ranges::for_each(std::filesystem::directory_iterator(get_application_data_path() / "images"), [&] (auto) {
        count += 1;
    });

    std::random_device device;
    std::mt19937 generator(device());
    std::uniform_int_distribution<> distribution(0, count-1);

    return get_application_data_path() / "images" / fmt::format("Jahy-{}.png", distribution(generator));
}

Result<void> render_goddess_window()
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

    return {};
}
