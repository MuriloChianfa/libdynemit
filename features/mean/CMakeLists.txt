# Mean Feature
# SIMD-optimized mean computation

file(GLOB MEAN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(mean_obj OBJECT ${MEAN_SOURCES})
target_include_directories(mean_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(mean_obj PUBLIC dynemit_core)
set_target_properties(mean_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_mean STATIC $<TARGET_OBJECTS:mean_obj>)
target_include_directories(dynemit_mean PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dynemit_mean PUBLIC dynemit_core dynemit_sum)

include(GNUInstallDirs)
install(TARGETS dynemit_mean ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/mean.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

dynemit_add_feature_test(mean)
dynemit_add_feature_bench(mean f64)
