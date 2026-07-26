# Gini Coefficient Feature
file(GLOB GINI_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(gini_obj OBJECT ${GINI_SOURCES})

target_include_directories(gini_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(gini_obj PUBLIC dynemit_core)

set_target_properties(gini_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_gini STATIC $<TARGET_OBJECTS:gini_obj>)

target_include_directories(dynemit_gini PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(dynemit_gini PUBLIC dynemit_core)

include(GNUInstallDirs)

install(TARGETS dynemit_gini ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/gini.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit
)

dynemit_add_feature_test(gini)
dynemit_add_feature_bench(gini f64)
