# Min Feature
# SIMD-optimized minimum reductions

file(GLOB MIN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(min_obj OBJECT ${MIN_SOURCES})
target_include_directories(min_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(min_obj PUBLIC dynemit_core)
set_target_properties(min_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_min STATIC $<TARGET_OBJECTS:min_obj>)
target_include_directories(dynemit_min PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dynemit_min PUBLIC dynemit_core)

include(GNUInstallDirs)
install(TARGETS dynemit_min ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/min.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

# Tests
add_executable(test_min tests/test_min.c)
target_include_directories(test_min PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_min PRIVATE dynemit_min dynemit_core unity m)
add_test(NAME test_min COMMAND test_min)

# Benchmarks
add_executable(bench_min benchmarks/bench_min.c)
target_include_directories(bench_min PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_min PRIVATE dynemit_min dynemit_core m)
