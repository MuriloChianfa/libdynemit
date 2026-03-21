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

# Tests
add_executable(test_hill tests/test_hill.c)
target_include_directories(test_hill PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_hill PRIVATE dynemit_hill dynemit_core unity m)
add_test(NAME test_hill COMMAND test_hill)

# Benchmarks
add_executable(bench_hill_f64 benchmarks/bench_hill_f64.c)
target_include_directories(bench_hill_f64 PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_hill_f64 PRIVATE dynemit_hill dynemit_core m)
