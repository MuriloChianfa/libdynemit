# Helpers for feature test and benchmark targets.
# Gates on DYNEMIT_BUILD_TESTS / DYNEMIT_BUILD_BENCHMARKS so feature
# CMakeLists.txt files do not need per-target if() checks.

# dynemit_add_feature_test(<feature>
#   [NAME <test_name>]              # default: test_<feature>
#   [SOURCES ...]                   # default: tests/<NAME>.c
#   [LIBS ...]                      # extra libs beyond dynemit_<feature> dynemit_core unity m
#   [INCLUDES ...]                  # extra include dirs beyond include/
#   [FAULT_ALLOC]                   # fault_alloc.c + allocator wraps + tests/ include
#   [PTHREAD]
#   [WORKING_DIRECTORY_BUILD]       # ctest WORKING_DIRECTORY = CMAKE_BINARY_DIR
# )
function(dynemit_add_feature_test FEATURE)
    cmake_parse_arguments(ARG
        "FAULT_ALLOC;PTHREAD;WORKING_DIRECTORY_BUILD"
        "NAME"
        "SOURCES;LIBS;INCLUDES"
        ${ARGN}
    )

    if(NOT DYNEMIT_BUILD_TESTS)
        return()
    endif()

    if(NOT ARG_NAME)
        set(ARG_NAME "test_${FEATURE}")
    endif()

    if(NOT ARG_SOURCES)
        set(ARG_SOURCES "tests/${ARG_NAME}.c")
    endif()

    if(ARG_FAULT_ALLOC)
        list(APPEND ARG_SOURCES "${PROJECT_SOURCE_DIR}/tests/fault_alloc.c")
        list(APPEND ARG_INCLUDES "${PROJECT_SOURCE_DIR}/tests")
    endif()

    add_executable(${ARG_NAME} ${ARG_SOURCES})
    target_include_directories(${ARG_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${ARG_INCLUDES}
    )

    set(_libs dynemit_${FEATURE} ${ARG_LIBS} dynemit_core unity m)
    if(ARG_PTHREAD)
        list(APPEND _libs pthread)
    endif()
    target_link_libraries(${ARG_NAME} PRIVATE ${_libs})

    if(ARG_FAULT_ALLOC)
        target_link_options(${ARG_NAME} PRIVATE
            -Wl,--wrap=aligned_alloc
            -Wl,--wrap=malloc
            -Wl,--wrap=calloc
            -Wl,--wrap=realloc
        )
    endif()

    add_test(NAME ${ARG_NAME} COMMAND ${ARG_NAME})
    if(ARG_WORKING_DIRECTORY_BUILD)
        set_tests_properties(${ARG_NAME} PROPERTIES WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    endif()
endfunction()

# dynemit_add_feature_bench(<feature> <variant>
#   [LIBS ...]                      # extra libs beyond dynemit_<feature> dynemit_core m
# )
# Creates bench_<feature>_<variant> from benchmarks/bench_<feature>_<variant>.c
function(dynemit_add_feature_bench FEATURE VARIANT)
    cmake_parse_arguments(ARG "" "" "LIBS" ${ARGN})

    if(NOT DYNEMIT_BUILD_BENCHMARKS)
        return()
    endif()

    set(_name "bench_${FEATURE}_${VARIANT}")
    add_executable(${_name} "benchmarks/${_name}.c")
    target_include_directories(${_name} PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/bench
    )
    target_link_libraries(${_name} PRIVATE
        dynemit_${FEATURE} ${ARG_LIBS} dynemit_core m
    )
endfunction()
