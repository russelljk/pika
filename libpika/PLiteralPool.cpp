/*
 *  PLiteralPool.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"

namespace pika {

LiteralPool::LiteralPool(Engine* eng) : engine(eng) {}

LiteralPool::~LiteralPool() {}

LiteralPool* LiteralPool::Create(Engine* eng)
{
    LiteralPool* litpool = 0;
    PIKA_NEW(LiteralPool, litpool, (eng));
    eng->AddToGC(litpool);
    return litpool;
}

u2 LiteralPool::Add(pint_t i)
{
    Value v;
    v.tag = TAG_integer;
    v.val.integer = i;
    return Add(v);
}

u2 LiteralPool::Add(preal_t r)
{
    Value v;
    v.tag = TAG_real;
    v.val.real = r;
    return Add(v);
}

u2 LiteralPool::Add(String* s)
{
    Value v(s);
    return Add(v);
}

u2 LiteralPool::Add(const Value& v)
{
    size_t idx = literals.GetSize();

    if (idx >= PIKA_MAX_LITERALS)
    {
        RaiseException("LiteralPool capacity exceeded. Maximum of %d literals allowed per pool.", PIKA_MAX_LITERALS);
    }

    if (v.IsCollectible())
        engine->GetGC()->WriteBarrier(this, v.val.basic);

    literals.Push(v);

    return (u2)idx;
}

void LiteralPool::MarkRefs(Collector* c)
{
    for (size_t i = 0; i < literals.GetSize(); ++i)
    {
        MarkValue(c, literals[i]);
    }
}

}// pika
