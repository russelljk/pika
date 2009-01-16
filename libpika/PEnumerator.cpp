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

namespace pika
{
PIKA_IMPL(Enumerator)

Type* Enumerator::GetType() const { return engine->Enumerator_Type; }
    
bool Enumerator::GetSlot(const Value& key, Value& result)
{
    return engine->Enumerator_Type->GetField(key, result);
}

Enumerator* DummyEnumerator::Create(Engine* eng)
{
    DummyEnumerator* e = 0;
    PIKA_NEW(DummyEnumerator, e, (eng));
    eng->AddToGC(e);
    return e;
}

}// pika

