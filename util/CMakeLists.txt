SET(LIBNAME "nemoutil")
ADD_LIBRARY(${LIBNAME} SHARED
    util.h
    util-common.c
    util-color.h
    util-log.h
    util-list.h
    util-file.c
    util-file.h
    util-image.cpp
    util-image.h
    util-xml.c
    util-xml.h
    util-config.c
    util-config.h
    util-dbus.c
    util-dbus.h
    util-svg.c
    util-svg.h
    util-con.c
    util-con.h
    )

TARGET_LINK_LIBRARIES(${LIBNAME}
    ${NEMOAPPS_COMMON_LIBRARIES}
	)

CONFIGURE_FILE(${LIBNAME}.pc.in ${LIBNAME}.pc @ONLY)
INSTALL(TARGETS ${LIBNAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
INSTALL(FILES
    util.h
    util-common.h
    util-color.h
    util-log.h
    util-list.h
    util-file.h
    util-image.h
    util-xml.h
    util-config.h
    util-dbus.h
    util-svg.h
    util-con.h
    DESTINATION ${CMAKE_INSTALL_INCDIR}/${LIBNAME}
    )
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${LIBNAME}.pc
    DESTINATION ${CMAKE_INSTALL_PKGDIR}
    )
