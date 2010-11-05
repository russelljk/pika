/*
 *  PEnumerator.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PConfig.h"
#include "PValue.h"
#include "PCollector.h"
#include "PBasic.h"
#include "PEngine.h"
#include "PEnumerator.h"
#include "PObject.h"
#include "PType.h"
#include "PContext.h"

namespace pika {

PIKA_IMPL(Enumerator)

Type* Enumerator::GetType() const { return engine->Enumerator_Type; }
    
bool Enumerator::GetSlot(const Value& key, Value& result)
{
    return engine->Enumerator_Type->GetField(key, result);
}

void Enumerator::CallNext(Context* ctx, u4 count)
{
    if (count != 0)
        count--;        
    Value res(NULL_VALUE);
    this->GetCurrent(res);
    ctx->Push(res);    
    
    
    while (count--)
    {
        ctx->PushNull();
    }
}

Enumerator* DummyEnumerator::Create(Engine* eng)
{
    DummyEnumerator* e = 0;
    PIKA_NEW(DummyEnumerator, e, (eng));
    eng->AddToGC(e);
    return e;
}

Value Enumerator::ToValue()
{
    Value v(this);
    return v;
}

}// pika

