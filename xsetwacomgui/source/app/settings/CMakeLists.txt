add_subdirectory(tablet)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(xsetwacomgui_SourceFiles ${xsetwacomgui_SourceFiles}
    "${DIR}/ApplicationSettings.cpp"

    PARENT_SCOPE
)

