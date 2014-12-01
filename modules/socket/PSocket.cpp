#include "PSocketModule.h"

namespace pika {
Socket::Socket(Engine* engine, Type* type) : Object(engine, type), socket(0)
{
    
}

Socket::~Socket()
{
    
}

void Socket::Init(Context* ctx)
{
    u2 argc = ctx->GetArgCount();
    if (argc != 2)
    {
        ctx->WrongArgCount();
    }
    
    int domain = ctx->GetIntArg(0);
    int type = ctx->GetIntArg(1);
    
    if (domain <= AF_min || domain >= AF_max)
    {
        RaiseException(Exception::ERROR_type, "Invalid domain specified for Socket.init argument 1.");
    }    
    else if (type <= SOCK_min || type >= SOCK_max)
    {
        RaiseException(Exception::ERROR_type, "Invalid socket type specified for Socket.init argument 2.");
    }
    
    if (!Pika_Socket(this->socket, domain, type, 0))
    {
        RaiseExceptionFromErrno(Exception::ERROR_type, "Attempt to create socket failed", errno);
        

    }
}

void Socket::Connect(String* addr)
{
    Pika_address* sockaddr = 0;    
    if (!Pika_Connect(this->socket, sockaddr))
    {
        RaiseExceptionFromErrno(Exception::ERROR_runtime, "Attempt to create socket failed", errno);
    }
}

Socket* Socket::StaticNew(Engine* eng, Type* type)
{
    Socket* socket = 0;
    GCNEW(eng, Socket, socket, (eng, type));
    return socket;
}

void Socket::Constructor(Engine* eng, Type* obj_type, Value& res)
{
    Object* socket = Socket::StaticNew(eng, obj_type);
    res.Set(socket);
}

PIKA_IMPL(Socket)

}// pika

using namespace pika;

void Initialize_Socket(Package* socket, Engine* eng)
{
    String* Socket_String = eng->AllocString("Socket");
    Type*   Socket_Type   = Type::Create(eng, Socket_String, eng->Object_Type, Socket::Constructor, socket);
    socket->SetSlot(Socket_String, Socket_Type);
    eng->AddBaseType(Socket_String, Socket_Type);
}
