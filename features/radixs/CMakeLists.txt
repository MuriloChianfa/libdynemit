# Radix Sort Feature
# SIMD-optimized LSD radix sort (u32, u64) and counting sort (u16)

file(GLOB RADIXS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(radixs_obj OBJECT ${RADIXS_SOURCES})
target_include_directories(radixs_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(radixs_obj PUBLIC dynemit_core)
set_target_properties(radixs_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_radixs STATIC $<TARGET_OBJECTS:radixs_obj>)
target_include_directories(dynemit_radixs PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dynemit_radixs PUBLIC dynemit_core)

include(GNUInstallDirs)
install(TARGETS dynemit_radixs ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/radixs.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

# Tests
add_executable(test_radixs tests/test_radixs.c)
target_include_directories(test_radixs PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_radixs PRIVATE dynemit_radixs dynemit_core unity m)
add_test(NAME test_radixs COMMAND test_radixs)

# Benchmarks
foreach(t u16 u32 u64)
  add_executable(bench_radixs_${t} benchmarks/bench_radixs_${t}.c)
  target_include_directories(bench_radixs_${t} PRIVATE
      ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
  target_link_libraries(bench_radixs_${t} PRIVATE dynemit_radixs dynemit_core m)
endforeach()
