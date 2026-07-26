# Kurtosis Feature
# SIMD-optimized excess kurtosis computation

file(GLOB KURTOSIS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(kurtosis_obj OBJECT ${KURTOSIS_SOURCES})
target_include_directories(kurtosis_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(kurtosis_obj PUBLIC dynemit_core)
set_target_properties(kurtosis_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_kurtosis STATIC $<TARGET_OBJECTS:kurtosis_obj>)
target_include_directories(dynemit_kurtosis PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dynemit_kurtosis PUBLIC dynemit_core dynemit_mean)

include(GNUInstallDirs)
install(TARGETS dynemit_kurtosis ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/kurtosis.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

dynemit_add_feature_test(kurtosis LIBS dynemit_mean)
dynemit_add_feature_bench(kurtosis f64 LIBS dynemit_mean)
