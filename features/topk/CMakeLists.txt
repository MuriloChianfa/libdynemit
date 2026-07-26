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

dynemit_add_feature_test(topk)
dynemit_add_feature_bench(topk f64)
