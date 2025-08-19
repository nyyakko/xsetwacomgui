#include "GoddessWindow.hpp"

#include "platform/Environment.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image/stb_image.h"

#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <imgui/imgui.hpp>
#include <imgui/imgui_impl_glfw.hpp>
#include <imgui/imgui_impl_opengl3.hpp>

#include <limits>

#define GODDESS_IMAGE_PATH (get_application_data_path() / "images" / "jahy.png").c_str()

using namespace liberror;

Result<void> render_goddess_window()
{
    static auto width = 0, height = 0;
    static auto channels = 0;
    static auto image = stbi_load(GODDESS_IMAGE_PATH, &width, &height, &channels, STBI_rgb_alpha);
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
        return make_error("Failed to load goddess image");
    }

    static ImVec2 frameDimensions { static_cast<float>(width) * 70/100, static_cast<float>(height) * 70/100 };
    ImGui::SetCursorPos({ (ImGui::GetWindowWidth() - frameDimensions.x) / 2, (ImGui::GetWindowHeight() - frameDimensions.y) / 2 });
    ImGui::Image(imageTexture, frameDimensions);

    return {};
}
