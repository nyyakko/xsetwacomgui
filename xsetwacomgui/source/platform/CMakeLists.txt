add_subdirectory(events)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(xsetwacomgui_SourceFiles ${xsetwacomgui_SourceFiles}
    "${DIR}/Monitor.cpp"
    "${DIR}/Environment.cpp"

    PARENT_SCOPE
)

