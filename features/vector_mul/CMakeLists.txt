# Vector Multiply Feature
# SIMD-optimized vector multiplication implementation

# Object library for bundling into all-in-one library
add_library(vector_mul_obj OBJECT 
    vector_mul.c
)

target_include_directories(vector_mul_obj 
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(vector_mul_obj PUBLIC dynemit_core)

# Set position independent code for use in shared libraries
set_target_properties(vector_mul_obj PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# Individual static library
add_library(dynemit_vector_mul STATIC 
    $<TARGET_OBJECTS:vector_mul_obj>
)

target_include_directories(dynemit_vector_mul 
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(dynemit_vector_mul PUBLIC dynemit_core)

# Installation
include(GNUInstallDirs)

install(TARGETS dynemit_vector_mul
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/vector_mul.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit
)
