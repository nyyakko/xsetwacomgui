add_subdirectory(about)
add_subdirectory(components)
add_subdirectory(main)
add_subdirectory(mappings)
add_subdirectory(profile)
add_subdirectory(settings)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(xsetwacomgui_SourceFiles ${xsetwacomgui_SourceFiles}
    "${DIR}/FreeType.cpp"
    "${DIR}/Localisation.cpp"

    PARENT_SCOPE
)

