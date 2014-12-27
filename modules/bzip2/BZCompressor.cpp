/*
 *  BZCompressor.cpp
 *  See Copyright Notice in Pika.h
 */
#include "BZip2.h"

namespace pika {

Compressor::Compressor(Engine* engine, Type* type) : ThisSuper(engine, type), level(5)
{
}
        
Compressor::~Compressor()
{
}

Compressor* Compressor::StaticNew(Engine* eng, Type* type)
{
    Compressor* obj = 0;
    GCNEW(eng, Compressor, obj, (eng, type));
    return obj;
}

void Compressor::Constructor(Engine* eng, Type* obj_type, Value& res)
{
    Object* obj = Compressor::StaticNew(eng, obj_type);
    res.Set(obj);
}

ClassInfo* Compressor::GetErrorClass()
{
    return CompressError::StaticGetClass();
}

int Compressor::Call(int flush)
{
    return BZ2_bzCompress(&stream, flush);
}

void Compressor::End()
{
    BZ2_bzCompressEnd(&stream);
}

void Compressor::Begin()
{
    ThisSuper::Begin();
    
    int ret = BZ2_bzCompressInit(&stream, this->level, 0, 30);
    
    if (ret != BZ_OK) {
        RaiseException(
            this->GetErrorClass(),
            "Attempt to initialze Compressor failed."
        );
    }
}

pint_t Compressor::GetLevel() { return this->level; }

void Compressor::SetLevel(pint_t lvl) {
    if (lvl < 10 && lvl > 0)
    {
        this->level = lvl;
        return;
    }
    RaiseException(
        this->GetErrorClass(),
        "Invalid compression level of %d specified.",
        lvl
    );
}

PIKA_IMPL(Compressor)

}// pika
