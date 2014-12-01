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

String* SocketAddress::GetAddress()
{
    char* s = Pika_ntop(this->addr);
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
    .Method(&SocketAddress::GetAddress,   "getAddress", 0)
    ;
}