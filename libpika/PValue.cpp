/*
 *  PValue.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"

namespace pika 
{
static const char* STypeNames[] = 
{
/*  0  */ "Null",
/*  1  */ "Boolean",
/*  2  */ "Integer",
/*  3  */ "Real",
/*  4  */ "Index",
/*  5  */ "GCObj",
/*  6  */ "Def",
/*  7  */ "String",
/*  8  */ "Enumerator",
/*  9  */ "Property",
/* 10  */ "UserData",
/* 11  */ "Object",
/* 12  */ "Invalid Tag",
};

PIKA_API const char* GetTypeString(u2 e)
{
    if (e < MAX_TAG)
        return STypeNames[e];
    return STypeNames[MAX_TAG];
}

bool Value::IsDerivedFrom(ClassInfo* c)
{
    return (tag == TAG_object) && val.object && val.object->IsDerivedFrom(c);
}

const char* ScriptException::GetMessage() const
{
    Value res(NULL_VALUE);
    if (msg) 
        return msg;
    else if (var.IsString())
        return var.val.str->GetBuffer();
    else if (var.IsObject() && var.val.object->GetSlot(var.val.object->GetEngine()->message_String, res))
    {
        if (res.IsString())
            return res.val.str->GetBuffer();
    }
    return 0;
}

}// pika
