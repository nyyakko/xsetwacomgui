cmake_minimum_required(VERSION 3.25)

project(
    xsetwacomgui
        LANGUAGES CXX
        DESCRIPTION "A graphical xsetwacom wrapper for ease of use"
)

include(cmake/helpers/add_subproject.cmake)
include(cmake/helpers/use_cpm.cmake)
include(cmake/packaging.cmake)

find_package(Freetype REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

CPMAddPackage(URI "gh:ericniebler/range-v3#master"  EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:fmtlib/fmt#10.2.1"            EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:gabime/spdlog@1.15.3"         EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:glfw/glfw#3.4"                EXCLUDE_FROM_ALL YES OPTIONS "GLFW_BUILD_WAYLAND OFF")
CPMAddPackage(URI "gh:Neargye/magic_enum@0.9.7"     EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:nlohmann/json@3.12.0"         EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:nyyakko/imgui#master"         EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:nyyakko/LibError#master"      EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:nyyakko/LibExec#master"       EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:OlivierLDff/asio.cmake@1.3.1" EXCLUDE_FROM_ALL YES)

include(cmake/static_analyzers.cmake)
include(GNUInstallDirs)

add_subproject(xsetwacomgui
    CC_OPTIONS "-Wno-gnu-statement-expression-from-macro-expansion"
    LIBRARIES
        asio::asio fmt::fmt freetype glfw imgui::imgui LibError::LibError
        LibExec::LibExec magic_enum::magic_enum nlohmann_json::nlohmann_json
        OpenGL::GL range-v3::range-v3 spdlog::spdlog udev
)
