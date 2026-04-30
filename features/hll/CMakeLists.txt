# HLL (HyperLogLog++) Feature
# SIMD-optimized approximate distinct-count cardinality estimation

file(GLOB HLL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
if(NOT DYNEMIT_TS)
    list(REMOVE_ITEM HLL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/hll_tls.c")
endif()

add_library(hll_obj OBJECT ${HLL_SOURCES})
target_include_directories(hll_obj PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR})
if(DYNEMIT_TS)
    target_link_libraries(hll_obj PUBLIC dynemit_core Threads::Threads)
else()
    target_link_libraries(hll_obj PUBLIC dynemit_core)
endif()
set_target_properties(hll_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_hll STATIC $<TARGET_OBJECTS:hll_obj>)
target_include_directories(dynemit_hll PUBLIC ${PROJECT_SOURCE_DIR}/include)
if(DYNEMIT_TS)
    target_link_libraries(dynemit_hll PUBLIC dynemit_core m Threads::Threads)
else()
    target_link_libraries(dynemit_hll PUBLIC dynemit_core m)
endif()

include(GNUInstallDirs)
install(TARGETS dynemit_hll ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/hll.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

# Tests
add_executable(test_hll tests/test_hll.c)
target_include_directories(test_hll PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_hll PRIVATE dynemit_hll dynemit_core unity m)
add_test(NAME test_hll COMMAND test_hll)

# Benchmarks
add_executable(bench_hll_u32 benchmarks/bench_hll_u32.c)
target_include_directories(bench_hll_u32 PRIVATE
    ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_hll_u32 PRIVATE dynemit_hll dynemit_core m)

add_executable(bench_hll_u64 benchmarks/bench_hll_u64.c)
target_include_directories(bench_hll_u64 PRIVATE
    ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_hll_u64 PRIVATE dynemit_hll dynemit_core m)
