#include "PSocketModule.h"

namespace pika {
SocketAddress::SocketAddress(Engine* engine, Type* type, Pika_address* addr)
     : Object(engine, type), addr(addr)
{    
}

SocketAddress::~SocketAddress()
{
    delete addr;
}

void SocketAddress::Init(Context* ctx)
{
    u2 argc = ctx->GetArgCount();
    bool ip6 = false;
    String* ip = 0;
    switch(argc)
    {
    case 2:
        ip6 = ctx->GetBoolArg(1);
    case 1:
        ip = ctx->GetStringArg(0);
        break;
    default:
        ctx->WrongArgCount();
    }
    
    this->addr = Pika_StringToNetwork(ip->GetBuffer(), ip6);
    
    if (!this->addr) {
        RaiseException(Exception::ERROR_runtime, "SocketAddress.init invalid argument 1 of '%s' specified. Excepted valid ip address. If DNS lookups are required use socket.getaddrinfo instead.", ip->GetBuffer());
    }
}

Pika_address* SocketAddress::GetAddress()
{
    if (!this->addr) {
        RaiseException(Exception::ERROR_runtime, "Attempt to use uninitialized SocketAddress.");
    }
    return this->addr;
}

Type* SocketAddress::StaticGetType(Engine* eng)
{
    return eng->GetBaseType(eng->GetString("SocketAddress"));
}

SocketAddress* SocketAddress::StaticNew(Engine* eng, Type* type, Pika_address* addr)
{
    SocketAddress* sa = 0;
    GCNEW(eng, SocketAddress, sa, (eng, type, addr));
    return sa;
}

void SocketAddress::Constructor(Engine* eng, Type* obj_type, Value& res)
{
    Object* addr = SocketAddress::StaticNew(eng, obj_type);
    res.Set(addr);
}

String* SocketAddress::GetAddressString()
{
    char* s = Pika_NetworkToString(this->addr);
    return this->engine->GetString(s);
}

PIKA_IMPL(SocketAddress)

}// pika

using namespace pika;

void Initialize_SocketAddress(Package* socket, Engine* eng)
{
    String* SocketAddress_String = eng->AllocString("SocketAddress");
    Type*   SocketAddress_Type   = Type::Create(eng, SocketAddress_String, eng->Object_Type, SocketAddress::Constructor, socket);
    socket->SetSlot(SocketAddress_String, SocketAddress_Type);
    eng->AddBaseType(SocketAddress_String, SocketAddress_Type);
    
    SlotBinder<SocketAddress>(eng, SocketAddress_Type)
    .Method(&SocketAddress::GetAddressString,   "getAddressString", 0)
    ;
}