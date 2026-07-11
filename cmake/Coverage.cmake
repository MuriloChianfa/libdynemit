option(DYNEMIT_COVERAGE "Build with code coverage instrumentation (requires GCC + lcov)" OFF)
set(DYNEMIT_COVERAGE_TEST_WRAPPER ""
    CACHE STRING
    "Optional emulator wrapper for coverage tests (e.g. 'sde64 -spr --'). Empty uses ctest.")

if(DYNEMIT_COVERAGE)
    if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "Coverage requires GCC (for gcov). Found: ${CMAKE_C_COMPILER_ID}")
    endif()

    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)

    find_program(LCOV lcov REQUIRED)
    find_program(GENHTML genhtml REQUIRED)

    set(LCOV_IGNORE --ignore-errors inconsistent,inconsistent
                    --ignore-errors mismatch,mismatch
                    --ignore-errors negative,negative
                    --ignore-errors unused,unused)

    set(_coverage_run_tests
        ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} --output-on-failure)
    if(DYNEMIT_COVERAGE_TEST_WRAPPER)
        set(_coverage_run_tests
            ${CMAKE_COMMAND} -E env
                "DYNEMIT_TEST_WRAPPER=${DYNEMIT_COVERAGE_TEST_WRAPPER}"
            ${CMAKE_SOURCE_DIR}/scripts/run_tests_under_emu.sh
            --build-dir ${CMAKE_BINARY_DIR})
        message(STATUS "Coverage test wrapper: ${DYNEMIT_COVERAGE_TEST_WRAPPER}")
    endif()

    add_custom_target(coverage
        COMMAND ${LCOV} --zerocounters --directory ${CMAKE_BINARY_DIR}
        COMMAND ${_coverage_run_tests}
        COMMAND ${LCOV} --capture --directory ${CMAKE_BINARY_DIR}
                --output-file coverage_raw.info
                --rc branch_coverage=1
                ${LCOV_IGNORE}
        COMMAND ${LCOV} --extract coverage_raw.info
                "*/src/*.c"
                "*/features/*/*.c"
                --output-file coverage_extracted.info
                --rc branch_coverage=1
                ${LCOV_IGNORE}
        COMMAND ${LCOV} --remove coverage_extracted.info
                "*/features/*/tests/*"
                "*/features/*/benchmarks/*"
                "*/_deps/*"
                --output-file coverage.info
                --rc branch_coverage=1
                ${LCOV_IGNORE}
        COMMAND ${GENHTML} coverage.info
                --output-directory coverage_report
                --branch-coverage
                ${LCOV_IGNORE}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running tests and generating coverage report in build/coverage_report/"
    )
endif()
