cmake_minimum_required(VERSION 3.16)
project(simd-dynamic-dispatch
    VERSION 1.0.0
    DESCRIPTION "SIMD Dynamic Dispatch Library"
    LANGUAGES C
)

# Option to list available features
option(LIST_FEATURES "List available SIMD features and exit" OFF)

if(LIST_FEATURES)
    message(STATUS "")
    message(STATUS "=== Available Dynemit Features ===")
    message(STATUS "  - core              (CPU detection and SIMD level API)")
    message(STATUS "  - vector_add        (SIMD-optimized vector addition)")
    message(STATUS "  - vector_mul        (SIMD-optimized vector multiplication)")
    message(STATUS "  - vector_sub        (SIMD-optimized vector subtraction)")
    message(STATUS "===================================")
    message(STATUS "")
    message(STATUS "Usage modes:")
    message(STATUS "  1. All-in-one:    Link with -ldynemit (includes all features)")
    message(STATUS "  2. Modular:       Link with -ldynemit_core -ldynemit_<feature>")
    message(STATUS "  3. Core only:     Link with -ldynemit_core")
    message(STATUS "")
    message(FATAL_ERROR "Feature list displayed. Exiting as requested.")
endif()

# Set C standard
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Require GCC compiler
if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "This project requires GCC compiler. Found: ${CMAKE_C_COMPILER_ID}")
endif()

# Require GCC 13+ for C23 support
if(CMAKE_C_COMPILER_VERSION VERSION_LESS "13.0")
    message(FATAL_ERROR "GCC 13 or newer is required for C23 support. Found: ${CMAKE_C_COMPILER_VERSION}")
endif()

# Default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_RELEASE "-O3")

# Check for x86 architecture (required for SIMD intrinsics)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i686|i386")
    set(X86_ARCH TRUE)
else()
    set(X86_ARCH FALSE)
    message(WARNING "Non-x86 architecture detected. SIMD optimizations will be limited.")
endif()

# Add subdirectories for core and features
add_subdirectory(src)
add_subdirectory(features/vector_add)
add_subdirectory(features/vector_mul)
add_subdirectory(features/vector_sub)
add_subdirectory(bench)

# Enable testing
enable_testing()
add_subdirectory(tests)

# Create all-in-one library combining core + all features
add_library(dynemit STATIC
    $<TARGET_OBJECTS:dynemit_core_obj>
    $<TARGET_OBJECTS:vector_add_obj>
    $<TARGET_OBJECTS:vector_mul_obj>
    $<TARGET_OBJECTS:vector_sub_obj>
    src/dynemit_features.c  # Feature list for all-in-one library
)

target_include_directories(dynemit 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(dynemit PUBLIC m)

# Define DYNEMIT_ALL_FEATURES for users of the all-in-one library
target_compile_definitions(dynemit PUBLIC DYNEMIT_ALL_FEATURES)

# Installation rules
include(GNUInstallDirs)

install(TARGETS dynemit
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES include/dynemit.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

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
message(STATUS "Install prefix:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "============================================")
message(STATUS "")
