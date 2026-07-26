# Hill Feature
# Heavy-tail detection via Hill estimator

file(GLOB HILL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(hill_obj OBJECT ${HILL_SOURCES})
target_include_directories(hill_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(hill_obj PUBLIC dynemit_core)
set_target_properties(hill_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_hill STATIC $<TARGET_OBJECTS:hill_obj>)
target_include_directories(dynemit_hill PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dynemit_hill PUBLIC dynemit_core)

include(GNUInstallDirs)
install(TARGETS dynemit_hill ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/hill.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

dynemit_add_feature_test(hill)
dynemit_add_feature_bench(hill f64)
