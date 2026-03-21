# Sub Feature
# SIMD-optimized element-wise vector subtraction

file(GLOB SUB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(sub_obj OBJECT ${SUB_SOURCES})
target_include_directories(sub_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(sub_obj PUBLIC dynemit_core)
set_target_properties(sub_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_sub STATIC $<TARGET_OBJECTS:sub_obj>)
target_include_directories(dynemit_sub PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dynemit_sub PUBLIC dynemit_core)

include(GNUInstallDirs)
install(TARGETS dynemit_sub ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/sub.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

# Tests
add_executable(test_sub tests/test_sub.c)
target_include_directories(test_sub PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_sub PRIVATE dynemit_sub dynemit_core unity m)
add_test(NAME test_sub COMMAND test_sub)

# Benchmarks
add_executable(bench_sub_f32 benchmarks/bench_sub_f32.c)
target_include_directories(bench_sub_f32 PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_sub_f32 PRIVATE dynemit_sub dynemit_core m)
