# Install script for directory: /home/root/Work/nemoapps

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/etc/nemoapps/base.conf")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/etc/nemoapps" TYPE FILE FILES "/home/root/Work/nemoapps/build/base.conf")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/root/Work/nemoapps/build/icons/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/images/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/helper/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/util/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/widget/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/graph/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/sound/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/test/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/clock/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/clock2/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/stat/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/text/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/weather/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/3d/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/explorer/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/explorer2/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/browser/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/player/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/pdf/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/image/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/keyboard/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/card/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/background/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/udevd/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/connmand/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/demo/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/remote/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/wall/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/root/Work/nemoapps/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
