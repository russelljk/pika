#
# CMakeLists.txt for pikare
# TODO: Install library and headers
#
message (STATUS "********* Starting pikare library *********")

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

if (RE_EXTERNAL_PCRE)
    include (FindPCRE)
    
    if (PCRE_FOUND)
        # echo what we found.
        message (STATUS  ${PCRE_INCLUDE_DIR})
        message (STATUS  ${PCRE_LIBRARIES})
    
        include_directories ( ${PCRE_INCLUDE_DIR} )
        set (HAVE_PCRE 1)
        set (ADD_LIBS ${ADD_LIBS} ${PCRE_LIBRARIES})
        message (STATUS "RegExp: Using external PCRE.")
    endif (PCRE_FOUND)
endif (RE_EXTERNAL_PCRE)

# ----------------------------- POSIX ------------------------------

if (RE_EXTERNAL_POSIX)
    include (CheckFunctionExists)
    
    message (STATUS  "checking for Posix re functions.")
    check_function_exists ("regcomp;regerror;regexec;regfree" HAVE_POSIX_RE)
endif(RE_EXTERNAL_POSIX)

# -------------------------------------------------------------------------
# Set version numbers
# -------------------------------------------------------------------------

set (CPACK_PACKAGE_VERSION_MAJOR ${pika_LIB_VERSION_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR ${pika_LIB_VERSION_MINOR})
set (CPACK_PACKAGE_VERSION_PATCH ${pika_LIB_VERSION_PATCH})

set (pikare_LIB_VERSION "${pika_LIB_VERSION_MAJOR}.${pika_LIB_VERSION_MINOR}.${pika_LIB_VERSION_PATCH}")

# --------------------- Source + Header Files ----------------------

if (NOT HAVE_POSIX_RE AND NOT HAVE_PCRE)
    add_subdirectory(pcre)
    include_directories (${PROJECT_BINARY_DIR})
    set (HAVE_PCRE 1)
    message (STATUS "RegExp: Using internal PCRE library.")
endif (NOT HAVE_POSIX_RE AND NOT HAVE_PCRE)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake-re_config.h.in 
               ${CMAKE_CURRENT_SOURCE_DIR}/re_config.h)

set (re_LIB_SRCS PlatRE.cpp PRegExp.cpp)
set (re_LIB_HEADERS PlatRE.h PRegExp.h ${CMAKE_CURRENT_SOURCE_DIR}/re_config.h)

source_group (include FILES ${re_LIB_HEADERS})
source_group (source  FILES ${re_LIB_SRCS})

include_directories (${Pika_SOURCE_DIR}/libpika)

link_directories (${Pika_BINARY_DIR}/libpika)

add_library (pikare SHARED ${re_LIB_SRCS} ${re_LIB_HEADERS})
target_link_libraries (pikare ${ADD_LIBS} pika)
if (NOT RE_EXTERNAL_PCRE AND HAVE_PCRE)
    target_link_libraries (pikare pcre)
endif (NOT RE_EXTERNAL_PCRE AND HAVE_PCRE)

set_target_properties (pikare 	PROPERTIES
                            	VERSION           ${pikare_LIB_VERSION}
                            	SOVERSION         ${pikare_LIB_VERSION})

install (TARGETS        pikare
         RUNTIME        DESTINATION bin
         LIBRARY        DESTINATION lib/pika
         ARCHIVE        DESTINATION lib/pika         
         FRAMEWORK      DESTINATION Library/Frameworks)
