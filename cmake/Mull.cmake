option(DYNEMIT_MULL "Enable mutation testing with Mull (requires Clang + mull)" OFF)

if(DYNEMIT_MULL)
    if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR "Mull requires Clang. Found: ${CMAKE_C_COMPILER_ID}")
    endif()

    find_program(MULL_RUNNER mull-runner
        NAMES mull-runner-20 mull-runner-19 mull-runner-18 mull-runner-17 mull-runner-16
        REQUIRED)

    find_file(MULL_FRONTEND
        NAMES mull-ir-frontend-20 mull-ir-frontend-19 mull-ir-frontend-18
              mull-ir-frontend-17 mull-ir-frontend-16
        PATHS /usr/lib /usr/local/lib
        REQUIRED)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpass-plugin=${MULL_FRONTEND} -g -grecord-command-line -O0")

    message(STATUS "Mull enabled: runner=${MULL_RUNNER}, frontend=${MULL_FRONTEND}")
endif()
