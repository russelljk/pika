#
# CMakeLists.txt for pikaRegExp
# TODO: Install library and headers
#
message (STATUS "********* Starting pikaRegExp library *********")

# Disable 'unsecure' warnings in VC++

if (WIN32)
    if (MSVC)
        add_definitions(-D_SCL_SECURE_NO_WARNINGS)
        add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    endif (MSVC)
endif (WIN32)

# -------------------- Look for regexp libs -----------------------

set (ADD_LIBS "")

# ----------------------------- PCRE ------------------------------

include (FindPCRE)

if (PCRE_FOUND)
    # echo what we found.
    message (STATUS  ${PCRE_INCLUDE_DIR})
    message (STATUS  ${PCRE_LIBRARIES})

    include_directories ( ${PCRE_INCLUDE_DIR} )
    set (HAVE_PCRE 1)
    set (ADD_LIBS ${ADD_LIBS} ${PCRE_LIBRARIES})
endif (PCRE_FOUND)

# ----------------------------- POSIX ------------------------------

include (CheckFunctionExists)

message (STATUS  "checking for Posix re functions.")
check_function_exists ("regcomp;regerror;regexec;regfree" HAVE_POSIX_RE)

# --------------------- Source + Header Files ----------------------

if (NOT HAVE_POSIX_RE AND NOT HAVE_PCRE)
    message (STATUS "no regular expression library found.")
    return ()
endif (NOT HAVE_POSIX_RE AND NOT HAVE_PCRE)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake-re_config.h.in 
               ${CMAKE_CURRENT_SOURCE_DIR}/re_config.h)

set (re_LIB_SRCS PlatRE.cpp PRegExp.cpp)
set (re_LIB_HEADERS PlatRE.h PRegExp.h ${CMAKE_CURRENT_SOURCE_DIR}/re_config.h)

source_group (include FILES ${re_LIB_HEADERS})
source_group (source  FILES ${re_LIB_SRCS})

include_directories (${Pika_SOURCE_DIR}/libpika)

link_directories (${Pika_BINARY_DIR}/libpika)

add_library (pikaRegExp SHARED ${re_LIB_SRCS} ${re_LIB_HEADERS})
target_link_libraries (pikaRegExp ${ADD_LIBS} pika)
