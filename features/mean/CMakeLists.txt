# Mean Feature
# SIMD-optimized mean computation

file(GLOB MEAN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(mean_obj OBJECT ${MEAN_SOURCES})
target_include_directories(mean_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(mean_obj PUBLIC dynemit_core)
set_target_properties(mean_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_mean STATIC $<TARGET_OBJECTS:mean_obj>)
target_include_directories(dynemit_mean PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dynemit_mean PUBLIC dynemit_core dynemit_sum)

include(GNUInstallDirs)
install(TARGETS dynemit_mean ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/mean.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

# Tests
add_executable(test_mean tests/test_mean.c)
target_include_directories(test_mean PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_mean PRIVATE dynemit_mean dynemit_core unity m)
add_test(NAME test_mean COMMAND test_mean)

# Benchmarks
add_executable(bench_mean_f64 benchmarks/bench_mean_f64.c)
target_include_directories(bench_mean_f64 PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_mean_f64 PRIVATE dynemit_mean dynemit_core m)
