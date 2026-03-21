# Max Feature
# SIMD-optimized maximum reductions

file(GLOB MAX_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(max_obj OBJECT ${MAX_SOURCES})
target_include_directories(max_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(max_obj PUBLIC dynemit_core)
set_target_properties(max_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_max STATIC $<TARGET_OBJECTS:max_obj>)
target_include_directories(dynemit_max PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dynemit_max PUBLIC dynemit_core)

include(GNUInstallDirs)
install(TARGETS dynemit_max ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/max.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

# Tests
add_executable(test_max tests/test_max.c)
target_include_directories(test_max PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_max PRIVATE dynemit_max dynemit_core unity m)
add_test(NAME test_max COMMAND test_max)

# Benchmarks
add_executable(bench_max_f64 benchmarks/bench_max_f64.c)
target_include_directories(bench_max_f64 PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_max_f64 PRIVATE dynemit_max dynemit_core m)

add_executable(bench_max_u32 benchmarks/bench_max_u32.c)
target_include_directories(bench_max_u32 PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_max_u32 PRIVATE dynemit_max dynemit_core m)
