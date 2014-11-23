#
# CMakeLists.txt for pikacurses
# TODO: Install library and headers
# TODO: support pdcurses
#
message (STATUS "********* Starting pikacurses library *********")

# Disable 'unsecure' warnings in VC++

if (WIN32)
    if (MSVC)
        add_definitions(-D_SCL_SECURE_NO_WARNINGS)
        add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    endif (MSVC)
endif (WIN32)

# -------------------- Look for regexp libs -----------------------

set (ADD_LIBS "")

# ----------------------------- curses/ncurses library ------------------------------

if (CURSES_INCLUDE_DIR AND CURSES_LIBRARIES)
    # If curses library and include paths are already defined.
    message (STATUS  "Using the curses library provided.")
    set(CURSES_FOUND TRUE)
else (CURSES_INCLUDE_DIR AND CURSES_LIBRARIES)
    # Otherwise we have to search for them.
    include (FindCurses)
endif (CURSES_INCLUDE_DIR AND CURSES_LIBRARIES)

if (NOT CURSES_FOUND)
    message (STATUS "Curses library not found.")
    return ()
endif (NOT CURSES_FOUND)

message (STATUS  ${CURSES_INCLUDE_DIR})
message (STATUS  ${CURSES_LIBRARIES})
    
# Add the headers file path.
include_directories ( ${CURSES_INCLUDE_DIR} )
    
# Add the library path.
set (ADD_LIBS ${ADD_LIBS} ${CURSES_LIBRARIES})

# -------------------------------------------------------------------------
# Set version numbers
# -------------------------------------------------------------------------

set (CPACK_PACKAGE_VERSION_MAJOR ${pika_LIB_VERSION_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR ${pika_LIB_VERSION_MINOR})
set (CPACK_PACKAGE_VERSION_PATCH ${pika_LIB_VERSION_PATCH})

set (pikacurses_LIB_VERSION "${pika_LIB_VERSION_MAJOR}.${pika_LIB_VERSION_MINOR}.${pika_LIB_VERSION_PATCH}")

# --------------------- Source + Header Files ----------------------

set (HAVE_CURSES_H          ${CURSES_HAVE_CURSES_H})
set (HAVE_NCURSES_H         ${CURSES_HAVE_NCURSES_H})
set (HAVE_NCURSES_NCURSES_H ${CURSES_HAVE_NCURSES_NCURSES_H})
set (HAVE_NCURSES_CURSES_H  ${CURSES_HAVE_NCURSES_CURSES_H})
set (HAVE_PDCURSES_H        ${CURSES_HAVE_PDCURSES_H})

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/cmake-curses_config.h.in 
                ${CMAKE_CURRENT_SOURCE_DIR}/curses_config.h)
    
set (curses_LIB_SRCS PCurses.cpp)
set (curses_LIB_HEADERS PCurses.h curses_config.h)

source_group (include FILES ${curses_LIB_HEADERS})
source_group (source  FILES ${curses_LIB_SRCS})
    
include_directories (${Pika_SOURCE_DIR}/libpika)

link_directories (${Pika_BINARY_DIR}/libpika)
    
add_library (pikacurses SHARED ${curses_LIB_SRCS} ${curses_LIB_HEADERS})

set_target_properties (pikacurses   PROPERTIES
                            	    VERSION     ${pikacurses_LIB_VERSION}
                            	    SOVERSION   ${pikacurses_LIB_VERSION})

target_link_libraries (pikacurses ${ADD_LIBS} pika)

if (APPLE)
    set_target_properties( pikacurses 
                PROPERTIES 
                MACOSX_RPATH        TRUE
                INSTALL_RPATH       "${CMAKE_INSTALL_PREFIX}/lib"
                INSTALL_NAME_DIR    "${CMAKE_INSTALL_PREFIX}/lib")
endif (APPLE)

install (TARGETS        pikacurses
         RUNTIME        DESTINATION bin
         LIBRARY        DESTINATION lib/pika
         ARCHIVE        DESTINATION lib/pika         
         FRAMEWORK      DESTINATION Library/Frameworks)

