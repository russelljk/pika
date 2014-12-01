/*
 *  SockeyPlatform.h
 *  See Copyright Notice in PSocketModule.h
 */
#ifndef SOCKET_PLATFORM_HEADER
#define SOCKET_PLATFORM_HEADER

#include "PMemory.h"
#include "socket_config.h"

struct Pika_socket;

struct Pika_address
{
    enum Kind {
        ADDR_ip,
        ADDR_unix,
    };
    
    virtual ~Pika_address() = 0;
    
    virtual Kind   GetKind()    const = 0;
    virtual void*  GetAddress() const = 0;
    virtual size_t GetLength()  const = 0;
};

enum AddressFamily {
    AF_min = 0,
    AF_unspec = 0,
    AF_unix = 1,
    AF_inet = 2,
    AF_max = AF_inet,
};

enum SocketType {
    SOCK_min = 1,
    SOCK_stream = 1,
    SOCK_datagram = 2,
    SOCK_raw = 3,
    SOCK_max = SOCK_raw,
};

bool    Pika_Socket(Pika_socket* sock_ptr, int domain, int type, int protocol);
bool    Pika_Connect(Pika_socket*, Pika_address*);

char*   Pika_ErrorMessage(int);
void    Pika_FreeErrorMessage(char*);

#endif
