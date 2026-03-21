# Add Feature
# SIMD-optimized element-wise vector addition

file(GLOB ADD_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(add_obj OBJECT ${ADD_SOURCES})
target_include_directories(add_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(add_obj PUBLIC dynemit_core)
set_target_properties(add_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_add STATIC $<TARGET_OBJECTS:add_obj>)
target_include_directories(dynemit_add PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dynemit_add PUBLIC dynemit_core)

include(GNUInstallDirs)
install(TARGETS dynemit_add ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/add.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

# Tests
add_executable(test_add tests/test_add.c)
target_include_directories(test_add PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_add PRIVATE dynemit_add dynemit_core unity m)
add_test(NAME test_add COMMAND test_add)

# Benchmarks
add_executable(bench_add_f32 benchmarks/bench_add_f32.c)
target_include_directories(bench_add_f32 PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_add_f32 PRIVATE dynemit_add dynemit_core m)
