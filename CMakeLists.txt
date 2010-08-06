
cmake_minimum_required(VERSION 2.6)
project(Pika)

if (APPLE AND PIKA_FAT_BINARY)
    # set arch to 32/64 bit ppc & x86
    set (CMAKE_OSX_ARCHITECTURES "ppc;i386;x86_64;ppc64;")
    
    # set gcc version the 4.0. Otherwise we can't use 10.4u sdk
    set (PIKA_CXX_COMPILER   "/usr/bin/c++-4.0")
    set (CMAKE_CXX_COMPILER  ${PIKA_CXX_COMPILER})
    set (CMAKE_GENERATOR_CXX ${PIKA_CXX_COMPILER})
    
    set (PIKA_CC_COMPILER    "/usr/bin/gcc-4.0")
    set (CMAKE_C_COMPILER    ${PIKA_CC_COMPILER})
    set (CMAKE_GENERATOR_CC  ${PIKA_CC_COMPILER})
    set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "4.0")
    # set the sdk version to 10.4
    set (CMAKE_C_FLAGS     "${CMAKE_C_FLAGS}   -mmacosx-version-min=10.4")
    set (CMAKE_CXX_FLAGS   "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.4")    
    set (CMAKE_OSX_SYSROOT "/Developer/SDKs/MacOSX10.4u.sdk/")
endif (APPLE AND PIKA_FAT_BINARY)

# cmake module path 
set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# set build type to for single-configuration generators (ie Makefiles)
if (NOT CMAKE_BUILD_TYPE)
   set (CMAKE_BUILD_TYPE "Release")
endif (NOT CMAKE_BUILD_TYPE)

# Redirect the libraries and executables to the same directory.

SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
     CACHE PATH                     "Directory for all Libraries")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
     CACHE PATH                     "Directory for all Executables.")

SET(CPACK_NSIS_MODIFY_PATH ON)

if (PIKA_JUST_LIB)
    add_subdirectory(libpika)
else (PIKA_JUST_LIB)
    add_subdirectory(pikac)
    add_subdirectory(libpika)
    
    if (NOT PIKA_NO_MODULES)
    	add_subdirectory(re)
        add_subdirectory(curses)
    endif (NOT PIKA_NO_MODULES)
endif (PIKA_JUST_LIB)
