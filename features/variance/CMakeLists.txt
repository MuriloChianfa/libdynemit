# Variance Feature
# SIMD-optimized sample variance computation

file(GLOB VARIANCE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(variance_obj OBJECT ${VARIANCE_SOURCES})
target_include_directories(variance_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(variance_obj PUBLIC dynemit_core)
set_target_properties(variance_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_variance STATIC $<TARGET_OBJECTS:variance_obj>)
target_include_directories(dynemit_variance PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dynemit_variance PUBLIC dynemit_core dynemit_mean)

include(GNUInstallDirs)
install(TARGETS dynemit_variance ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/variance.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

# Tests
add_executable(test_variance tests/test_variance.c)
target_include_directories(test_variance PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_variance PRIVATE dynemit_variance dynemit_mean dynemit_core unity m)
add_test(NAME test_variance COMMAND test_variance)

# Benchmarks
add_executable(bench_variance_f64 benchmarks/bench_variance_f64.c)
target_include_directories(bench_variance_f64 PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_variance_f64 PRIVATE dynemit_variance dynemit_mean dynemit_core m)
