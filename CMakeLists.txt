
cmake_minimum_required(VERSION 2.6)
project(Pika)

if (NOT CMAKE_INSTALL_PREFIX)
    SET(CMAKE_INSTALL_PREFIX "/usr/local")
endif (NOT CMAKE_INSTALL_PREFIX)

# -------------------------------------------------------------------------
# Set version numbers
# -------------------------------------------------------------------------

set (pika_LIB_VERSION_MAJOR "1")
set (pika_LIB_VERSION_MINOR "0")
set (pika_LIB_VERSION_PATCH "2")
set (pika_ABI_VERSION "1")

if (APPLE AND PIKA_FAT_BINARY)
    # set arch to 32/64 bit ppc & x86
    
    set (CMAKE_OSX_ARCHITECTURES "i386;x86_64;")
    
    # set gcc version the 4.0. Otherwise we can't use 10.4u sdk
    
    set (PIKA_CXX_COMPILER   "/usr/bin/c++-4.0")
    set (CMAKE_CXX_COMPILER  ${PIKA_CXX_COMPILER})
    set (CMAKE_GENERATOR_CXX ${PIKA_CXX_COMPILER})
    
    set (PIKA_CC_COMPILER    "/usr/bin/gcc-4.0")
    set (CMAKE_C_COMPILER    ${PIKA_CC_COMPILER})
    set (CMAKE_GENERATOR_CC  ${PIKA_CC_COMPILER})
    set (CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "4.0")
    
    # set the sdk version to 10.5
    
    set (CMAKE_C_FLAGS     "${CMAKE_C_FLAGS}   -mmacosx-version-min=10.5")
    set (CMAKE_CXX_FLAGS   "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.5")    
    set (CMAKE_OSX_SYSROOT "/Developer/SDKs/MacOSX10.5.sdk/")
endif (APPLE AND PIKA_FAT_BINARY)
# INSTALL_NAME_DIR "${LIB_DIR}"

# CMake module path 

set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Set build type to for single-configuration generators (ie Makefiles) to 'Release'.

if (NOT CMAKE_BUILD_TYPE)
   set (CMAKE_BUILD_TYPE "Release")
endif (NOT CMAKE_BUILD_TYPE)

# -------------------------------------------------------------
# Redirect the libraries and executables to the same directory.
# -------------------------------------------------------------

SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
     CACHE PATH                     "Directory for all Libraries")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
     CACHE PATH                     "Directory for all Executables.")

# ------------------------------------------------
# Initialize CPack variables
# ------------------------------------------------

# We want to (optionally) add the path to the bin directory.
SET(CPACK_NSIS_MODIFY_PATH ON)

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Pika")
SET(CPACK_PACKAGE_VENDOR              "Pika")

# CPack complains when building a .dmg if the resource files have
# no extension. This way we just 'copy' them into the build
# directory with a txt extension.

configure_file ("${CMAKE_SOURCE_DIR}/README" 
                "${CMAKE_CURRENT_BINARY_DIR}/README.txt")

configure_file ("${CMAKE_SOURCE_DIR}/LICENSE" 
                "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt")

configure_file ("${CMAKE_SOURCE_DIR}/CPackWelcome" 
                "${CMAKE_CURRENT_BINARY_DIR}/CPackWelcome.txt")

SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_BINARY_DIR}/README.txt")
SET(CPACK_RESOURCE_FILE_LICENSE    "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt")
SET(CPACK_RESOURCE_FILE_README     "${CMAKE_CURRENT_BINARY_DIR}/README.txt")
SET(CPACK_RESOURCE_FILE_WELCOME    "${CMAKE_CURRENT_BINARY_DIR}/CPackWelcome.txt")

SET(CPACK_NSIS_HELP_LINK      "http://www.pika-lang.org")
SET(CPACK_NSIS_URL_INFO_ABOUT "http://www.pika-lang.org")
SET(CPACK_NSIS_CONTACT        "russell.j.kyle@gmail.com")


# -------------------------------------
# Add the subdirectorys for each target
# -------------------------------------

if (PIKA_JUST_LIB)
    add_subdirectory(libpika)
    add_subdirectory(samples)
else (PIKA_JUST_LIB)
    add_subdirectory(pikac)
    add_subdirectory(libpika)        
    add_subdirectory(samples)
    if (NOT PIKA_NO_MODULES)
    	add_subdirectory(modules)
    endif (NOT PIKA_NO_MODULES)
endif (PIKA_JUST_LIB)
