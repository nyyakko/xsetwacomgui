add_subdirectory(ipc)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(xsetwacomgui_SourceFiles ${xsetwacomgui_SourceFiles}
    "${DIR}/Context.cpp"
    "${DIR}/Help.cpp"

    PARENT_SCOPE
)

