# Herfindahl-Hirschman Index (HHI) Feature

file(GLOB HHI_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

# Object library for bundling into all-in-one library
add_library(hhi_obj OBJECT ${HHI_SOURCES})

target_include_directories(hhi_obj
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(hhi_obj PUBLIC dynemit_core)

set_target_properties(hhi_obj PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# Individual static library
add_library(dynemit_hhi STATIC
    $<TARGET_OBJECTS:hhi_obj>
)

target_include_directories(dynemit_hhi
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(dynemit_hhi PUBLIC dynemit_core)

# Installation
include(GNUInstallDirs)

install(TARGETS dynemit_hhi
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/hhi.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit
)

# Tests
add_executable(test_hhi tests/test_hhi.c)
target_include_directories(test_hhi PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_hhi PRIVATE dynemit_hhi dynemit_core unity m)
add_test(NAME test_hhi COMMAND test_hhi)

# Benchmarks
add_executable(bench_hhi benchmarks/bench_hhi.c)
target_include_directories(bench_hhi PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_hhi PRIVATE dynemit_hhi dynemit_core m)
