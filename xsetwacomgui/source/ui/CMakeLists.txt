add_subdirectory(widgets)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(xsetwacomgui_SourceFiles ${xsetwacomgui_SourceFiles}
    "${DIR}/Localisation.cpp"
    "${DIR}/Scaling.cpp"

    PARENT_SCOPE
)

