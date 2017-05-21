SET(LIBNAME "xemoutil")

FIND_PATH(MAGIC_INCLUDE_DIR NAMES magic.h)
FIND_LIBRARY(MAGIC_LIBRARY NAMES libmagic.so)
INCLUDE_DIRECTORIES(${MAGIC_INCLUDE_DIR})

PKG_CHECK_MODULES(NEMOUTIL REQUIRED expat json-c dbus-1 libcurl fontconfig)
INCLUDE_DIRECTORIES(${NEMOUTIL_INCLUDE_DIRS})
LINK_DIRECTORIES(${NEMOUTIL_LIBRARY_DIRS})

ADD_LIBRARY(${LIBNAME} SHARED
    xemoutil.c
    xemoutil.h
    xemoutil-common.c
    xemoutil-color.h
    xemoutil-log.h
    xemoutil-list.h
    xemoutil-file.c
    xemoutil-file.h
    xemoutil-image.cpp
    xemoutil-image.h
    xemoutil-xml.c
    xemoutil-xml.h
    xemoutil-config.c
    xemoutil-config.h
    xemoutil-dbus.c
    xemoutil-dbus.h
    xemoutil-svg.c
    xemoutil-svg.h
    xemoutil-con.c
    xemoutil-con.h
    xemoutil-path.c
    xemoutil-path.h
    xemoutil-json.c
    xemoutil-json.h
    xemoutil-string.c
    xemoutil-string.h
    xemoutil-font.c
    xemoutil-font.h
    )
TARGET_LINK_LIBRARIES(${LIBNAME}
    ${NEMOAPPS_COMMON_LIBRARIES}
    ${NEMOUTIL_LIBRARIES}
    ${MAGIC_LIBRARY}
	)

INSTALL(TARGETS ${LIBNAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
INSTALL(FILES
    xemoutil.h
    xemoutil-common.h
    xemoutil-color.h
    xemoutil-log.h
    xemoutil-list.h
    xemoutil-file.h
    xemoutil-image.h
    xemoutil-xml.h
    xemoutil-config.h
    xemoutil-dbus.h
    xemoutil-svg.h
    xemoutil-con.h
    xemoutil-path.h
    xemoutil-json.h
    xemoutil-string.h
    xemoutil-font.h
    DESTINATION ${CMAKE_INSTALL_INCDIR}/${LIBNAME}
    )
CONFIGURE_FILE(${LIBNAME}.pc.in ${LIBNAME}.pc @ONLY)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${LIBNAME}.pc
    DESTINATION ${CMAKE_INSTALL_PKGDIR}
    )
