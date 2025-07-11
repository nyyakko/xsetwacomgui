add_subdirectory(external)
add_subdirectory(platform)
add_subdirectory(settings)
add_subdirectory(ui)
add_subdirectory(core)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(xsetwacomgui_HeaderFiles ${xsetwacomgui_HeaderFiles}
    "${DIR}/MainWindow.hpp"
    "${DIR}/SettingsWindow.hpp"
    "${DIR}/GoddessWindow.hpp"

    PARENT_SCOPE
)

