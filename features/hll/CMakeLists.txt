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

dynemit_add_feature_test(hll FAULT_ALLOC)
dynemit_add_feature_bench(hll u32)
dynemit_add_feature_bench(hll u64)
