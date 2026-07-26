# Max Feature
# SIMD-optimized maximum reductions

file(GLOB MAX_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(max_obj OBJECT ${MAX_SOURCES})
target_include_directories(max_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(max_obj PUBLIC dynemit_core)
set_target_properties(max_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_max STATIC $<TARGET_OBJECTS:max_obj>)
target_include_directories(dynemit_max PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dynemit_max PUBLIC dynemit_core)

include(GNUInstallDirs)
install(TARGETS dynemit_max ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/max.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

dynemit_add_feature_test(max)
dynemit_add_feature_bench(max f64)
dynemit_add_feature_bench(max u32)
