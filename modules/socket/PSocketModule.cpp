/*
 *  PSocketModule.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PSocketModule.h"

using namespace pika;

extern void Initialize_Socket(Package*, Engine*);
extern void Initialize_SocketAddress(Package*, Engine*);

struct AutoFreeErrorString {
    AutoFreeErrorString(char* str): str(str)
    {        
    }
    
    ~AutoFreeErrorString()
    {
        Pika_FreeString(str);
    }
    char* str;
};

int socket_getaddrinfo(Context* ctx, Value&)
{
    u2 argc = ctx->GetArgCount();
    if (argc == 0 || argc > 2) {
        ctx->WrongArgCount();
    }
    const char* extra = 0;
    const char* addr = 0;
    switch(argc)
    {
    case 2:
        extra = ctx->GetStringArg(1)->GetBuffer();
        //
        // Fall through |
        //              |
        //              V
    case 1:
        addr = ctx->GetStringArg(0)->GetBuffer();
        break;
    }
    
    int error = 0;
    Pika_address* paddr = Pika_GetAddressInfo(addr, extra, error);
    
    if (paddr)
    {
        Engine* eng = ctx->GetEngine();
        SocketAddress* sa = SocketAddress::StaticNew(eng, SocketAddress::StaticGetType(eng), paddr);
        ctx->Push(sa);
        return 1;
    }
    else
    {
        if (error != 0)
        {
            char* str = Pika_GetAddressInfoError(error);
            AutoFreeErrorString afes(str);
            RaiseException(Exception::ERROR_runtime, "Attempt to get address resulted in error '%s'", str);
        }
        RaiseException(Exception::ERROR_runtime, "Attempt to get address information for '%s' failed.", addr);    
    }
    return 0;
}

PIKA_MODULE(socket, eng, socket)
{
    GCPAUSE(eng);
    
    static RegisterFunction socket_Functions[] = {
        { "getaddrinfo", socket_getaddrinfo, 0, DEF_VAR_ARGS, 0 },  
    };
    
    static NamedConstant AddressFamily_Constants[] = {
        { "AF_UNSPEC", AF_unspec },
        { "AF_UNIX",   AF_unix   },
        { "AF_INET",   AF_inet   },
    };
    
    static NamedConstant SocketType_Constants[] = {
        { "SOCK_STREAM",     SOCK_stream },
        { "SOCK_DGRAM",      SOCK_dgram },
        { "SOCK_RAW",        SOCK_raw },
    };
    
    static NamedConstant ShutdownType_Constants[] = {
        { "SHUT_RD",    SHUT_rd   },
        { "SHUT_WR",    SHUT_wr   },
        { "SHUT_RDWR",  SHUT_rdwr },
    };
    
    static NamedConstant SocketOption_Constants[] = {
        { "SO_LINGER",      SO_linger    },
        { "SO_KEEPALIVE",   SO_keepalive },
        { "SO_DEBUG",       SO_debug     },
        { "SO_REUSEADDR",   SO_reuseaddr },
        { "SO_SNDBUF",      SO_sndbuf    },
        { "SO_RCVBUF",      SO_rcvbuf    },
    };
    
    socket->EnterFunctions(socket_Functions, countof(socket_Functions));
    
    Basic::EnterConstants(socket, AddressFamily_Constants, countof(AddressFamily_Constants));
    Basic::EnterConstants(socket, SocketType_Constants,    countof(SocketType_Constants));    
    Basic::EnterConstants(socket, ShutdownType_Constants,  countof(ShutdownType_Constants));
    Basic::EnterConstants(socket, SocketOption_Constants,  countof(SocketOption_Constants));
            
    Initialize_Socket(socket, eng);
    Initialize_SocketAddress(socket, eng);
    
    return socket;
}
