add_subdirectory(external)
add_subdirectory(platform)
add_subdirectory(settings)
add_subdirectory(ui)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(xsetwacomgui_HeaderFiles ${xsetwacomgui_HeaderFiles}
    PARENT_SCOPE
)

