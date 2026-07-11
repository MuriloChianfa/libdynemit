# Log2 Feature
# SIMD-optimized vectorized log2 computation

file(GLOB LOG2_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(log2_obj OBJECT ${LOG2_SOURCES})
target_include_directories(log2_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(log2_obj PUBLIC dynemit_core)
set_target_properties(log2_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_log2 STATIC $<TARGET_OBJECTS:log2_obj>)
target_include_directories(dynemit_log2 PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dynemit_log2 PUBLIC dynemit_core)

include(GNUInstallDirs)
install(TARGETS dynemit_log2 ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/log2.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

# Tests
add_executable(test_log2 tests/test_log2.c)
target_include_directories(test_log2 PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(test_log2 PRIVATE dynemit_log2 dynemit_core unity m)
add_test(NAME test_log2 COMMAND test_log2)

# Benchmarks
add_executable(bench_log2_f64 benchmarks/bench_log2_f64.c)
target_include_directories(bench_log2_f64 PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_log2_f64 PRIVATE dynemit_log2 dynemit_core m)
