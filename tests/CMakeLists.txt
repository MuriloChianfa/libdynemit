# Tests for dynemit library

# Enable C++ for C++ compatibility tests
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Test 1: Feature discovery test (using all-in-one library)
add_executable(test_features test_features.c)
target_include_directories(test_features PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_features PRIVATE dynemit m)

# Test 2: Vector operations correctness test
add_executable(test_vector_ops test_vector_ops.c)
target_include_directories(test_vector_ops PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_vector_ops PRIVATE dynemit m)

# Test 3: Thread-safe SIMD detection test
add_executable(test_thread_safe_detection test_thread_safe_detection.c)
target_include_directories(test_thread_safe_detection PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_thread_safe_detection PRIVATE dynemit_core m pthread)

# Test 4: EXPLICIT_RUNTIME_RESOLVER macro test
add_executable(test_resolver_macro test_resolver_macro.c)
target_include_directories(test_resolver_macro PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_resolver_macro PRIVATE dynemit_core m)

# C++ compatibility tests
add_executable(test_cpp_basic test_cpp_basic.cpp)
target_include_directories(test_cpp_basic PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_cpp_basic PRIVATE dynemit m)

add_executable(test_cpp_features test_cpp_features.cpp)
target_include_directories(test_cpp_features PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_cpp_features PRIVATE dynemit m)

add_executable(test_cpp_resolver_macro test_cpp_resolver_macro.cpp)
target_include_directories(test_cpp_resolver_macro PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_cpp_resolver_macro PRIVATE dynemit_core m)

# Add tests
add_test(NAME test_features COMMAND test_features)
add_test(NAME test_vector_ops COMMAND test_vector_ops)
add_test(NAME test_thread_safe_detection COMMAND test_thread_safe_detection)
add_test(NAME test_resolver_macro COMMAND test_resolver_macro)
add_test(NAME test_cpp_basic COMMAND test_cpp_basic)
add_test(NAME test_cpp_features COMMAND test_cpp_features)
add_test(NAME test_cpp_resolver_macro COMMAND test_cpp_resolver_macro)
