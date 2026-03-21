# Skewness Feature
# SIMD-optimized skewness computation

file(GLOB SKEWNESS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(skewness_obj OBJECT ${SKEWNESS_SOURCES})
target_include_directories(skewness_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(skewness_obj PUBLIC dynemit_core)
set_target_properties(skewness_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_skewness STATIC $<TARGET_OBJECTS:skewness_obj>)
target_include_directories(dynemit_skewness PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dynemit_skewness PUBLIC dynemit_core dynemit_mean)

include(GNUInstallDirs)
install(TARGETS dynemit_skewness ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/skewness.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

# Tests
add_executable(test_skewness tests/test_skewness.c)
target_include_directories(test_skewness PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_skewness PRIVATE dynemit_skewness dynemit_mean dynemit_core unity m)
add_test(NAME test_skewness COMMAND test_skewness)

# Benchmarks
add_executable(bench_skewness_f64 benchmarks/bench_skewness_f64.c)
target_include_directories(bench_skewness_f64 PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_skewness_f64 PRIVATE dynemit_skewness dynemit_mean dynemit_core m)
