/*
 *  PBasic.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PConfig.h"
#include "PValue.h"
#include "PCollector.h"
#include "PBasic.h"
#include "PEngine.h"
#include "PEnumerator.h"
#include "PTable.h"

namespace pika
{

PIKA_IMPL(Basic)

bool Basic::SetSlot(const char* key, Value& value, u4 attr)
{
    String* strKey = engine->AllocString(key);
    Value vkey(strKey);
    return SetSlot(vkey, value, attr);
}

Enumerator* Basic::GetEnumerator(String*)
{
    return DummyEnumerator::Create(engine);
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

