/*
 *  PZDecompressor.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PZLib.h"

namespace pika {

Decompressor::Decompressor(Engine* engine, Type* type) : ThisSuper(engine, type)
{
}
        
Decompressor::~Decompressor()
{
}

Decompressor* Decompressor::StaticNew(Engine* eng, Type* type)
{
    Decompressor* obj = 0;
    GCNEW(eng, Decompressor, obj, (eng, type));
    return obj;
}

void Decompressor::Constructor(Engine* eng, Type* obj_type, Value& res)
{
    Object* obj = Decompressor::StaticNew(eng, obj_type);
    res.Set(obj);
}

ClassInfo* Decompressor::GetErrorClass()
{
    return DecompressError::StaticGetClass();
}

int Decompressor::Call(int flush)
{
    return inflate(&stream, Z_SYNC_FLUSH);
}

void Decompressor::End()
{
    inflateEnd(&stream);
}

void Decompressor::Begin()
{
    ThisSuper::Begin();
    
    int ret = inflateInit(&stream);
    
    if (ret != Z_OK) {
        RaiseException(
            this->GetErrorClass(),
            "Attempt to initialze Decompressor failed."
        );
    }
}

PIKA_IMPL(Decompressor)

}// pika
