# Vector Add Feature
# SIMD-optimized vector addition implementation

# Object library for bundling into all-in-one library
add_library(vector_add_obj OBJECT 
    vector_add.c
)

target_include_directories(vector_add_obj 
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(vector_add_obj PUBLIC dynemit_core)

# Set position independent code for use in shared libraries
set_target_properties(vector_add_obj PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# Individual static library
add_library(dynemit_vector_add STATIC 
    $<TARGET_OBJECTS:vector_add_obj>
)

target_include_directories(dynemit_vector_add 
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(dynemit_vector_add PUBLIC dynemit_core)

# Installation
include(GNUInstallDirs)

install(TARGETS dynemit_vector_add
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/vector_add.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit
)
