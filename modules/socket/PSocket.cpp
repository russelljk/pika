/*
 *  PSocket.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PSocketModule.h"

namespace pika {
Socket::Socket(Engine* engine, Type* type) : Object(engine, type), socket(0)
{
    PIKA_NEW(Pika_socket, socket, ());
}

Socket::~Socket()
{
    Close();
    delete socket;
}

void Socket::Close()
{
    if (socket)
    {
        Pika_Close(socket);
    }
}

void Socket::SetSockOpt(pint_t opt, Value val)
{
    int ival = 0;
    
    if (val.IsBoolean())
    {
        ival = val.val.index ? 1 : 0;
    }
    else if (val.IsInteger())
    {
        ival = val.val.integer;
    }
    else
    {
        String* typestr = engine->GetTypenameOf(val);
        RaiseException(Exception::ERROR_type, "Invalid socket option value of type '%s'.", typestr->GetBuffer());
    }
    if (!Pika_SetSockOpt(this->socket, opt, ival))
    {
        RaiseExceptionFromErrno(Exception::ERROR_type, "Attempt to set socket option failed", errno);
    }
}

Value Socket::GetSockOpt(pint_t opt)
{
    Value res(NULL_VALUE);
    int val = 0;
    if (!Pika_GetSockOpt(this->socket, opt, &val))
    {
        RaiseExceptionFromErrno(Exception::ERROR_type, "Attempt to get socket option failed", errno);
    }
    res.Set((pint_t)val);
    return res;
}

void Socket::Init(Context* ctx)
{
    u2 argc = ctx->GetArgCount();
    if (argc != 2 && argc != 3)
    {
        ctx->WrongArgCount();
    }
    
    int domain = AF_inet;
    int type = SOCK_stream;
    bool nonblocking = false;
    
    if (argc == 3) {
        nonblocking = ctx->GetBoolArg(2);
    }
    
    switch(argc) {
    case 3:
        nonblocking = ctx->GetBoolArg(2);
    case 2:
        type = ctx->GetIntArg(1);
    case 1:
        domain = ctx->GetIntArg(0);
        break;
    default:
        if (argc != 0) {
            ctx->WrongArgCount();
        }
    };
    
    if (domain < AF_min || domain > AF_max)
    {
        RaiseException(Exception::ERROR_type, "Invalid domain specified for Socket.init argument 1.");
    }    
    else if (type < SOCK_min || type > SOCK_max)
    {
        RaiseException(Exception::ERROR_type, "Invalid socket type specified for Socket.init argument 2.");
    }
    
    if (!Pika_Socket(this->socket, domain, type, 0))
    {
        RaiseExceptionFromErrno(Exception::ERROR_type, "Attempt to create socket failed", errno);
    }
    
    if (nonblocking)
    {
        if (!Pika_NonBlocking(this->socket))
        {
            RaiseExceptionFromErrno(Exception::ERROR_type, "Attempt to create non-blocking socket failed", errno);
        }
    }
}

void Socket::Shutdown(int what)
{
    if (what < SHUT_min || what > SHUT_max)
    {
        RaiseException(Exception::ERROR_type, "Invalid type for Socket.shutdown argument 1.");
    }
    
    if (!Pika_Shutdown(this->socket, what))
    {
        RaiseExceptionFromErrno(Exception::ERROR_type, "Attempt to shutdown socket failed", errno);
    }
}

void Socket::Connect(Value& addr)
{
    Pika_address* sockaddr = 0;
    if (addr.IsDerivedFrom(SocketAddress::StaticGetClass()))
    {
        sockaddr = static_cast<SocketAddress*>(addr.val.object)->GetAddress(); 
    }
    else
    {
        RaiseException(Exception::ERROR_type, "Socket.connect expects argument 0 to be of type SocketAddress.");
    }
    
    if (!Pika_Connect(this->socket, sockaddr))
    {
        RaiseExceptionFromErrno(Exception::ERROR_runtime, "Attempt to connect socket failed", errno);
    }
}

void Socket::Bind(Value& addr)
{
    Pika_address* sockaddr = 0;
    if (addr.IsDerivedFrom(SocketAddress::StaticGetClass()))
    {
        sockaddr = static_cast<SocketAddress*>(addr.val.object)->GetAddress(); 
    }
    else
    {
        RaiseException(Exception::ERROR_type, "Socket.bindto expects argument 0 to be of type SocketAddress.");
    }
    
    if (!Pika_Bind(this->socket, sockaddr))
    {
        RaiseExceptionFromErrno(Exception::ERROR_runtime, "Attempt to bind socket failed", errno);
    }
}

bool Socket::Accept(Socket** sock, SocketAddress** sockaddr)
{    
    GCPAUSE_NORUN(engine);
    int fd = 0;
    Pika_address* addr = Pika_Accept(this->socket, fd);
    
    if (fd < 0) // Not a file descriptor but an error.
    {
        RaiseExceptionFromErrno(Exception::ERROR_runtime, "Attempt to accept socket failed", fd);
    }
    
    if (addr && fd)
    {
        *sock = Socket::StaticNew(this->engine, this->GetType());
        (*sock)->socket->fd = fd;
        *sockaddr = SocketAddress::StaticNew(this->engine, SocketAddress::StaticGetType(this->engine), addr);
        return true;
    }
    return false;
}

void Socket::Send(String* buff)
{
    ssize_t sent = 0;
    size_t amt = 0;
    size_t length = buff->GetLength();
    int flags = 0;
    
    while ((sent = Pika_Send(this->socket, (void*)(buff->GetBuffer() + sent), length - amt, flags)) > 0)
    {
        amt += sent;
        if (amt >= length)
        {
            return;
        }
    }
    
    if (sent < 0)
    {
        RaiseExceptionFromErrno(Exception::ERROR_runtime, "Attempt to send from socket failed", errno);
    }
}

void Socket::SendTo(String* buff, Value& addr)
{
    ssize_t sent = 0;
    size_t amt = 0;
    size_t length = buff->GetLength();
    int flags = 0;
    Pika_address* sockaddr = 0;
    
    if (addr.IsDerivedFrom(SocketAddress::StaticGetClass()))
    {
        sockaddr = static_cast<SocketAddress*>(addr.val.object)->GetAddress(); 
    }
    else
    {
        RaiseException(Exception::ERROR_type, "Socket.sendto expects argument 0 to be of type SocketAddress.");
    }
    
    while ((sent = Pika_SendTo(this->socket, (void*)(buff->GetBuffer() + sent), length - amt, flags, sockaddr)) > 0)
    {
        amt += sent;
        if (amt >= length)
        {
            return;
        }
    }
    
    if (sent < 0)
    {
        RaiseExceptionFromErrno(Exception::ERROR_runtime, "Attempt to send from socket failed", errno);
    }
}

pint_t Socket::Fileno()
{
    if (!this->socket)
        return 0;
    return this->socket->fd;
}

String* Socket::RecvFrom(pint_t buffsize, SocketAddress** fromAddr)
{
    GCPAUSE_NORUN(engine);
    
    if (buffsize <= 0)
    {
        RaiseException(Exception::ERROR_runtime, "Socket.recvfrom argument 1 must be greater than zero.");
    }
    Buffer<char> buff(buffsize);
    int flags = 0;
    Pika_address* addr = 0;
    ssize_t amt = Pika_RecvFrom(this->socket, (void*)buff.GetAt(0), buff.GetSize(), flags, &addr);
    
    if (addr)
    {
        *fromAddr = SocketAddress::StaticNew(this->engine, SocketAddress::StaticGetType(this->engine), addr);
        delete addr;
    }
    
    if (amt > 0)
    {
        ssize_t sbuffsize = (ssize_t)(buffsize);
        ssize_t len = Min<ssize_t>(amt, sbuffsize);
        return engine->GetString(buff.GetAt(0), len);
    }
    else if (amt < 0)
    {
        RaiseExceptionFromErrno(Exception::ERROR_runtime, "Attempt to recvfrom from socket failed", errno);
    }
    return engine->emptyString; 
}

String* Socket::Recv(pint_t buffsize)
{
    if (buffsize <= 0)
    {
        RaiseException(Exception::ERROR_runtime, "Socket.recv argument 1 must be greater than zero.");
    }
    
    Buffer<char> buff(buffsize);
    int flags = 0;
    ssize_t amt = Pika_Recv(this->socket, (void*)buff.GetAt(0), buff.GetSize(), flags);
    
    // std::cout << "Recv: " << amt << std::endl;
    if (amt > 0)
    {
        ssize_t sbuffsize = (ssize_t)(buffsize);
        ssize_t len = Min<ssize_t>(amt, sbuffsize);
        return engine->GetString(buff.GetAt(0), len);
    }
    else if (amt < 0)
    {
        RaiseExceptionFromErrno(Exception::ERROR_runtime, "Attempt to recv from socket failed", errno);
    }
    return engine->emptyString;
}

void Socket::Listen(int bl)
{
    if (!Pika_Listen(this->socket, bl))
    {
        RaiseExceptionFromErrno(Exception::ERROR_runtime, "Attempt to listen with socket failed", errno);
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

int Socket_accept(Context* ctx, Value& self)
{
    Socket* socket = static_cast<Socket*>(self.val.object);
    Socket* retSock = 0;
    SocketAddress* retAddr = 0;
    if (socket->Accept(&retSock, &retAddr)) {
        ctx->Push(retSock);
        ctx->Push(retAddr);
    } else {
        ctx->PushNull();
        ctx->PushNull();
    }
    return 2;
}

int Socket_recvfrom(Context* ctx, Value& self)
{
    Socket* socket = static_cast<Socket*>(self.val.object);
    SocketAddress* retAddr = 0;
    pint_t len = ctx->GetIntArg(0);
    String* res = socket->RecvFrom(len, &retAddr); 
    if (retAddr) {
        ctx->Push(retAddr);
    } else {
        ctx->PushNull();
    }
    
    if (res) {
        ctx->Push(res);
    } else {
        ctx->PushNull();
    }
    return 2;
}

void Initialize_Socket(Package* socket, Engine* eng)
{
    String* Socket_String = eng->AllocString("Socket");
    Type*   Socket_Type   = Type::Create(eng, Socket_String, eng->Object_Type, Socket::Constructor, socket);
    socket->SetSlot(Socket_String, Socket_Type);
    eng->AddBaseType(Socket_String, Socket_Type);
    
    SlotBinder<Socket>(eng, Socket_Type)
    .RegisterMethod(Socket_accept,      "accept",   0, false, true , 0)
    .RegisterMethod(Socket_recvfrom,    "recvfrom", 2, false, true , 0)
    .Method(&Socket::Bind,          "bindto",   0)
    .Method(&Socket::Close,         "close",    0)
    .Method(&Socket::Connect,       "connect",  0)
    .Method(&Socket::Listen,        "listen",   0)
    .Method(&Socket::Send,          "send",     0)
    .Method(&Socket::SendTo,        "sendto",   0)
    .Method(&Socket::Shutdown,      "shutdown", 0)
    .Method(&Socket::Recv,          "recv",     0)
    .Method(&Socket::GetSockOpt,    "getsockopt", 0)
    .Method(&Socket::SetSockOpt,    "setsockopt", 0)
    .Method(&Socket::Fileno,        "fileno")
    .Alias("onDispose",             "close")
    ;
}
