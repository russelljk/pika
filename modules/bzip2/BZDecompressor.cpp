/*
 *  BZDecompressor.cpp
 *  See Copyright Notice in Pika.h
 */
#include "BZip2.h"

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

int Decompressor::Call(int)
{
    return BZ2_bzDecompress(&stream);
}

void Decompressor::End()
{
    BZ2_bzDecompressEnd(&stream);
}

void Decompressor::Begin()
{
    ThisSuper::Begin();
    
    int ret = BZ2_bzDecompressInit(&stream, 0, 0);
    
    if (ret != BZ_OK) {
        RaiseException(
            this->GetErrorClass(),
            "Attempt to initialze Decompressor failed."
        );
    }
}

PIKA_IMPL(Decompressor)

}// pika
