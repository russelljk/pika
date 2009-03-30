cmake_minimum_required(VERSION 2.6)
project(Pika)

# Redirect the libraries and executables to the same directory.

SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
     CACHE PATH                     "Directory for all Libraries")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
     CACHE PATH                     "Directory for all Executables.")

add_subdirectory(pikac)
add_subdirectory(libpika)

if (PCRE_INCLUDE_DIR AND PCRE_PCRE_LIBRARY)
    add_subdirectory(re)
else (PCRE_INCLUDE_DIR AND PCRE_PCRE_LIBRARY)
    if (UNIX)
        add_subdirectory(re)
    endif (UNIX)
endif (PCRE_INCLUDE_DIR AND PCRE_PCRE_LIBRARY)

if (CURSES_INCLUDE_DIR AND CURSES_LIBRARIES)
    add_subdirectory(curses)
else (CURSES_INCLUDE_DIR AND CURSES_LIBRARIES)
    if (UNIX)
        add_subdirectory(curses)
    endif (UNIX)
endif (CURSES_INCLUDE_DIR AND CURSES_LIBRARIES)
