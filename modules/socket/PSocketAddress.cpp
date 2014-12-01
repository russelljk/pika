#include "PSocketModule.h"

namespace pika {
SocketAddress::SocketAddress(Engine* engine, Type* type) : Object(engine, type), addr(0)
{
    
}

SocketAddress::~SocketAddress()
{
    
}

void SocketAddress::Init(Context* ctx)
{
    u2 argc = ctx->GetArgCount();
}

SocketAddress* SocketAddress::StaticNew(Engine* eng, Type* type)
{
    SocketAddress* addr = 0;
    GCNEW(eng, SocketAddress, addr, (eng, type));
    return addr;
}

void SocketAddress::Constructor(Engine* eng, Type* obj_type, Value& res)
{
    Object* addr = SocketAddress::StaticNew(eng, obj_type);
    res.Set(addr);
}

PIKA_IMPL(SocketAddress)

}// pika
