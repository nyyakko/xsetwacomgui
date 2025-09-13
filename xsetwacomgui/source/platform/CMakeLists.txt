add_subdirectory(hid)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(xsetwacomgui_SourceFiles ${xsetwacomgui_SourceFiles}
    "${DIR}/Environment.cpp"
    "${DIR}/Daemon.cpp"

    PARENT_SCOPE
)

