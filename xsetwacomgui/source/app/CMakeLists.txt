add_subdirectory(core)
add_subdirectory(settings)
add_subdirectory(ui)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(xsetwacomgui_SourceFiles ${xsetwacomgui_SourceFiles}
    PARENT_SCOPE
)

