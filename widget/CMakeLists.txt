SET(LIBNAME "nemowidget")
ADD_LIBRARY(${LIBNAME} SHARED
    widget.h
    widget.c
    widgets.h
    button.c
    frame.c
    bar.c
    #bars.c
    dim.c
    )
ADD_DEPENDENCIES(${LIBNAME} xemoutil nemoui)
TARGET_LINK_LIBRARIES(${LIBNAME}
    xemoutil nemoui
    ${NEMOAPPS_COMMON_LIBRARIES}
	)

CONFIGURE_FILE(${LIBNAME}.pc.in ${LIBNAME}.pc @ONLY)
INSTALL(TARGETS ${LIBNAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
INSTALL(FILES widget.h widgets.h
    DESTINATION ${CMAKE_INSTALL_INCDIR}/${LIBNAME}
    )
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${LIBNAME}.pc
    DESTINATION ${CMAKE_INSTALL_PKGDIR}
    )
