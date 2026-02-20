# Top-K Feature
# SIMD-optimized top-K concentration ratio computation

file(GLOB TOPK_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(topk_obj OBJECT ${TOPK_SOURCES})
target_include_directories(topk_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(topk_obj PUBLIC dynemit_core)
set_target_properties(topk_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_topk STATIC $<TARGET_OBJECTS:topk_obj>)
target_include_directories(dynemit_topk PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dynemit_topk PUBLIC dynemit_core)

include(GNUInstallDirs)
install(TARGETS dynemit_topk ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/topk.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

# Tests
add_executable(test_topk tests/test_topk.c)
target_include_directories(test_topk PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_topk PRIVATE dynemit_topk dynemit_core unity m)
add_test(NAME test_topk COMMAND test_topk)

# Benchmarks
add_executable(bench_topk benchmarks/bench_topk.c)
target_include_directories(bench_topk PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_topk PRIVATE dynemit_topk dynemit_core m)
