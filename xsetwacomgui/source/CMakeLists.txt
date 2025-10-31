add_subdirectory(app)
add_subdirectory(platform)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(xsetwacomgui_SourceFiles ${xsetwacomgui_SourceFiles}
    "${DIR}/Main.cpp"

    PARENT_SCOPE
)

