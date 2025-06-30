add_subdirectory(core)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(xsetwacomgui_HeaderFiles ${xsetwacomgui_HeaderFiles}
    "${DIR}/SettingsWindow.hpp"
    "${DIR}/MainWindow.hpp"
    "${DIR}/GoddessWindow.hpp"

    PARENT_SCOPE
)

