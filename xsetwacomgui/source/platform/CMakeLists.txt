add_subdirectory(hid)
add_subdirectory(mqueue)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(xsetwacomgui_SourceFiles ${xsetwacomgui_SourceFiles}
    "${DIR}/Environment.cpp"
    "${DIR}/Daemon.cpp"
    "${DIR}/Notify.cpp"

    PARENT_SCOPE
)

