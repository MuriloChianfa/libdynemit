# Histogram Feature
# SIMD-optimized range-based element counting
file(GLOB HISTOGRAM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(histogram_obj OBJECT ${HISTOGRAM_SOURCES})
target_include_directories(histogram_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(histogram_obj PUBLIC dynemit_core)
set_target_properties(histogram_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_histogram STATIC $<TARGET_OBJECTS:histogram_obj>)
target_include_directories(dynemit_histogram PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dynemit_histogram PUBLIC dynemit_core)

include(GNUInstallDirs)
install(TARGETS dynemit_histogram ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/histogram.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

dynemit_add_feature_test(histogram FAULT_ALLOC)
dynemit_add_feature_bench(histogram u16)
