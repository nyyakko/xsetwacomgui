add_subdirectory(widgets)
add_subdirectory(windows)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(xsetwacomgui_HeaderFiles ${xsetwacomgui_HeaderFiles}
    "${DIR}/Scaling.hpp"
    "${DIR}/Localisation.hpp"
    "${DIR}/FreeType.hpp"

    PARENT_SCOPE
)

