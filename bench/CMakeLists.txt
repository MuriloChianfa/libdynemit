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

# Optional static linking for portability (disabled by default for package builds)
option(DYNEMIT_STATIC_BENCHMARKS "Statically link benchmarks" OFF)
if(DYNEMIT_STATIC_BENCHMARKS)
    target_link_options(benchmark_vector_mul
        PRIVATE
            -static
    )
endif()

# Benchmark - Vector Multiply Feature Comparison
# Performance benchmark for comparing different SIMD levels

add_executable(benchmark_vector_mul_feature_compare 
    benchmark_vector_mul_feature_compare.c
)

target_include_directories(benchmark_vector_mul_feature_compare 
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(benchmark_vector_mul_feature_compare 
    PRIVATE 
        dynemit_core
        m
)

# Optional static linking for portability (disabled by default for package builds)
if(DYNEMIT_STATIC_BENCHMARKS)
    target_link_options(benchmark_vector_mul_feature_compare
        PRIVATE
            -static
    )
endif()
