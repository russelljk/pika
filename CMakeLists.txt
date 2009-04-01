cmake_minimum_required(VERSION 2.6)
project(Pika)

# Redirect the libraries and executables to the same directory.

SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
     CACHE PATH                     "Directory for all Libraries")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
     CACHE PATH                     "Directory for all Executables.")

add_subdirectory(pikac)
add_subdirectory(libpika)
add_subdirectory(re)
add_subdirectory(curses)
