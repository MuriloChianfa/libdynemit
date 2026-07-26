# Variance Feature
# SIMD-optimized sample variance computation

file(GLOB VARIANCE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(variance_obj OBJECT ${VARIANCE_SOURCES})
target_include_directories(variance_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(variance_obj PUBLIC dynemit_core)
set_target_properties(variance_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_variance STATIC $<TARGET_OBJECTS:variance_obj>)
target_include_directories(dynemit_variance PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dynemit_variance PUBLIC dynemit_core dynemit_mean)

include(GNUInstallDirs)
install(TARGETS dynemit_variance ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/variance.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

dynemit_add_feature_test(variance LIBS dynemit_mean)
dynemit_add_feature_bench(variance f64 LIBS dynemit_mean)
