add_subdirectory(widgets)
add_subdirectory(windows)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(xsetwacomgui_SourceFiles ${xsetwacomgui_SourceFiles}
    "${DIR}/Localisation.cpp"
    "${DIR}/Scaling.cpp"
    "${DIR}/FreeType.cpp"

    PARENT_SCOPE
)

