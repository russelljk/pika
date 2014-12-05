#include "SocketPlatform.h"
#include <unistd.h>
#include <fcntl.h>
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

INLINE int ConvertAddressFamily(int fam)
{
    switch(fam) {
    case AF_unspec: return AF_UNSPEC;
    case AF_unix:   return AF_UNIX;
    case AF_inet:   return AF_INET; }
    return fam;
}

INLINE int ConvertSocketOption(int opt)
{
    switch(opt) {
    case SO_linger:       return SO_LINGER;
    case SO_keepalive:    return SO_KEEPALIVE;
    case SO_debug:        return SO_DEBUG;
    case SO_reuseaddr:    return SO_REUSEADDR;
    case SO_sndbuf:       return SO_SNDBUF;
    case SO_rcvbuf:       return SO_RCVBUF; }
    return opt;
}

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
    
    u2 GetPort() const
    {
        return ntohs(this->addr.sin_port);
    }
    
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
    
    u2 GetPort() const
    {
        return ntohs(this->addr.sin6_port);
    }
    
    sockaddr_in6 addr;
};

struct Pika_unaddress : Pika_address
{
    virtual ~Pika_unaddress() {}

    virtual Kind GetKind() const { return Pika_address::ADDR_unix; }
            
    virtual void*  GetAddress() const { return (void*)&this->addr; }
    
    virtual size_t GetLength() const { return sizeof(this->addr); }
    
    u2 GetPort() const { return 0; }
    
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
    int fd = socket(ConvertAddressFamily(domain), type, protocol);
    if (fd < 0) {
        return false;
    }
    sock_ptr->fd = fd;
    return true;
}

void Pika_Close(Pika_socket* sock_ptr)
{
    if (sock_ptr && sock_ptr->fd)
    {
        close(sock_ptr->fd);
        sock_ptr->fd = 0;
    }
}

bool Pika_Shutdown(Pika_socket* sock_ptr, int what)
{
    if (shutdown(sock_ptr->fd, what) < 0)
    {
        return false;
    }
    return true;
}

ssize_t Pika_Send(Pika_socket* sock_ptr, void* buff, size_t length, int flags)
{
    ssize_t res = send(sock_ptr->fd, buff, length, flags);
    return res;
}

ssize_t Pika_Recv(Pika_socket* sock_ptr, void* buff, size_t length, int flags)
{
    ssize_t res = recv(sock_ptr->fd, buff, length, flags);
    return res;
}

ssize_t Pika_SendTo(Pika_socket* sock_ptr, void* buff, size_t length, int flags, Pika_address* addr)
{
    ssize_t res = sendto(sock_ptr->fd, buff, length, flags, (sockaddr*)addr->GetAddress(), addr->GetLength());
    return res;
}

ssize_t Pika_RecvFrom(Pika_socket* sock_ptr, void* buff, size_t length, int flags, Pika_address** paddr)
{
    socklen_t addr_size = sizeof(sockaddr_in);
    sockaddr_in addr;
    Pika_memzero(&addr, addr_size);
    ssize_t res = recvfrom(sock_ptr->fd, buff, length, flags, (sockaddr*)&addr, &addr_size);
    if (res >= 0) {
        Pika_address* ipaddr = 0;
        PIKA_NEW(Pika_ipaddress, ipaddr, (&addr));
        *paddr = ipaddr;
    }
    return res;
}

bool Pika_SetSockOpt(Pika_socket* sock_ptr, int opt, int optval)
{
    if (setsockopt(sock_ptr->fd, SOL_SOCKET, ConvertSocketOption(opt), (void*)&optval, sizeof(int)) < 0)
    {
        return true;
    }
    return false;
}

bool Pika_GetSockOpt(Pika_socket* sock_ptr, int opt, int* optval)
{
    socklen_t sz = 0;
    if (getsockopt(sock_ptr->fd, SOL_SOCKET, ConvertSocketOption(opt), (void*)optval, &sz) < 0)
    {
        return true;
    }
    return false;
}

Pika_address* Pika_Accept(Pika_socket* sock_ptr, int& fd)
{
    socklen_t addr_size = sizeof(sockaddr_in);
    sockaddr_in addr;
    Pika_memzero(&addr, addr_size);
    
    if ((fd = accept(sock_ptr->fd, (sockaddr*)&addr, &addr_size)) < 0)
    {
        if (fd == EAGAIN || fd == EWOULDBLOCK) {
            fd = 0;
        } else {
            fd = errno;
        }
        return 0;
    }
    Pika_address* ipaddr = 0;
    PIKA_NEW(Pika_ipaddress, ipaddr, (&addr));
    return ipaddr;
}

bool Pika_Listen(Pika_socket* sock_ptr, int back_log)
{
    if (listen(sock_ptr->fd, back_log) < 0)
    {
        return false;
    }
    return true;
}

bool Pika_Bind(Pika_socket* sock_ptr, Pika_address* addr)
{
    if (bind(sock_ptr->fd, (sockaddr*)addr->GetAddress(), addr->GetLength()) < 0)
    {
        return false;
    }
    return true;
}

bool Pika_Connect(Pika_socket* sock_ptr, Pika_address* addr)
{
    if (connect(sock_ptr->fd, (sockaddr*)addr->GetAddress(), addr->GetLength()) < 0)
    {
        return false;
    }
    return true;
}

char* Pika_GetAddressInfoError(int error)
{
    const char* errorMessage = gai_strerror(error);
    char* s = (char*)Pika_strdup(errorMessage);
    return s;
}

Pika_address* Pika_GetAddressInfo(const char* addrStr, const char* extra, int& status)
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
            break;
        }
        else if (curr->ai_family == AF_INET6)
        {
            sockaddr_in6* ipv6 = (sockaddr_in6*)curr->ai_addr;
            // addr = &(ipv6->sin6_addr);
            // found = true;
            PIKA_NEW(Pika_ipaddress6, addrres, (ipv6));
            break;
        }
        
        // if (found)
        // {
        //     size_t const IPSTR_LENGTH = INET6_ADDRSTRLEN * sizeof(char);
        //     char* ipstr = (char*)Pika_malloc(IPSTR_LENGTH);
        //     inet_ntop(curr->ai_family, addr, ipstr, IPSTR_LENGTH);
        //     return ipstr;
        // }
    }
    if (res) freeaddrinfo(res);
    return addrres;
}

char* Pika_NetworkToString(Pika_address* paddr)
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

Pika_address* Pika_StringToNetwork(const char* addr, bool ip6)
{
    Pika_address* pikaaddr = 0;
    if (ip6) {
        sockaddr_in in4;
        Pika_memzero(&in4, sizeof(in4));
        if (inet_pton(AF_INET6, addr, &in4) < 0) {
            return 0;
        }
        PIKA_NEW(Pika_ipaddress, pikaaddr, (&in4));
    } else {
        sockaddr_in6 in6;
        Pika_memzero(&in6, sizeof(in6));
        if (inet_pton(AF_INET, addr, &in6) < 0) {
            return 0;
        }
        PIKA_NEW(Pika_ipaddress6, pikaaddr, (&in6));
    }
    return pikaaddr;
}

bool Pika_NonBlocking(Pika_socket* sock_ptr)
{
    return (fcntl(sock_ptr->fd, F_SETFL, O_NONBLOCK) >= 0);    
}

