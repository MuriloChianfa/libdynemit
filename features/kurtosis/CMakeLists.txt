# Kurtosis Feature
# SIMD-optimized excess kurtosis computation

file(GLOB KURTOSIS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(kurtosis_obj OBJECT ${KURTOSIS_SOURCES})
target_include_directories(kurtosis_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(kurtosis_obj PUBLIC dynemit_core)
set_target_properties(kurtosis_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_kurtosis STATIC $<TARGET_OBJECTS:kurtosis_obj>)
target_include_directories(dynemit_kurtosis PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dynemit_kurtosis PUBLIC dynemit_core dynemit_mean)

include(GNUInstallDirs)
install(TARGETS dynemit_kurtosis ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/kurtosis.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

# Tests
add_executable(test_kurtosis tests/test_kurtosis.c)
target_include_directories(test_kurtosis PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_kurtosis PRIVATE dynemit_kurtosis dynemit_mean dynemit_core unity m)
add_test(NAME test_kurtosis COMMAND test_kurtosis)

# Benchmarks
add_executable(bench_kurtosis_f64 benchmarks/bench_kurtosis_f64.c)
target_include_directories(bench_kurtosis_f64 PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_kurtosis_f64 PRIVATE dynemit_kurtosis dynemit_mean dynemit_core m)
