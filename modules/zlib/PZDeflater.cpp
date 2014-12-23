/*
 *  PZDeflater.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PZlib.h"

namespace pika {

Deflater::Deflater(Engine* engine, Type* type) : ThisSuper(engine, type)
{
}
        
Deflater::~Deflater()
{
}

Deflater* Deflater::StaticNew(Engine* eng, Type* type)
{
    Deflater* obj = 0;
    GCNEW(eng, Deflater, obj, (eng, type));
    return obj;
}

void Deflater::Constructor(Engine* eng, Type* obj_type, Value& res)
{
    Object* obj = Deflater::StaticNew(eng, obj_type);
    res.Set(obj);
}

PIKA_IMPL(Deflater)

}// pika
