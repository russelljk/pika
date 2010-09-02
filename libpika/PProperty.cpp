/*
 *  PProperty.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PBasic.h"
#include "PProperty.h"
#include "PString.h"
#include "PObject.h"
#include "PType.h"
#include "PDef.h"
#include "PFunction.h"
#include "PEngine.h"

namespace pika {
PIKA_IMPL(Property)

Property::Property(Engine* eng, String* nm, Function* get, Function* set)
        : Basic(eng),
        getter(get),
        setter(set),
        name(nm) 
{
}

Property::~Property()
{
}

Type* Property::GetType() const { return engine->Property_Type; }

bool Property::GetSlot(const Value& key, Value& result)
{
    return engine->Property_Type->GetField(key, result);
}

void Property::MarkRefs(Collector* c)
{
    if (name)   name->Mark(c);
    if (getter) getter->Mark(c);
    if (setter) setter->Mark(c);
}

bool Property::CanWrite() { return setter != 0; }
bool Property::CanRead() { return getter != 0; }

Function* Property::Reader() { return getter; }
Function* Property::Writer() { return setter; }
String*   Property::Name()   { return name; }

Property* Property::CreateReadWrite(Engine* eng, String* name, Function* getter, Function* setter)
{
    Property* prop = 0;
    GCNEW(eng, Property, prop, (eng, name, getter, setter));
    return prop;
}

Property* Property::CreateRead(Engine* eng, String* name, Function* getter)
{
    Property* prop = 0;
    GCNEW(eng, Property, prop, (eng, name, getter, 0));
    return prop;
}

Property* Property::CreateWrite(Engine* eng, String* name, Function* setter)
{
    Property* prop = 0;
    GCNEW(eng, Property, prop, (eng, name, 0, setter));    
    return prop;
}

void Property::SetWriter(Function* s)
{
    setter = s;
    WriteBarrier(setter);
}

void Property::SetRead(Function* g)
{
    getter = g;
    WriteBarrier(getter);
}

}// pika

