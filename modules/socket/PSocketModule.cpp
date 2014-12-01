#include "PSocketModule.h"

using namespace pika;

PIKA_MODULE(socket, eng, socket)
{
    GCPAUSE(eng);
        
    String* Socket_String = eng->AllocString("Socket");
    Type*   Socket_Type   = Type::Create(eng, Socket_String, eng->Object_Type, Socket::Constructor, socket);
        
    static NamedConstant addressFamily_Constants[] = {
        { "AF_UNSPEC", AF_unspec },
        { "AF_UNIX",   AF_unix   },
        { "AF_INET",   AF_inet   },
    };
    
    static NamedConstant socketType_Constants[] = {
        { "SOCK_STREAM",     SOCK_stream },
        { "SOCK_DATAGRAM",   SOCK_datagram },
        { "SOCK_RAW",        SOCK_raw },
    };
    
    Basic::EnterConstants(socket, addressFamily_Constants, sizeof(addressFamily_Constants));
    Basic::EnterConstants(socket, socketType_Constants,    sizeof(socketType_Constants));    
    
    return socket;
}
