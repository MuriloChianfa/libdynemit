# Benchmark - Vector Multiply
# Performance benchmark for vector multiplication

add_executable(benchmark_vector_mul 
    benchmark_vector_mul.c
)

target_include_directories(benchmark_vector_mul 
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(benchmark_vector_mul 
    PRIVATE 
        dynemit_vector_mul
        dynemit_core
)

# Static linking for portability across different glibc versions
target_link_options(benchmark_vector_mul 
    PRIVATE 
        -static
)
