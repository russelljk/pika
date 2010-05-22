cmake_minimum_required(VERSION 2.6)
project(Pika)

#set (CMAKE_GENERATOR_CC "/usr/bin/gcc-4.0")
#set (CMAKE_GENERATOR_CXX "/usr/bin/c++-4.0")
#set (CMAKE_OSX_ARCHITECTURES "ppc;i386;x86_64;ppc64;")
#set (CMAKE_OSX_SYSROOT "/Developer/SDKs/MacOSX10.4u.sdk/")
#export MACOSX_DEPLOYMENT_TARGET=10.4
#message (STATUS ${CMAKE_CXX_COMPILER})


# set build type to for single-configuration generators (ie Makefiles)
if (NOT CMAKE_BUILD_TYPE)
   set (CMAKE_BUILD_TYPE "Release")
endif (NOT CMAKE_BUILD_TYPE)

# Redirect the libraries and executables to the same directory.

SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
     CACHE PATH                     "Directory for all Libraries")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
     CACHE PATH                     "Directory for all Executables.")

add_subdirectory(pikac)
add_subdirectory(libpika)
#add_subdirectory(re)
#add_subdirectory(curses)
