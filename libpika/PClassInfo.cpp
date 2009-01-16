/*
 *  PClassInfo.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PConfig.h"
#include "PUtil.h"
#include "PMemory.h"
#include "PClassInfo.h"
#include "PMemPool.h"

namespace pika
{

ClassInfo* ClassInfo::firstClass = 0;

ClassInfo::ClassInfo(char* n, ClassInfo* s)
    : name(n), super(s)
{
    next = firstClass;
    firstClass = this;
}

ClassInfo::~ClassInfo() {}

ClassInfo* ClassInfo::Create(char* name, ClassInfo* super)
{
    static MemObjPool<ClassInfo> classInfoPool(32);
    
    void* pv = classInfoPool.RawAlloc();
    ClassInfo* classInfo = new(pv) ClassInfo(name, super);
    return classInfo;
}

bool ClassInfo::IsDerivedFrom(const ClassInfo* other) const
{
    if (!other)
        return false;
        
    for (const ClassInfo* cls = this; cls; cls = cls->super)
    {
        if (cls == other)
            return true;
    }
    return false;
}

}//pika
