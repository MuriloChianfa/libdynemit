# Core Dynemit Library
# CPU feature detection and SIMD level detection

# Object library for bundling into all-in-one library
add_library(dynemit_core_obj OBJECT 
    dynemit.c
)

target_include_directories(dynemit_core_obj 
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/include
)

# Set position independent code for use in shared libraries
set_target_properties(dynemit_core_obj PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# Individual static library
add_library(dynemit_core STATIC 
    $<TARGET_OBJECTS:dynemit_core_obj>
)

target_include_directories(dynemit_core 
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/include
)

# Link math library if needed
target_link_libraries(dynemit_core PUBLIC m)

# Installation
include(GNUInstallDirs)

install(TARGETS dynemit_core
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES 
    ${PROJECT_SOURCE_DIR}/include/dynemit/core.h
    ${PROJECT_SOURCE_DIR}/include/dynemit/err.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit
)
