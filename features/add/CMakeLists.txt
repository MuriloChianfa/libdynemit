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

dynemit_add_feature_test(add)
dynemit_add_feature_bench(add f32)
