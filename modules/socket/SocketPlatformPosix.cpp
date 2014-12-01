#include "SocketPlatform.h"

#if defined(HAVE_SYS_SOCKET_H)
#   include <sys/socket.h>
#endif

#if defined(HAVE_NETINIT_IN_H)
#   include <netinet/in.h>
#endif

#if defined(HAVE_SYS_UN_H)
#   include <sys/un.h>
#endif

#if defined(HAVE_ARPA_INET_H)
#   include <arpa/inet.h>
#endif

#if defined(HAVE_NETDB_H)
#   include <netdb.h>
#endif

struct Pika_socket
{
    int fd;
};

struct Pika_ipaddress : Pika_address
{
    virtual ~Pika_ipaddress() {}
    
    virtual Kind GetKind() const { return Pika_address::ADDR_ip; }
    
    virtual void* GetAddress() { return &this->addr; }
    
    virtual size_t GetLength() { return sizeof(this->addr); }
    
    sockaddr_in addr;
};

struct Pika_unaddress : Pika_address
{
    virtual ~Pika_unaddress() {}

    virtual Kind GetKind() const { return Pika_address::ADDR_unix; }
            
    virtual void*  GetAddress() { return &this->addr; }
    
    virtual size_t GetLength() { return sizeof(this->addr); }
    
    sockaddr_un addr;
};
/*
Text address to packed -> inet_aton | inet_pton
Packed to text -> inet_ntoa | inet_ntop


gethostbyname 
getaddrinfo


inet_pton -> connect
*/

bool Pika_Socket(Pika_socket* sock_ptr, int domain, int type, int protocol)
{
    int fd = socket(domain, type, protocol);
    if (fd < 0) {
        return false;
    }
    sock_ptr->fd = fd;
    return true;
}

bool Pika_Connect(Pika_socket* sock_ptr, Pika_address* addr)
{
    connect(sock_ptr->fd, (struct sockaddr*)addr->GetAddress(), addr->GetLength());
    return true;
}