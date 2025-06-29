function(find_executable EXECUTABLE)
    find_program(${EXECUTABLE}_PATH ${EXECUTABLE})

    if (NOT ${EXECUTABLE}_PATH)
        if (ARGV1)
            message(FATAL_ERROR "${EXECUTABLE} could not be found.")
        endif()
    else()
        message(STATUS "Found ${EXECUTABLE}: ${${EXECUTABLE}_PATH}")
    endif()
endfunction()

