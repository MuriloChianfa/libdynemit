# Entropy Feature
# SIMD-optimized Shannon entropy computation

file(GLOB ENTROPY_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(entropy_obj OBJECT ${ENTROPY_SOURCES})
target_include_directories(entropy_obj PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/features/log2)
if(DYNEMIT_TS)
    target_link_libraries(entropy_obj PUBLIC dynemit_core Threads::Threads)
else()
    target_link_libraries(entropy_obj PUBLIC dynemit_core)
endif()
set_target_properties(entropy_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_entropy STATIC $<TARGET_OBJECTS:entropy_obj>)
target_include_directories(dynemit_entropy PUBLIC ${PROJECT_SOURCE_DIR}/include)
if(DYNEMIT_TS)
    target_link_libraries(dynemit_entropy PUBLIC dynemit_core Threads::Threads)
else()
    target_link_libraries(dynemit_entropy PUBLIC dynemit_core)
endif()

include(GNUInstallDirs)
install(TARGETS dynemit_entropy ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/entropy.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

dynemit_add_feature_test(entropy FAULT_ALLOC PTHREAD)
if(DYNEMIT_TS)
    dynemit_add_feature_test(entropy
        NAME test_entropy_ts
        SOURCES ${PROJECT_SOURCE_DIR}/tests/test_entropy_ts.c
        FAULT_ALLOC
        PTHREAD
    )
endif()
dynemit_add_feature_bench(entropy u16)
dynemit_add_feature_bench(entropy u32)
dynemit_add_feature_bench(entropy histogram)
