/*
 *  PObject.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PConfig.h"
#include "PCollector.h"
#include "PEngine.h"
#include "PContext.h"
#include "PNativeBind.h"
#include "PProxy.h"

namespace pika {
PIKA_IMPL(Proxy)

Proxy::Proxy(Engine* eng, Type* type) : ThisSuper(eng, type), object(NULL_VALUE), name(NULL_VALUE), property(0)
{
}

Proxy::~Proxy()
{
}

void Proxy::Init(Context* ctx)
{
    ctx->CheckArgCount(2);
    object = ctx->GetArg(0);
    name = ctx->GetArg(1);
    
    WriteBarrier(object);
    WriteBarrier(name);
    
    if (object.tag >= TAG_basic)
    {
        Basic* basic = object.val.basic;
        Value res;
        if (!basic->GetSlot(name, res))
        {
        }
        if (res.tag == TAG_property)
        {
            this->property = res.val.property;
            WriteBarrier(property);
        }
        else {
            RaiseException("attempt to get property %s from object of type %s.", engine->ToString(ctx, name), engine->GetTypenameOf(object));
        }
    }
}

void Proxy::Constructor(Engine* eng, Type* type, Value& res)
{
    Proxy* p = 0;
    GCNEW(eng, Proxy, p, (eng, type));
    res.Set(p);
}

void Proxy::MarkRefs(Collector* c)
{
    ThisSuper::MarkRefs(c);
    MarkValue(c, object);
    MarkValue(c, name);
    if (property)
        property->Mark(c);
}

Function* Proxy::Attach(Function* fn)
{
    if (fn)
    {
        GCPAUSE_NORUN(engine);
        if (object.IsDerivedFrom(Type::StaticGetClass()))
        {
            Type* t = (Type*)object.val.object;
            InstanceMethod* im = InstanceMethod::Create(engine, 0, fn, t);
            return im;
        }
        else
        {
            BoundFunction* bf = BoundFunction::Create(engine, 0, fn, object);
            return bf;
        }
    }
    return fn;
}

void Proxy::SetReader(Function* fn, bool attach)
{
    if (attach && fn) {
        fn =  Attach(fn);
    }
    property->SetRead(fn);
}

void Proxy::SetWriter(Function* fn, bool attach)
{
    if (attach && fn) {
        fn =  Attach(fn);
    }
    property->SetWriter(fn);    
}

bool Proxy::IsRead() { return property ? property->CanRead() : false; }
bool Proxy::IsWrite() { return property ? property->CanWrite() : false; }
bool Proxy::IsReadWrite() { return property ? property->CanRead() && property->CanWrite() : false; }

Function* Proxy::Reader() { return property ? property->Reader() : 0; }
Function* Proxy::Writer() { return property ? property->Writer() : 0; }

Value Proxy::Obj() { return object; }
Value Proxy::Name() { return name; }    

namespace {

int Proxy_setReader(Context* ctx, Value& self)
{
    Proxy* proxy = (Proxy*)self.val.object;
    u2 argc = ctx->GetArgCount();
    Function* fn = 0;
    bool attach = false;
    switch (argc) {
    case 2:
        attach = ctx->GetBoolArg(1);
    case 1:
        fn = ctx->GetArgT<Function>(0);
        break;
    default:
        ctx->WrongArgCount();
    }
    proxy->SetReader(fn, attach);
    return 0;
}

int Proxy_setWriter(Context* ctx, Value& self)
{
    Proxy* proxy = (Proxy*)self.val.object;
    u2 argc = ctx->GetArgCount();
    Function* fn = 0;
    bool attach = false;
    switch (argc) {
    case 2:
        attach = ctx->GetBoolArg(1);
    case 1:
        fn = ctx->GetArgT<Function>(0);
        break;
    default:
        ctx->WrongArgCount();
    }
    proxy->SetWriter(fn, attach);
    return 0;
}

}// namespace

void Proxy::StaticInitType(Engine* eng)
{
    String* Proxy_String = eng->AllocString("Proxy");
    Type* Proxy_Type = Type::Create(eng, Proxy_String, eng->Object_Type, Proxy::Constructor, eng->Property_Type);
    
    eng->Property_Type->SetSlot(Proxy_String, Proxy_Type);
    
    SlotBinder<Proxy>(eng, Proxy_Type, eng->Property_Type)
    .Method(&Proxy::IsRead, "read?")
    .Method(&Proxy::IsWrite, "write?")
    .Method(&Proxy::IsReadWrite, "readWrite?")
    .Method(&Proxy::Writer, "getWriter")
    .Method(&Proxy::Reader, "getReader")
    .PropertyR("object", &Proxy::Obj, "getObject")
    .PropertyR("name", &Proxy::Name, "getName")
    .Register(Proxy_setReader, "setReader")
    .Register(Proxy_setWriter, "setWriter")
    ;
}

}// pika
