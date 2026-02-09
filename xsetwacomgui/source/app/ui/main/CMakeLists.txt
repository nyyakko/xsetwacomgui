add_subdirectory(components)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(xsetwacomgui_SourceFiles ${xsetwacomgui_SourceFiles}
    "${DIR}/MainWindow.cpp"

    PARENT_SCOPE
)

