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
    u4 argc = ctx->GetArgCount();
    if (argc == 1) {
        this->property = ctx->GetPropertyArg(0);
        name.Set(this->property->Name());
        WriteBarrier(property);
        WriteBarrier(name);
        
        Function* f = property->Writer() ? property->Writer() : property->Reader();
        if (f) {            
            object.Set(f->GetLocation());
            WriteBarrier(object);
        }        
    } else if (argc == 2) {
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
                RaiseException("Attempt to get property %s from object of type %s.", engine->SafeToString(ctx, name), engine->GetTypenameOf(object));
            }
        }
    } else {
        ctx->CheckArgCount(2);
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

int Proxy_getReader(Context* ctx, Value& self)
{
    Proxy* proxy = (Proxy*)self.val.object;
    Function* f = proxy->Reader();
    if (f) {
        ctx->Push(f);
        return 1;
    } 
    ctx->PushNull();
    return 1;
}

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

int Proxy_getWriter(Context* ctx, Value& self)
{
    Proxy* proxy = (Proxy*)self.val.object;
    Function* f = proxy->Writer();
    if (f) {
        ctx->Push(f);
        return 1;
    } 
    ctx->PushNull();
    return 1;
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
    
    static RegisterProperty Proxy_Properties[] = {
    { "reader", Proxy_getReader, "getReader", Proxy_setReader, "setReader", false, 0, 0 },
    { "writer", Proxy_getWriter, "getWriter", Proxy_setWriter, "setWriter", false, 0, 0 },
    };
    
    SlotBinder<Proxy>(eng, Proxy_Type)
    .Method(&Proxy::IsRead, "read?")
    .Method(&Proxy::IsWrite, "write?")
    .Method(&Proxy::IsReadWrite, "readWrite?")
    .PropertyR("object", &Proxy::Obj, "getObject")
    .PropertyR("name", &Proxy::Name, "getName")
    ;
    Proxy_Type->EnterProperties(Proxy_Properties, countof(Proxy_Properties));
}

}// pika
