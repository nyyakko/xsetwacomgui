add_subdirectory(components)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(xsetwacomgui_SourceFiles ${xsetwacomgui_SourceFiles}
    "${DIR}/GoddessWindow.cpp"
    "${DIR}/MainWindow.cpp"
    "${DIR}/MappingsWindow.cpp"
    "${DIR}/ProfileWindow.cpp"
    "${DIR}/SettingsWindow.cpp"

    PARENT_SCOPE
)

