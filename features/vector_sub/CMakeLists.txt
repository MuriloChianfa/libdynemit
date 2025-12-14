# Vector Sub Feature
# SIMD-optimized vector subtraction implementation

# Object library for bundling into all-in-one library
add_library(vector_sub_obj OBJECT 
    vector_sub.c
)

target_include_directories(vector_sub_obj 
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(vector_sub_obj PUBLIC dynemit_core)

# Individual static library
add_library(dynemit_vector_sub STATIC 
    $<TARGET_OBJECTS:vector_sub_obj>
)

target_include_directories(dynemit_vector_sub 
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(dynemit_vector_sub PUBLIC dynemit_core)

# Installation
include(GNUInstallDirs)

install(TARGETS dynemit_vector_sub
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/vector_sub.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit
)
