# Simpson's Diversity Index Feature

file(GLOB SIMPSON_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

# Object library for bundling into all-in-one library
add_library(simpson_obj OBJECT ${SIMPSON_SOURCES})

target_include_directories(simpson_obj
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(simpson_obj PUBLIC dynemit_core)

set_target_properties(simpson_obj PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# Individual static library
add_library(dynemit_simpson STATIC
    $<TARGET_OBJECTS:simpson_obj>
)

target_include_directories(dynemit_simpson
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(dynemit_simpson PUBLIC dynemit_core)

# Installation
include(GNUInstallDirs)

install(TARGETS dynemit_simpson
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/simpson.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit
)

# Tests
add_executable(test_simpson tests/test_simpson.c ${PROJECT_SOURCE_DIR}/tests/fault_alloc.c)
target_include_directories(test_simpson PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(test_simpson PRIVATE dynemit_simpson dynemit_core unity m)
target_link_options(test_simpson PRIVATE
    -Wl,--wrap=aligned_alloc -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
add_test(NAME test_simpson COMMAND test_simpson)
set_tests_properties(test_simpson PROPERTIES WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# Benchmarks
add_executable(bench_simpson_u16 benchmarks/bench_simpson_u16.c)
target_include_directories(bench_simpson_u16 PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_simpson_u16 PRIVATE dynemit_simpson dynemit_core m)
