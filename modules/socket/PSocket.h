#ifndef PIKA_SOCKETS_HEADER
#define PIKA_SOCKETS_HEADER

#if defined(PIKA_NIX)
#   if defined(HAVE_SYS_SOCKET_H)
#       include <sys/sockey.h>
#   endif
#   if defined(HAVE_NETINIT_IN_H)
#       include <netinet/in.h>
#   endif
#   if defined(HAVE_SYS_UN_H)
#       include <sys/un.h>
#   endif
#   if defined(HAVE_ARPA_INET_H)
#       include <arpa/inet.h>
#   endif
#   if defined(HAVE_NETDB_H)
#       include <netdb.h>
#   endif
#elif defined(PIKA_WIN)
#   include <windows.h>
#   if defined(HAVE_WINSOCK2_H)
#       include <winsock2.h>
#   elif defined(HAVE_WINSOCK_H)
#       include <winsock.h>
#   endif
#endif

#endif