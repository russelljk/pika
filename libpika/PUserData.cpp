/*
 *  PUserData.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"

namespace pika {

PIKA_IMPL(UserData)

UserData::UserData(Engine* eng, Type* obj_type, void* data, UserDataInfo* info)
        : Basic(eng),
        type(obj_type),
        info(info),
        ptr(data)
{}

UserData::~UserData() {}

Type* UserData::GetType() const { return type; }

Value UserData::ToValue()
{
    Value v(this);
    return v;
}

bool UserData::Finalize()
{
    if (GetInfo() && GetInfo()->finalize) {
        return GetInfo()->finalize(this);
    }
    return true;
}

void UserData::MarkRefs(Collector* c)
{
    if (type)
    {
        type->Mark(c);
    }
    if (GetInfo() && GetInfo()->mark)
    {
        GetInfo()->mark(c, this);
    }
}

bool UserData::GetSlot(const Value& val, Value& res)
{
    if (GetInfo() && GetInfo()->get)
    {
        return GetInfo()->get(this, val, res);
    }
    else if (type)
    {
        return type->GetField(val, res);
    }
    return false;
}

bool UserData::SetSlot(const Value& key, Value& value, u4 attr)
{
    // TODO: Type::CanSet
    if (GetInfo() && GetInfo()->set)
        return GetInfo()->set(this, key, value, attr);
    return false;
}

UserData* UserData::CreateWithPointer(Engine* eng, Type* obj_type, void* data, UserDataInfo* info)
{
    UserData* ud;
    PIKA_NEW(UserData, ud, (eng, obj_type, data, info));
    eng->AddToGC(ud);
    return ud;
}

UserData* UserData::CreateManaged(Engine* eng, Type* obj_type, size_t length, UserDataInfo* info)
{
    UserData* ud;
    size_t numBytes = sizeof(UserData) + length;
    void* ptr  = Pika_calloc(numBytes, 1);
    void* data = (void*)(((UserData*)ptr) + 1);
    
    ud = new(ptr) UserData(eng, obj_type, data, info);
    eng->AddToGC(ud);
    
    return ud;
}

}// pika
