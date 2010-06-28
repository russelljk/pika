/*
 *  PClassInfo.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PConfig.h"
#include "PUtil.h"
#include "PMemory.h"
#include "PClassInfo.h"
#include "PMemPool.h"

namespace pika {

ClassInfo** ClassInfo::GetFirstClass()
{
    static ClassInfo* first_class = 0;
    return &first_class;
}
    
ClassInfo::ClassInfo(const char* n, ClassInfo* s)
    : name(n), super(s)
{
    ClassInfo** firstClass = GetFirstClass();
    next = *firstClass;
    *firstClass = this;
}

ClassInfo::~ClassInfo() {}

ClassInfo* ClassInfo::Create(const char* name, ClassInfo* super)
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

}// pika
