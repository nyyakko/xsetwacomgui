add_subdirectory(platform)
add_subdirectory(settings)
add_subdirectory(ui)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(xsetwacomgui_SourceFiles ${xsetwacomgui_SourceFiles}
    "${DIR}/Main.cpp"

    PARENT_SCOPE
)

