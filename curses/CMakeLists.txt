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

# ----------------------------- CURSES ------------------------------

include (FindCurses)

if (CURSES_FOUND)
    # echo what we found.
    message (STATUS  ${CURSES_INCLUDE_DIR})
    message (STATUS  ${CURSES_LIBRARIES})
    set(HAVE_CURSES 1)
    include_directories ( ${CURSES_INCLUDE_DIR} )
    set (ADD_LIBS ${ADD_LIBS} ${CURSES_LIBRARIES})
endif (CURSES_FOUND)

# --------------------- Source + Header Files ----------------------

if (NOT HAVE_CURSES)
    message (FATAL "No curses library found.")
endif (NOT HAVE_CURSES) 

set (curses_LIB_SRCS PCurses.cpp)
set (curses_LIB_HEADERS PCurses.h)

source_group (include FILES ${curses_LIB_HEADERS})
source_group (source  FILES ${curses_LIB_SRCS})

include_directories (${Pika_SOURCE_DIR}/libpika)
link_directories (${Pika_BINARY_DIR}/libpika)

add_library (pikacurses SHARED ${curses_LIB_SRCS} ${curses_LIB_HEADERS})
target_link_libraries (pikacurses ${ADD_LIBS} pika)
