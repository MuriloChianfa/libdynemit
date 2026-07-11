# Tests for dynemit library

# Enable C++ for C++ compatibility tests
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Test 1: Feature discovery test (using all-in-one library)
add_executable(test_features test_features.c)
target_include_directories(test_features PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_features PRIVATE dynemit unity m)

# Bounded memory helpers (src/mem.h)
add_executable(test_mem test_mem.c)
target_include_directories(test_mem PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(test_mem PRIVATE unity m)

# Test 2: Thread-safe SIMD detection test
add_executable(test_thread_safe_detection test_thread_safe_detection.c)
target_include_directories(test_thread_safe_detection PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_thread_safe_detection PRIVATE dynemit_core unity m pthread)

# Test 4: EXPLICIT_RUNTIME_RESOLVER macro test
add_executable(test_resolver_macro test_resolver_macro.c)
target_include_directories(test_resolver_macro PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_resolver_macro PRIVATE dynemit_core unity m)

# Google Test C++ tests
include(GoogleTest)

add_executable(test_cpp_basic test_cpp_basic.cpp)
target_include_directories(test_cpp_basic PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_cpp_basic PRIVATE dynemit GTest::gtest_main m)

add_executable(test_cpp_features test_cpp_features.cpp)
target_include_directories(test_cpp_features PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_cpp_features PRIVATE dynemit GTest::gtest_main m)

add_executable(test_cpp_resolver_macro test_cpp_resolver_macro.cpp)
target_include_directories(test_cpp_resolver_macro PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_cpp_resolver_macro PRIVATE dynemit_core GTest::gtest_main m)

# C test registration
add_test(NAME test_features COMMAND test_features)
add_test(NAME test_mem COMMAND test_mem)
add_test(NAME test_thread_safe_detection COMMAND test_thread_safe_detection)
add_test(NAME test_resolver_macro COMMAND test_resolver_macro)

# GTest auto-discovery
if(DYNEMIT_SANITIZE)
    set(_gtest_discovery_mode PRE_TEST)
else()
    set(_gtest_discovery_mode POST_BUILD)
endif()

gtest_discover_tests(test_cpp_basic DISCOVERY_MODE ${_gtest_discovery_mode})
gtest_discover_tests(test_cpp_features DISCOVERY_MODE ${_gtest_discovery_mode})
gtest_discover_tests(test_cpp_resolver_macro DISCOVERY_MODE ${_gtest_discovery_mode})
