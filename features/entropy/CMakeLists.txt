# Entropy Feature
# SIMD-optimized Shannon entropy computation

file(GLOB ENTROPY_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(entropy_obj OBJECT ${ENTROPY_SOURCES})
target_include_directories(entropy_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(entropy_obj PUBLIC dynemit_core)
set_target_properties(entropy_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_entropy STATIC $<TARGET_OBJECTS:entropy_obj>)
target_include_directories(dynemit_entropy PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dynemit_entropy PUBLIC dynemit_core)

include(GNUInstallDirs)
install(TARGETS dynemit_entropy ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/entropy.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

# Tests
add_executable(test_entropy tests/test_entropy.c)
target_include_directories(test_entropy PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_entropy PRIVATE dynemit_entropy dynemit_core unity m)
add_test(NAME test_entropy COMMAND test_entropy)

# Benchmarks
add_executable(bench_entropy benchmarks/bench_entropy.c)
target_include_directories(bench_entropy PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(bench_entropy PRIVATE dynemit_entropy dynemit_core m)
