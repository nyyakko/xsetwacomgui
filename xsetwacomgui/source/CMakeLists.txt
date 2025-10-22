add_subdirectory(core)
add_subdirectory(platform)
add_subdirectory(settings)
add_subdirectory(ui)
add_subdirectory(cli)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(xsetwacomgui_SourceFiles ${xsetwacomgui_SourceFiles}
    "${DIR}/Main.cpp"
    "${DIR}/MainWindow.cpp"
    "${DIR}/SettingsWindow.cpp"
    "${DIR}/MappingsWindow.cpp"
    "${DIR}/GoddessWindow.cpp"
    "${DIR}/ProfileWindow.cpp"

    PARENT_SCOPE
)

