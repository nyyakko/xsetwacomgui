function(enable_cppcheck PROJECT)
    find_program(CPPCHECK cppcheck)

    if (NOT CPPCHECK)
        message(WARNING "[${PROJECT}] Couldn't find a valid ``cppcheck`` installation.")
        return()
    endif()

    if(CMAKE_GENERATOR MATCHES ".*Visual Studio.*")
        set(CPPCHECK_TEMPLATE "vs")
    else()
        set(CPPCHECK_TEMPLATE "gcc")
    endif()

    if("${CPPCHECK_OPTIONS}" STREQUAL "")
        set(CPPCHECK_OPTIONS ${CPPCHECK}
            --template=${CPPCHECK_TEMPLATE}
            --enable=style,performance,warning,portability
            --inline-suppr
            --suppress=normalCheckLevelMaxBranches
            --suppress=cppcheckError
            --suppress=internalAstError
            --suppress=unmatchedSuppression
            --suppress=passedByValue
            --suppress=syntaxError
            --suppress=preprocessorErrorDirective
            --inconclusive
            --error-exitcode=2
            --std=c++23
        )
    else()
        set(CPPCHECK_OPTIONS ${CPPCHECK} --template=${CPPCHECK_TEMPLATE} ${CPPCHECK_OPTIONS})
    endif()

    if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
        set(CPPCHECK_OPTIONS ${CPPCHECK_OPTIONS} --platform=win64)
    endif()

    set_target_properties(${PROJECT} PROPERTIES CXX_CPPCHECK "${CPPCHECK_OPTIONS}")
endfunction()
