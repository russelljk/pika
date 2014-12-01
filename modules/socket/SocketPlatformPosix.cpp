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

Pika_address::~Pika_address() {}

struct Pika_ipaddress : Pika_address
{
    Pika_ipaddress(sockaddr_in* a) {
        Pika_memcpy(&addr, a, sizeof(sockaddr_in));
    }
    
    virtual ~Pika_ipaddress() {}
    
    virtual Kind GetKind() const { return Pika_address::ADDR_ip; }
    
    virtual void* GetAddress() const { return (void*)&this->addr; }
    
    virtual size_t GetLength() const { return sizeof(this->addr); }
    
    sockaddr_in addr;
};

struct Pika_ipaddress6 : Pika_address
{
    Pika_ipaddress6(sockaddr_in6* a) {
        Pika_memcpy(&addr, a, sizeof(sockaddr_in6));
    }
    
    virtual ~Pika_ipaddress6() {}
    
    virtual Kind GetKind() const { return Pika_address::ADDR_ip6; }
    
    virtual void* GetAddress() const { return (void*)&this->addr; }
    
    virtual size_t GetLength() const { return sizeof(this->addr); }
    
    sockaddr_in6 addr;
};

struct Pika_unaddress : Pika_address
{
    virtual ~Pika_unaddress() {}

    virtual Kind GetKind() const { return Pika_address::ADDR_unix; }
            
    virtual void*  GetAddress() const { return (void*)&this->addr; }
    
    virtual size_t GetLength() const { return sizeof(this->addr); }
    
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

char* Pika_GetAddrInfoError(int error)
{
    const char* errorMessage = gai_strerror(error);
    char* s = (char*)Pika_strdup(errorMessage);
    return s;
}

Pika_address* Pika_GetAddrInfo(const char* addrStr, const char* extra, int& status)
{
    addrinfo* res = 0;
    // void* addr = 0;
    addrinfo hints;
    Pika_memzero(&hints, sizeof(addrinfo));
    
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    if ((status = getaddrinfo(addrStr, extra, &hints, &res)) != 0)
    {
        return 0;
    }
    Pika_address* addrres = 0;
    // bool found = false;    
    for (addrinfo* curr = res; curr; curr = curr->ai_next)
    {
        if (curr->ai_family == AF_INET)
        {
            sockaddr_in* ipv4 = (sockaddr_in*)curr->ai_addr;
            
            // addr = &(ipv4->sin_addr);
            // found = true;
            PIKA_NEW(Pika_ipaddress, addrres, (ipv4));
            return addrres;
        }
        else if (curr->ai_family == AF_INET6)
        {
            sockaddr_in6* ipv6 = (sockaddr_in6*)curr->ai_addr;
            // addr = &(ipv6->sin6_addr);
            // found = true;
            PIKA_NEW(Pika_ipaddress6, addrres, (ipv6));
            return addrres;
        }
        
        // if (found)
        // {
        //     size_t const IPSTR_LENGTH = INET6_ADDRSTRLEN * sizeof(char);
        //     char* ipstr = (char*)Pika_malloc(IPSTR_LENGTH);
        //     inet_ntop(curr->ai_family, addr, ipstr, IPSTR_LENGTH);
        //     return ipstr;
        // }
    }    
    return addrres;
}

char* Pika_ntop(Pika_address* paddr)
{
    void* address_struct = paddr->GetAddress();
    void* addr = 0;
    int family = 0;
    if (paddr->GetKind() == Pika_address::ADDR_ip)
    {
        sockaddr_in* ipv4 = (sockaddr_in*)address_struct;
        addr = &(ipv4->sin_addr);
        family = AF_INET;
    }
    else if (paddr->GetKind() == Pika_address::ADDR_ip6)
    {
        sockaddr_in6* ipv6 = (sockaddr_in6*)address_struct;
        addr = &(ipv6->sin6_addr);
        family = AF_INET6;
    }
    
    size_t const IPSTR_LENGTH = INET6_ADDRSTRLEN * sizeof(char);
    char* ipstr = (char*)Pika_malloc(IPSTR_LENGTH);
    inet_ntop(family, addr, ipstr, IPSTR_LENGTH);
    return ipstr;
}
