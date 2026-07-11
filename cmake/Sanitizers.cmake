set(DYNEMIT_SANITIZE "" CACHE STRING
    "Sanitizer set: empty | address,undefined | thread")

if(DYNEMIT_SANITIZE)
    if(DYNEMIT_COVERAGE)
        message(FATAL_ERROR
            "DYNEMIT_SANITIZE and DYNEMIT_COVERAGE cannot be enabled together")
    endif()
    if(DYNEMIT_MULL)
        message(FATAL_ERROR
            "DYNEMIT_SANITIZE and DYNEMIT_MULL cannot be enabled together")
    endif()

    set(_DYNEMIT_SANITIZE_ALLOWED address,undefined thread)
    if(NOT DYNEMIT_SANITIZE IN_LIST _DYNEMIT_SANITIZE_ALLOWED)
        message(FATAL_ERROR
            "DYNEMIT_SANITIZE must be one of: ${_DYNEMIT_SANITIZE_ALLOWED}. "
            "Found: ${DYNEMIT_SANITIZE}")
    endif()

    add_compile_options(-fsanitize=${DYNEMIT_SANITIZE} -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=${DYNEMIT_SANITIZE})
    add_compile_definitions(DYNEMIT_NO_IFUNC=1)
    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mno-fmv -Wno-unused-command-line-argument")
    endif()

    message(STATUS "Sanitizers enabled: ${DYNEMIT_SANITIZE}")
endif()
