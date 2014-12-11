/*
 *  SockeyPlatform.h
 *  See Copyright Notice in Pika.h
 */
#ifndef SOCKET_PLATFORM_HEADER
#define SOCKET_PLATFORM_HEADER

#include "PMemory.h"
#include "PByteOrder.h"
#include "socket_config.h"

struct Pika_socket
{
    int fd;
};

struct Pika_address
{
    enum Kind {
        ADDR_ip,
        ADDR_ip6,
        ADDR_unix,
    };
    
    virtual ~Pika_address() = 0;
    
    virtual u2     GetPort()  const = 0;
    virtual Kind   GetKind()        const = 0;
    virtual void*  GetAddress()     const = 0;
    virtual size_t GetLength()      const = 0;
};

enum AddressFamily {
    AF_min      = 0,
    AF_unspec   = 0,
    AF_unix     = 1,
    AF_inet     = 2,
    AF_max      = AF_inet,
};

enum SocketType {
    SOCK_min    = 1,
    SOCK_stream = 1,
    SOCK_dgram  = 2,
    SOCK_raw    = 3,
    SOCK_max    = SOCK_raw,
};

enum ShutdownType {
    SHUT_min    = 0,
    SHUT_rd     = 0,
    SHUT_wr     = 1,
    SHUT_rdwr   = 2,
    SHUT_max    = SHUT_rdwr,
};

enum SocketOption {
    SO_min          = 1,
    SO_linger       = 1,
    SO_keepalive    = 2,
    SO_debug        = 4,
    SO_reuseaddr    = 8,
    SO_sndbuf       = 16,
    SO_rcvbuf       = 64,
    SO_max          = SO_rcvbuf,
};

bool    Pika_Socket(Pika_socket* sock_ptr, int domain, int type, int protocol);
void    Pika_Close(Pika_socket* sock_ptr);
ssize_t Pika_Send(Pika_socket* sock_ptr, void* buff, size_t length, int flags);
ssize_t Pika_Recv(Pika_socket* sock_ptr, void* buff, size_t length, int flags);

ssize_t Pika_SendTo(Pika_socket* sock_ptr, void* buff, size_t length, int flags, Pika_address* addr);
ssize_t Pika_RecvFrom(Pika_socket* sock_ptr, void* buff, size_t length, int flags, Pika_address** addr);

bool Pika_GetSockOpt(Pika_socket* sock_ptr, int opt, int* optval);
bool Pika_SetSockOpt(Pika_socket* sock_ptr, int opt, int optval);

Pika_address* Pika_Accept(Pika_socket*, int&);

bool    Pika_Bind(Pika_socket*, Pika_address*);
bool    Pika_Connect(Pika_socket*, Pika_address*);
bool    Pika_Listen(Pika_socket* sock_ptr, int back_log);

bool    Pika_Shutdown(Pika_socket*, int);
bool    Pika_NonBlocking(Pika_socket*);

char*           Pika_NetworkToString(Pika_address* paddr);
Pika_address*   Pika_StringToNetwork(const char* addr, bool ip6);

Pika_address*   Pika_GetAddressInfo(const char* addrStr, const char* extra, int& status);
char*           Pika_GetAddressInfoError(int);

char*           Pika_GetError(int);

void            Pika_FreeString(char*);

#endif
