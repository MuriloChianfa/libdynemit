# Sum Feature
# SIMD-optimized sum reductions

file(GLOB SUM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(sum_obj OBJECT ${SUM_SOURCES})
target_include_directories(sum_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(sum_obj PUBLIC dynemit_core)
set_target_properties(sum_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_sum STATIC $<TARGET_OBJECTS:sum_obj>)
target_include_directories(dynemit_sum PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dynemit_sum PUBLIC dynemit_core)

include(GNUInstallDirs)
install(TARGETS dynemit_sum ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/sum.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

# Tests
add_executable(test_sum tests/test_sum.c)
target_include_directories(test_sum PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_sum PRIVATE dynemit_sum dynemit_core unity m)
add_test(NAME test_sum COMMAND test_sum)

# Benchmarks
add_executable(bench_sum_f64 benchmarks/bench_sum_f64.c)
target_include_directories(bench_sum_f64 PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_sum_f64 PRIVATE dynemit_sum dynemit_core m)
