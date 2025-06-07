cmake_minimum_required(VERSION 3.25)

project(xsetwacomgui LANGUAGES CXX)

include(cmake/get_cpm.cmake)

CPMAddPackage("gh:fmtlib/fmt#10.2.1")
CPMAddPackage("gh:Dobiasd/FunctionalPlus#v0.2.24")
CPMAddPackage("gh:nlohmann/json@3.11.3")
CPMAddPackage("gh:gabime/spdlog@1.15.3")
CPMAddPackage(NAME raylib   URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz)
CPMAddPackage(NAME expected GITHUB_REPOSITORY nyyakko/expected GIT_TAG master OPTIONS "ENABLE_CPPCHECK OFF")
CPMAddPackage(NAME LibError GITHUB_REPOSITORY nyyakko/LibError GIT_TAG master OPTIONS "ENABLE_CPPCHECK OFF")
CPMAddPackage(NAME LibEnum  GITHUB_REPOSITORY nyyakko/LibEnum  GIT_TAG master OPTIONS "ENABLE_CPPCHECK OFF")
CPMAddPackage(NAME LibWacom GITHUB_REPOSITORY nyyakko/LibWacom GIT_TAG master OPTIONS "ENABLE_CPPCHECK OFF")
CPMAddPackage(NAME imgui    GITHUB_REPOSITORY nyyakko/imgui    GIT_TAG master OPTIONS "ENABLE_CPPCHECK OFF")

include(cmake/static_analyzers.cmake)

set(xsetwacomgui_CompilerOptions ${xsetwacomgui_CompilerOptions} -Wno-gnu-statement-expression-from-macro-expansion)
# set(xsetwacomgui_LinkerOptions ${xsetwacomgui_LinkerOptions})

set(xsetwacomgui_ExternalLibraries
    GL
    raylib
    fmt::fmt
    LibError::LibError
    LibEnum::LibEnum
    LibWacom::LibWacom
    imgui::imgui
    spdlog::spdlog
    FunctionalPlus::fplus
    nlohmann_json::nlohmann_json
)

add_subdirectory(xsetwacomgui)

