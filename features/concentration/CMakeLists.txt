# Concentration Feature (Composite)
# Calls topk_ratios, hill_estimator, and hhi

file(GLOB CONCENTRATION_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(concentration_obj OBJECT ${CONCENTRATION_SOURCES})
target_include_directories(concentration_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(concentration_obj PUBLIC dynemit_core)
set_target_properties(concentration_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_concentration STATIC $<TARGET_OBJECTS:concentration_obj>)
target_include_directories(dynemit_concentration PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dynemit_concentration PUBLIC dynemit_core dynemit_topk dynemit_hill dynemit_hhi)

include(GNUInstallDirs)
install(TARGETS dynemit_concentration ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/concentration.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

# Tests
add_executable(test_concentration tests/test_concentration.c)
target_include_directories(test_concentration PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_concentration PRIVATE dynemit_concentration dynemit_topk dynemit_hill dynemit_hhi dynemit_core unity m)
add_test(NAME test_concentration COMMAND test_concentration)

# Benchmarks
add_executable(bench_concentration_f64 benchmarks/bench_concentration_f64.c)
target_include_directories(bench_concentration_f64 PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_concentration_f64 PRIVATE dynemit_concentration dynemit_topk dynemit_hill dynemit_hhi dynemit_core m)
