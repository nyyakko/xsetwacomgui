function(find_xsetwacom)
    find_program(XSETWACOM_EXECUTABLE xsetwacom)

    if (NOT XSETWACOM_EXECUTABLE)
        message(FATAL_ERROR "xsetwacom could not be found. try installing the wacom driver before proceeding, also be advised that this program is intended to be run under X11.")
    else()
        message(STATUS "found xsetwacom: ${XSETWACOM_EXECUTABLE}")
    endif()
endfunction()
