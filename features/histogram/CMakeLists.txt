# Histogram Feature
# SIMD-optimized range-based element counting
file(GLOB HISTOGRAM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(histogram_obj OBJECT ${HISTOGRAM_SOURCES})
target_include_directories(histogram_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(histogram_obj PUBLIC dynemit_core)
set_target_properties(histogram_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_histogram STATIC $<TARGET_OBJECTS:histogram_obj>)
target_include_directories(dynemit_histogram PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dynemit_histogram PUBLIC dynemit_core)

include(GNUInstallDirs)
install(TARGETS dynemit_histogram ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/histogram.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

# Tests
add_executable(test_histogram tests/test_histogram.c ${PROJECT_SOURCE_DIR}/tests/fault_alloc.c)
target_include_directories(test_histogram PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(test_histogram PRIVATE dynemit_histogram dynemit_core unity m)
target_link_options(test_histogram PRIVATE
    -Wl,--wrap=aligned_alloc -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
add_test(NAME test_histogram COMMAND test_histogram)

# Benchmarks
add_executable(bench_histogram_u16 benchmarks/bench_histogram_u16.c)
target_include_directories(bench_histogram_u16 PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_histogram_u16 PRIVATE dynemit_histogram dynemit_core m)
