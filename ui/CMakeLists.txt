SET(LIBNAME "nemoui")
ADD_DEFINITIONS(-DLIBNAME="${LIBNAME}")

PKG_CHECK_MODULES(NEMOPLAY REQUIRED nemoplay)
INCLUDE_DIRECTORIES(${NEMOPLAY_INCLUDE_DIRS})
LINK_DIRECTORIES(${NEMOPLAY_LIBRARY_DIRS})

ADD_LIBRARY(${LIBNAME} SHARED
    nemoui-player.c
    nemoui-player.h
    nemoui-misc.h
    nemoui-graph.h
    nemoui-image.cpp
    nemoui-image.h
    nemoui-thread.c
    nemoui-thread.h
    nemoui-path.c
    nemoui-path.h
    nemoui-gesture.c
    nemoui-gesture.h
    nemoui-drawer.c
    nemoui-drawer.h
    nemoui-button.c
    nemoui-button.h
    nemoui.h
    )

TARGET_LINK_LIBRARIES(${LIBNAME}
    ${NEMOPLAY_LIBRARIES}
    ${NEMOAPPS_COMMON_LIBRARIES}
	)

CONFIGURE_FILE(${LIBNAME}.pc.in ${LIBNAME}.pc @ONLY)
INSTALL(TARGETS ${LIBNAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
INSTALL(FILES
    nemoui-player.h
    nemoui-misc.h
    nemoui-graph.h
    nemoui-image.h
    nemoui-thread.h
    nemoui-path.h
    nemoui-gesture.h
    nemoui.h
    DESTINATION ${CMAKE_INSTALL_INCDIR}/${LIBNAME}
    )
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${LIBNAME}.pc
    DESTINATION ${CMAKE_INSTALL_PKGDIR}
    )

SET(RES_DIR "${DATA_DIR}/ui/res")
ADD_DEFINITIONS(-DRES_DIR="${RES_DIR}")
INSTALL(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/res/
    DESTINATION ${RES_DIR}/
    )
