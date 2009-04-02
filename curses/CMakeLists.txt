message (STATUS "********* Starting pikacurses library *********")

# TODO: support pdcurses
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

if (CURSES_FOUND)
    # echo what we found.
    message (STATUS  ${CURSES_INCLUDE_DIR})
    message (STATUS  ${CURSES_LIBRARIES})
    
    set(HAVE_CURSES 1)
    
    # Add the headers file path.
    include_directories ( ${CURSES_INCLUDE_DIR} )
    
    # Add the library path.
    set (ADD_LIBS ${ADD_LIBS} ${CURSES_LIBRARIES})
endif (CURSES_FOUND)

# --------------------- Source + Header Files ----------------------

if (HAVE_CURSES)

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
    target_link_libraries (pikacurses ${ADD_LIBS} pika)
else (HAVE_CURSES)
   message (STATUS "Curses library not found.")
endif (HAVE_CURSES) 
