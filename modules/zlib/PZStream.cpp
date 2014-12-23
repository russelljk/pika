/*
 *  PZStream.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PZlib.h"

namespace pika {

ZStream::ZStream(Engine* engine, Type* type) : ThisSuper(engine, type)
{
    Pika_memzero(&stream, sizeof(z_stream));
}

ZStream::~ZStream()
{
}

PIKA_IMPL(ZStream)

}// pika
