# Tests for dynemit library

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

# Add tests
add_test(NAME test_features COMMAND test_features)
add_test(NAME test_vector_ops COMMAND test_vector_ops)
add_test(NAME test_thread_safe_detection COMMAND test_thread_safe_detection)
add_test(NAME test_resolver_macro COMMAND test_resolver_macro)
