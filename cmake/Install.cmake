include(GNUInstallDirs)

install(TARGETS dynemit_static dynemit_shared
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Development
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Runtime
    NAMELINK_COMPONENT Development
)

install(FILES include/dynemit.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT Development
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/libdynemit.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/libdynemit.pc
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libdynemit.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    COMPONENT Development
)
