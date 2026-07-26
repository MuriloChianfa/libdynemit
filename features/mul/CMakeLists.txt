# Mul Feature
# SIMD-optimized element-wise vector multiplication

file(GLOB MUL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(mul_obj OBJECT ${MUL_SOURCES})
target_include_directories(mul_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(mul_obj PUBLIC dynemit_core)
set_target_properties(mul_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_mul STATIC $<TARGET_OBJECTS:mul_obj>)
target_include_directories(dynemit_mul PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dynemit_mul PUBLIC dynemit_core)

include(GNUInstallDirs)
install(TARGETS dynemit_mul ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/mul.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

dynemit_add_feature_test(mul)
dynemit_add_feature_bench(mul f32)
