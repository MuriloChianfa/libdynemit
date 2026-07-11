# Entropy Feature
# SIMD-optimized Shannon entropy computation

file(GLOB ENTROPY_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(entropy_obj OBJECT ${ENTROPY_SOURCES})
target_include_directories(entropy_obj PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/features/log2)
if(DYNEMIT_TS)
    target_link_libraries(entropy_obj PUBLIC dynemit_core Threads::Threads)
else()
    target_link_libraries(entropy_obj PUBLIC dynemit_core)
endif()
set_target_properties(entropy_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_entropy STATIC $<TARGET_OBJECTS:entropy_obj>)
target_include_directories(dynemit_entropy PUBLIC ${PROJECT_SOURCE_DIR}/include)
if(DYNEMIT_TS)
    target_link_libraries(dynemit_entropy PUBLIC dynemit_core Threads::Threads)
else()
    target_link_libraries(dynemit_entropy PUBLIC dynemit_core)
endif()

include(GNUInstallDirs)
install(TARGETS dynemit_entropy ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/entropy.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

# Tests
add_executable(test_entropy tests/test_entropy.c ${PROJECT_SOURCE_DIR}/tests/fault_alloc.c)
target_include_directories(test_entropy PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(test_entropy PRIVATE dynemit_entropy dynemit_core unity m pthread)
target_link_options(test_entropy PRIVATE
    -Wl,--wrap=aligned_alloc -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
add_test(NAME test_entropy COMMAND test_entropy)

if(DYNEMIT_TS)
    add_executable(test_entropy_ts ${PROJECT_SOURCE_DIR}/tests/test_entropy_ts.c
        ${PROJECT_SOURCE_DIR}/tests/fault_alloc.c)
    target_include_directories(test_entropy_ts PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/tests)
    target_link_libraries(test_entropy_ts PRIVATE dynemit_entropy dynemit_core unity m pthread)
    target_link_options(test_entropy_ts PRIVATE
        -Wl,--wrap=aligned_alloc -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc)
    add_test(NAME test_entropy_ts COMMAND test_entropy_ts)
endif()

# Benchmarks
add_executable(bench_entropy_u16 benchmarks/bench_entropy_u16.c)
target_include_directories(bench_entropy_u16 PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_entropy_u16 PRIVATE dynemit_entropy dynemit_core m)

add_executable(bench_entropy_u32 benchmarks/bench_entropy_u32.c)
target_include_directories(bench_entropy_u32 PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_entropy_u32 PRIVATE dynemit_entropy dynemit_core m)

add_executable(bench_entropy_histogram benchmarks/bench_entropy_histogram.c)
target_include_directories(bench_entropy_histogram PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_entropy_histogram PRIVATE dynemit_entropy dynemit_core m)
