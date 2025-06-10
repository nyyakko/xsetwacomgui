cmake_minimum_required(VERSION 3.25)

project(xsetwacomgui LANGUAGES CXX)

include(cmake/get_cpm.cmake)
include(cmake/find_xrandr.cmake)
include(cmake/find_xsetwacom.cmake)

find_xrandr()
find_xsetwacom()
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

CPMAddPackage(URI "gh:Dobiasd/FunctionalPlus@0.2.24" EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:fmtlib/fmt#10.2.1"             EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:gabime/spdlog@1.15.3"          EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:nlohmann/json@3.12.0"          EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:nyyakko/expected#master"       EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:nyyakko/imgui#master"          EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:nyyakko/LibEnum#master"        EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:nyyakko/LibError#master"       EXCLUDE_FROM_ALL YES)
CPMAddPackage(URI "gh:nyyakko/LibWacom#master"       EXCLUDE_FROM_ALL YES)

include(cmake/static_analyzers.cmake)
include(GNUInstallDirs)

set(xsetwacomgui_CompilerOptions ${xsetwacomgui_CompilerOptions} -Wno-gnu-statement-expression-from-macro-expansion)
# set(xsetwacomgui_LinkerOptions ${xsetwacomgui_LinkerOptions})

set(xsetwacomgui_ExternalLibraries
    OpenGL::GL
    glfw
    imgui::imgui
    LibError::LibError
    LibEnum::LibEnum
    LibWacom::LibWacom
    FunctionalPlus::fplus
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    fmt::fmt
)

add_subdirectory(xsetwacomgui)

