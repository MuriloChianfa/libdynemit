cmake_minimum_required(VERSION 3.16)
project(simd-dynamic-dispatch
    VERSION 1.2.0
    DESCRIPTION "SIMD Dynamic Dispatch Library"
    LANGUAGES C
)

# Optional build modes and cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

include(ListFeatures)

# Export compile_commands.json for clangd / IDE tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C standard
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(CompilerChecks)
include(LibcChecks)

option(DYNEMIT_TS "Thread-safe scratch TLS" OFF)

if(DYNEMIT_TS)
    find_package(Threads REQUIRED)
endif()

# Default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wconversion -Werror")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_RELEASE "-O3")

# Detect target architecture for SIMD intrinsics
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i686|i386")
    set(X86_ARCH TRUE)
    set(ARM_ARCH FALSE)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
    set(X86_ARCH FALSE)
    set(ARM_ARCH TRUE)
else()
    set(X86_ARCH FALSE)
    set(ARM_ARCH FALSE)
    message(WARNING "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}. Only scalar fallback will be used.")
endif()

include(Coverage)
include(Mull)
include(Sanitizers)
include(Unity)

option(DYNEMIT_ENABLE_GTEST "Build Google Test C++ compatibility tests" ON)
if(DYNEMIT_ENABLE_GTEST)
    include(GTest)
endif()

# Add subdirectories for core and features
add_subdirectory(src)

# Enable testing before features (feature subdirectories define their own tests)
enable_testing()

# Automatically discover and add all feature subdirectories
file(GLOB FEATURE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/features/*")
set(FEATURE_OBJECTS "")
foreach(FEATURE_DIR ${FEATURE_DIRS})
    if(IS_DIRECTORY ${FEATURE_DIR})
        get_filename_component(FEATURE_NAME ${FEATURE_DIR} NAME)
        add_subdirectory(features/${FEATURE_NAME})
        target_compile_options(${FEATURE_NAME}_obj PRIVATE -falign-functions=64)
        target_include_directories(${FEATURE_NAME}_obj PRIVATE ${PROJECT_SOURCE_DIR}/src)
        list(APPEND FEATURE_OBJECTS $<TARGET_OBJECTS:${FEATURE_NAME}_obj>)
    endif()
endforeach()

if(TARGET entropy_obj AND TARGET hll_obj)
    if(DYNEMIT_TS)
        target_compile_definitions(entropy_obj PRIVATE DYNEMIT_TS=1)
        target_compile_definitions(hll_obj PRIVATE DYNEMIT_TS=1)
    else()
        target_compile_definitions(entropy_obj PRIVATE DYNEMIT_TS=0)
        target_compile_definitions(hll_obj PRIVATE DYNEMIT_TS=0)
    endif()
endif()

# Optionally statically link all bench_* targets for portable bundles
option(DYNEMIT_STATIC_BENCHMARKS "Statically link benchmarks" OFF)
if(DYNEMIT_STATIC_BENCHMARKS)
    foreach(FEATURE_DIR ${FEATURE_DIRS})
        get_filename_component(FNAME ${FEATURE_DIR} NAME)
        file(GLOB BENCH_SRCS "${FEATURE_DIR}/benchmarks/bench_${FNAME}_*.c")
        foreach(BENCH_SRC ${BENCH_SRCS})
            get_filename_component(BENCH_NAME ${BENCH_SRC} NAME_WE)
            if(TARGET ${BENCH_NAME})
                target_link_options(${BENCH_NAME} PRIVATE -static)
            endif()
        endforeach()
    endforeach()
    message(STATUS "Static benchmarks:  ON")
endif()

option(DYNEMIT_BUILD_TESTS "Build core library tests" ON)
if(DYNEMIT_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Create all-in-one STATIC library combining core + all features
add_library(dynemit_static STATIC
    $<TARGET_OBJECTS:dynemit_core_obj>
    ${FEATURE_OBJECTS}
    src/dynemit_features.c
)

set_target_properties(dynemit_static PROPERTIES
    OUTPUT_NAME dynemit
)

target_include_directories(dynemit_static 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(DYNEMIT_TS)
    target_link_libraries(dynemit_static PUBLIC m Threads::Threads)
else()
    target_link_libraries(dynemit_static PUBLIC m)
endif()

target_compile_definitions(dynemit_static PUBLIC DYNEMIT_ALL_FEATURES)

# Create all-in-one SHARED library combining core + all features
add_library(dynemit_shared SHARED
    $<TARGET_OBJECTS:dynemit_core_obj>
    ${FEATURE_OBJECTS}
    src/dynemit_features.c
)

set_target_properties(dynemit_shared PROPERTIES
    OUTPUT_NAME dynemit
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

target_include_directories(dynemit_shared 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(DYNEMIT_TS)
    target_link_libraries(dynemit_shared PUBLIC m Threads::Threads)
else()
    target_link_libraries(dynemit_shared PUBLIC m)
endif()

target_compile_definitions(dynemit_shared PUBLIC DYNEMIT_ALL_FEATURES)

# Create dynemit alias target for backward compatibility (points to static)
add_library(dynemit ALIAS dynemit_static)

include(Install)

# Print configuration summary
message(STATUS "")
message(STATUS "=== simd-dynamic-dispatch Configuration ===")
message(STATUS "C Standard:        C${CMAKE_C_STANDARD}")
message(STATUS "Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler:        ${CMAKE_C_COMPILER}")
message(STATUS "C Compiler ID:     ${CMAKE_C_COMPILER_ID}")
message(STATUS "C Compiler Ver:    ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "Architecture:      ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "x86 SIMD Support:  ${X86_ARCH}")
message(STATUS "ARM SIMD Support:  ${ARM_ARCH}")
message(STATUS "Thread-safe (DYNEMIT_TS): ${DYNEMIT_TS}")
message(STATUS "musl libc:         ${DYNEMIT_MUSL_LIBC}")
message(STATUS "Install prefix:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "============================================")
message(STATUS "")
