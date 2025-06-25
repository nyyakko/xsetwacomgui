add_subdirectory(core)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(xsetwacomgui_HeaderFiles ${xsetwacomgui_HeaderFiles}
    "${DIR}/ApplicationSettings.hpp"
    "${DIR}/TabletSettings.hpp"

    PARENT_SCOPE
)

