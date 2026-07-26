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

dynemit_add_feature_test(radixs FAULT_ALLOC)
foreach(t u16 u32 u64)
    dynemit_add_feature_bench(radixs ${t})
endforeach()
