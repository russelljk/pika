/*
 *  PDictionary.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PObjectEnumerator.h"

namespace pika {

PIKA_IMPL(Dictionary)

Dictionary::Dictionary(Engine* eng, Type* t) : ThisSuper(eng, t)
{
}

Dictionary::Dictionary(const Dictionary* rhs) :
    ThisSuper(rhs),
    elements(rhs->elements)
{
}

Dictionary::~Dictionary()
{
}

Object* Dictionary::Clone()
{
    Dictionary* d = 0;
    GCNEW(engine, Dictionary, d, (this));
    return d;
}

void Dictionary::MarkRefs(Collector* c)
{
    ThisSuper::MarkRefs(c);
    elements.DoMark(c);
}

Enumerator* Dictionary::GetEnumerator(String* kind)
{
    bool values=false;
    bool punt = true;
    if (kind == engine->elements_String)
    {
        values = true;
        punt = false;
    }
    else if (kind == engine->keys_String or kind == engine->emptyString)
    {
        values = false;
        punt = false;
    }
    
    if (!punt)
    {
        GCPAUSE_NORUN(engine);
        Enumerator* newEnumerator = 0;
        GCNEW(engine, ObjectEnumerator, newEnumerator, (engine, values, this, elements));
        return newEnumerator;
    }    
    return ThisSuper::GetEnumerator(kind);
}

bool Dictionary::BracketRead(const Value& key, Value& res)
{    
    return elements.Get(key, res);
}

bool Dictionary::BracketWrite(const Value& key, Value& value, u4 attr)
{
    return elements.Set(key, value, attr);
}
// TODO: Dictionary and Array ToString could easily go over String's MAX size allowed.
String* Dictionary::ToString()
{
    GCPAUSE_NORUN(engine);
    String* comma = engine->AllocString(", ");
    String*  brace_l = engine->AllocString("{ ");
    String*  brace_r = engine->AllocString(" }");
    String*  colon = engine->AllocString(": ");
    String* res = brace_l;
    Context* ctx = engine->GetActiveContextSafe();
    
    for (Table::Iterator i = elements.GetIterator(); i; )
    {
        Value& key = i->key;
        Value& val = i->val;
        
        String* str_key = engine->SafeToString(ctx, key);
        String* str_val = engine->SafeToString(ctx, val);  
        
        res = String::Concat(res, str_key);
        res = String::Concat(res, colon);
        res = String::Concat(res, str_val);
        
        if (++i)
            res = String::Concat(res, comma);
    }
    res = String::Concat(res, brace_r);
    return res;
}

void Dictionary::Constructor(Engine* eng, Type* type, Value& res)
{
    Dictionary* dict = 0;
    GCNEW(eng, Dictionary, dict, (eng, type));
    res.Set(dict);
}

Dictionary* Dictionary::Create(Engine* eng, Type* type)
{
    Dictionary* d = 0;
    GCNEW(eng, Dictionary, d, (eng, type));
    return d;
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

