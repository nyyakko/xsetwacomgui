add_subdirectory(udev)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(xsetwacomgui_HeaderFiles ${xsetwacomgui_HeaderFiles}
    "${DIR}/Display.hpp"
    "${DIR}/Environment.hpp"

    PARENT_SCOPE
)

