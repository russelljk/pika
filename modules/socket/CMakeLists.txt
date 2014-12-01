message (STATUS "********* Starting pikasocket library *********")

# Disable 'unsecure' warnings in VC++

if (WIN32)
    if (MSVC)
        add_definitions(-D_SCL_SECURE_NO_WARNINGS)
        add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    endif (MSVC)
endif (WIN32)

check_include_files (winsock.h      HAVE_WINSOCK_H)
check_include_files (winsock2.h     HAVE_WINSOCK2_H)
check_include_files (sys/socket.h   HAVE_SYS_SOCKET_H)
check_include_files (sys/dl.h       HAVE_SYS_DL_H)
check_include_files (netinet/in.h   HAVE_NETINIT_IN_H)
check_include_files (netinet6/in6.h HAVE_NETINIT6_IN6_H)
check_include_files (sys/un.h       HAVE_SYS_UN_H)
check_include_files (arpa/inet.h    HAVE_ARPA_INET_H)
check_include_files (netdb.h        HAVE_NETDB_H)


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake-socket_config.h.in
               ${CMAKE_CURRENT_SOURCE_DIR}/socket_config.h)

set (socket_LIB_SRCS PSocketAddress.cpp PSocketError.cpp SocketPlatform.cpp PSocket.cpp PSocketModule.cpp)
set (socket_LIB_HEADERS PSocketAddress.h PSocketError.h SocketPlatform.h PSocket.h PSocketModule.h socket_config.h)

if (WIN32)
    set (socket_LIB_SRCS ${socket_LIB_SRCS} SocketPlatformWin.cpp)
endif (WIN32)

if (UNIX)
    set (socket_LIB_SRCS ${socket_LIB_SRCS} SocketPlatformPosix.cpp)
endif (UNIX)

source_group (include FILES ${socket_LIB_HEADERS})
source_group (source  FILES ${socket_LIB_SRCS})

include_directories (${Pika_SOURCE_DIR}/libpika)
link_directories (${Pika_BINARY_DIR}/libpika)

add_library (pikasocket SHARED ${socket_LIB_SRCS} ${socket_LIB_HEADERS})
target_link_libraries (pikasocket ${ADD_LIBS} pika)

set_target_properties ( pikasocket 	PROPERTIES
                           	VERSION           ${pikasocket_LIB_VERSION}
                           	SOVERSION         ${pikasocket_LIB_VERSION})
if (APPLE)
   set_target_properties( pikasocket 
               PROPERTIES 
               MACOSX_RPATH TRUE
               INSTALL_RPATH    "${CMAKE_INSTALL_PREFIX}/lib"
               INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")
endif (APPLE)

install (TARGETS       pikasocket
        RUNTIME        DESTINATION bin
        LIBRARY        DESTINATION lib/pika
        ARCHIVE        DESTINATION lib/pika         
        FRAMEWORK      DESTINATION Library/Frameworks)
