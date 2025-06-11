add_subdirectory(external)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(xsetwacomgui_HeaderFiles ${xsetwacomgui_HeaderFiles}
    "${DIR}/Environment.hpp"
    "${DIR}/Localisation.hpp"
    "${DIR}/Monitor.hpp"
    "${DIR}/Scaling.hpp"
    "${DIR}/Settings.hpp"
    "${DIR}/Widgets.hpp"

    PARENT_SCOPE
)

