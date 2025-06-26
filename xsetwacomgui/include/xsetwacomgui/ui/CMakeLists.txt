add_subdirectory(widgets)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(xsetwacomgui_HeaderFiles ${xsetwacomgui_HeaderFiles}
    "${DIR}/Scaling.hpp"
    "${DIR}/Localisation.hpp"
    "${DIR}/FreeType.hpp"

    PARENT_SCOPE
)

