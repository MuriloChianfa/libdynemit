# Detect musl libc. GCC rejects GNU ifunc on musl targets, so use the
# constructor-time dispatch path (DYNEMIT_NO_IFUNC) used by sanitizer builds.
execute_process(
    COMMAND ${CMAKE_C_COMPILER} -dumpmachine
    OUTPUT_VARIABLE _DYNEMIT_C_TARGET
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

set(DYNEMIT_MUSL_LIBC FALSE)
if(_DYNEMIT_C_TARGET MATCHES "musl")
    set(DYNEMIT_MUSL_LIBC TRUE)
    add_compile_definitions(DYNEMIT_NO_IFUNC=1)
    message(STATUS "musl libc detected (${_DYNEMIT_C_TARGET}): DYNEMIT_NO_IFUNC enabled")
endif()
