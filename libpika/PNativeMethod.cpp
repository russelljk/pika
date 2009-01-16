/*
 *  PNativeMethod.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PNativeMethod.h"
#include "PFunction.h"

namespace pika
{

NativeMethodBase::~NativeMethodBase() 
{
}

void* NativeMethodBase::operator new(size_t n) 
{ 
    return Pika_malloc(n);
}

void NativeMethodBase::operator delete(void *v) 
{ 
    if (v)
        Pika_free(v); 
}

}// pika
