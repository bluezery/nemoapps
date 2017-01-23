SET(LIBNAME "nemoutil")
ADD_LIBRARY(${LIBNAME} SHARED
    nemoutil.h
    nemoutil-common.c
    nemoutil-color.h
    nemoutil-log.h
    nemoutil-list.h
    nemoutil-file.c
    nemoutil-file.h
    nemoutil-image.cpp
    nemoutil-image.h
    nemoutil-xml.c
    nemoutil-xml.h
    nemoutil-config.c
    nemoutil-config.h
    nemoutil-dbus.c
    nemoutil-dbus.h
    nemoutil-svg.c
    nemoutil-svg.h
    nemoutil-con.c
    nemoutil-con.h
    nemoutil-path.c
    nemoutil-path.h
    nemoutil-json.h
    )

TARGET_LINK_LIBRARIES(${LIBNAME}
    ${NEMOAPPS_COMMON_LIBRARIES}
	)

CONFIGURE_FILE(${LIBNAME}.pc.in ${LIBNAME}.pc @ONLY)
INSTALL(TARGETS ${LIBNAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
INSTALL(FILES
    nemoutil.h
    nemoutil-common.h
    nemoutil-color.h
    nemoutil-log.h
    nemoutil-list.h
    nemoutil-file.h
    nemoutil-image.h
    nemoutil-xml.h
    nemoutil-config.h
    nemoutil-dbus.h
    nemoutil-svg.h
    nemoutil-con.h
    nemoutil-path.h
    nemoutil-json.h
    ../helper/nemowrapper.h
    DESTINATION ${CMAKE_INSTALL_INCDIR}/${LIBNAME}
    )
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${LIBNAME}.pc
    DESTINATION ${CMAKE_INSTALL_PKGDIR}
    )
