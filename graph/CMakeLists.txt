SET(LIBNAME "nemodavi")
ADD_LIBRARY(${LIBNAME} SHARED
	nemodavi.c
	nemodavi_data.c
	nemodavi_selector.c
	nemodavi_attr.c
	nemodavi_transition.c
	nemodavi_layout.c
	nemodavi_item.c
	layouts/histogram.c
	layouts/donut.c
	layouts/donutbar.c
	layouts/donutarray.c
	layouts/arc.c
	layouts/area.c
	layouts/test.c
)

CONFIGURE_FILE(${LIBNAME}.pc.in ${LIBNAME}.pc @ONLY)
INSTALL(TARGETS ${LIBNAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES 
    nemodavi.h
    nemodavi_attr.h
    nemodavi_data.h
    nemodavi_item.h
    nemodavi_layout.h
    nemodavi_selector.h
    nemodavi_transition.h
    DESTINATION ${CMAKE_INSTALL_INCDIR}/${LIBNAME}
    )
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${LIBNAME}.pc
    DESTINATION ${CMAKE_INSTALL_PKGDIR}
    )

#ADD_SUBDIRECTORY(tests)
