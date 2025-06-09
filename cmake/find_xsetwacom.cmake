function(find_xsetwacom)
    find_program(XSETWACOM_EXECUTABLE xsetwacom)

    if (NOT XSETWACOM_EXECUTABLE)
        message(FATAL_ERROR "xsetwacom could not be found. make sure you have the wacom driver installed in your machine.")
    else()
        message(STATUS "found xsetwacom: ${XSETWACOM_EXECUTABLE}")
    endif()
endfunction()
