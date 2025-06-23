set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(xsetwacomgui_SourceFiles ${xsetwacomgui_SourceFiles}
    "${DIR}/Environment.cpp"
    "${DIR}/Localisation.cpp"
    "${DIR}/Main.cpp"
    "${DIR}/Monitor.cpp"
    "${DIR}/Settings.cpp"
    "${DIR}/Widgets.cpp"
    "${DIR}/USBListener.cpp"

    PARENT_SCOPE
)

