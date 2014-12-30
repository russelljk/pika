/*
 *  PBasic.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PConfig.h"
#include "PValue.h"
#include "PCollector.h"
#include "PBasic.h"
#include "PEngine.h"
#include "PTable.h"
#include "PFunction.h"
#include "PProperty.h"
#include "PContext.h"

namespace pika {
    
PIKA_IMPL(Basic)

bool Basic::GetSlot(const char* key, Value& result)
{
    String* strKey = engine->GetString(key);
    Value vkey(strKey);
    return GetSlot(vkey, result);
}

bool Basic::SetSlot(const char* key, Value& value, u4 attr)
{
    String* strKey = engine->GetString(key);
    Value vkey(strKey);
    return SetSlot(vkey, value, attr);
}

bool Basic::BracketRead(const char* key, Value& result)
{
    String* strKey = engine->GetString(key);
    Value vkey(strKey);
    return BracketRead(vkey, result);
}

bool Basic::BracketWrite(const char* key, Value& value, u4 attr)
{
    String* strKey = engine->GetString(key);
    Value vkey(strKey);
    return BracketWrite(vkey, value, attr);
}

bool Basic::GetAttr(Context* ctx, const char* key, Value& result)
{
    String* strKey = engine->GetString(key);
    Value vkey(strKey);
    return GetAttr(ctx, vkey, result);
}

bool Basic::SetAttr(Context* ctx, const char* key, Value& value, u4 attr)
{
    String* strKey = engine->GetString(key);
    Value vkey(strKey);
    return SetAttr(ctx, vkey, value, attr);
}

bool Basic::GetAttr(Context* ctx, const Value& key, Value& result)
{
    if (GetSlot(key, result))
    {
        if (result.IsProperty())
        {
            int calls = 0;
            ctx->Push(ToValue());
            ctx->DoPropertyGet(calls, result.val.property);
            if (calls > 0)
                ctx->Run();
            result = ctx->PopTop();
        }
        return true;
    }
    return false;
}

bool Basic::SetAttr(Context* ctx, const Value& key, Value& value, u4 attr)
{
    if (!SetSlot(key, value, attr))
    {
        Value result(NULL_VALUE);
        if (GetSlot(key, result) && result.IsProperty())
        {
            ctx->CheckStackSpace(3);
            ctx->Push(key);
            ctx->Push(ToValue());
            ctx->DoPropertySet(result.val.property);
            return true;
        }
        else
        {
            return false;
        }
    }
    return true;
}

void Basic::EnterConstants(Basic* b, NamedConstant* consts, size_t count)
{
    Engine* eng = b->GetEngine();
    GCPAUSE(eng);
    
    for (size_t i = 0; i < count; ++i)
    {
        Value vname(eng->AllocString(consts[i].name));
        Value val(consts[i].value);
        b->SetSlot(vname, val, Slot::ATTR_protected);
    }
}

void Basic::AddFunction(Function* f)
{
    SetSlot(f->GetName(), f);
}

void Basic::AddProperty(Property* p)
{
    // Without forcewrite a property write will fail if 
    // there is a previous property with the same name some where in the lookup chain.
    SetSlot(p->Name(), p, Slot::ATTR_forcewrite); 
}

void Basic::WriteBarrier(GCObject* gc_obj)
{
    engine->GetGC()->WriteBarrier(this, gc_obj);
}

void Basic::WriteBarrier(const Value& v)
{
    if (v.IsCollectible() && v.val.basic)    
        engine->GetGC()->WriteBarrier(this, v.val.basic);    
}

ClassInfo* Basic::StaticCreateClass()
{
    if (!BasicClassInfo)    
        BasicClassInfo = ClassInfo::Create("Basic", 0);    
    return BasicClassInfo;
}

ClassInfo* Basic::GetClassInfo() { return StaticGetClass(); }

}// pika

