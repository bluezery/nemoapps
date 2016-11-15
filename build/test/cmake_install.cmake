# Install script for directory: /home/root/Work/nemoapps/test

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

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/root/Work/nemoapps/build/test/button/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/test/frame/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/test/main/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/test/bar/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/test/pie/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/test/pies/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/test/animation/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/test/map/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/test/spinfx/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/test/gradient/cmake_install.cmake")
  include("/home/root/Work/nemoapps/build/test/plexus/cmake_install.cmake")

endif()

