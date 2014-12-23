/*
 *  PZInflater.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PZlib.h"

namespace pika {

Inflater::Inflater(Engine* engine, Type* type) : ThisSuper(engine, type)
{
}
        
Inflater::~Inflater()
{
}

Inflater* Inflater::StaticNew(Engine* eng, Type* type)
{
    Inflater* obj = 0;
    GCNEW(eng, Inflater, obj, (eng, type));
    return obj;
}

void Inflater::Constructor(Engine* eng, Type* obj_type, Value& res)
{
    Object* obj = Inflater::StaticNew(eng, obj_type);
    res.Set(obj);
}

PIKA_IMPL(Inflater)

}// pika
