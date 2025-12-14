# Tests for dynemit library

# Test 1: Feature discovery test (using all-in-one library)
add_executable(test_features test_features.c)
target_include_directories(test_features PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_features PRIVATE dynemit m)

# Test 2: Vector operations correctness test
add_executable(test_vector_ops test_vector_ops.c)
target_include_directories(test_vector_ops PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_vector_ops PRIVATE dynemit m)

# Add tests
add_test(NAME test_features COMMAND test_features)
add_test(NAME test_vector_ops COMMAND test_vector_ops)
