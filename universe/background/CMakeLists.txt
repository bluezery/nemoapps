SET(APPNAME "nemouniverse-background")
ADD_DEFINITIONS(-DAPPNAME="${APPNAME}")

ADD_EXECUTABLE(${APPNAME}
    background.c
    $<TARGET_OBJECTS:sound>
    )
ADD_DEPENDENCIES(${APPNAME} nemoutil nemowidget)

TARGET_LINK_LIBRARIES(${APPNAME}
    nemoutil
    nemowidget
    ${NEMOAPPS_COMMON_LIBRARIES}
	)

INSTALL(TARGETS ${APPNAME}
    DESTINATION bin
    )
