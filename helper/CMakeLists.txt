#ADD_LIBRARY(gsthelper OBJECT
#    gsthelper.c
#    gsthelper.h
#    )
#
#ADD_LIBRARY(glhelper OBJECT
#    glhelper.c
#    glhelper.h
#    )
#
ADD_LIBRARY(meshhelper OBJECT
    meshhelper.cpp
    meshhelper.h
    )

PKG_CHECK_MODULES(SKIA REQUIRED skia)
INCLUDE_DIRECTORIES(${SKIA_INCLUDE_DIRS})
LINK_DIRECTORIES(${SKIA_LIBRARY_DIRS})

PKG_CHECK_MODULES(DBUS REQUIRED dbus-1)
INCLUDE_DIRECTORIES(${DBUS_INCLUDE_DIRS})
LINK_DIRECTORIES(${DBUS_LIBRARY_DIRS})


ADD_LIBRARY(skiahelper OBJECT
    skiahelper.cpp
    skiahelper.h
    skiahelper.hpp
    )
ADD_DEPENDENCIES(skiahelper xemoutil)
#ADD_LIBRARY(oshelper OBJECT
#    oshelper.c
#    oshelper.h
#    )

ADD_LIBRARY(fbohelper OBJECT
    fbohelper.c
    fbohelper.h
    )

#ADD_LIBRARY(talehelper OBJECT
#    talehelper.c
#    talehelper.h
#    )
