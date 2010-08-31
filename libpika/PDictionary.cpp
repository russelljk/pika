/*
 *  PDictionary.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"

namespace pika {

PIKA_IMPL(Dictionary)

Dictionary::Dictionary(Engine* eng, Type* t) : ThisSuper(eng, t)
{
}

Dictionary::~Dictionary()
{
}

void Dictionary::MarkRefs(Collector* c)
{
    ThisSuper::MarkRefs(c);
    elements.DoMark(c);
}

bool Dictionary::BracketRead(const Value& key, Value& res)
{    
    return elements.Get(key, res);
}

bool Dictionary::BracketWrite(const Value& key, Value& value, u4 attr)
{
    return elements.Set(key, value, attr);
}

void Dictionary::Constructor(Engine* eng, Type* type, Value& res)
{
    Dictionary* dict = 0;
    GCNEW(eng, Dictionary, dict, (eng, type));
    res.Set(dict);
}

Array* Dictionary::Keys()
{
    GCPAUSE_NORUN(engine);
    Array* res = Array::Create(engine, engine->Array_Type,0,0);
    res->SetLength(elements.NumElements());
    size_t index = 0;
    for (Table::Iterator i = elements.GetIterator(); i; ++i) {
        (*res)[index++] = i->key;
    }
    return res;
}

Array* Dictionary::Values()
{
    GCPAUSE_NORUN(engine);
    Array* res = Array::Create(engine, engine->Array_Type,0,0);
    res->SetLength(elements.NumElements());
    size_t index = 0;
    for (Table::Iterator i = elements.GetIterator(); i; ++i) {
        (*res)[index++] = i->val;
    }
    return res;
}

void Dictionary::StaticInitType(Engine* eng)
{
    GCPAUSE_NORUN(eng);
    eng->Dictionary_Type = Type::Create(eng, eng->AllocString("Dictionary"), eng->Object_Type, Dictionary::Constructor, eng->GetWorld());
    SlotBinder<Dictionary>(eng, eng->Dictionary_Type, eng->GetWorld())
    .Method(&Dictionary::Keys,      "keys")
    .Method(&Dictionary::Values,    "values")
    ;
}

}// pika

