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

# Tests
add_executable(test_gini tests/test_gini.c)
target_include_directories(test_gini PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_gini PRIVATE dynemit_gini dynemit_core unity m)
add_test(NAME test_gini COMMAND test_gini)

# Benchmarks
add_executable(bench_gini_f64 benchmarks/bench_gini_f64.c)
target_include_directories(bench_gini_f64 PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_gini_f64 PRIVATE dynemit_gini dynemit_core m)
