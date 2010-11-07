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

Dictionary::Dictionary(const Dictionary* rhs) : ThisSuper(rhs),
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

Iterator* Dictionary::Iterate(String* kind)
{
    bool punt = true;
    IterateKind k = IK_default;
    if (kind == engine->elements_String)
    {
        k = IK_values;
        punt = false;
    }
    else if (kind == engine->keys_String)
    {
        k = IK_keys;
        punt = false;
    }
    else if (kind == engine->emptyString)
    {
        punt = false;
    }
    
    if (!punt)
    {
        GCPAUSE_NORUN(engine);
        Iterator* newEnumerator = 0;
        GCNEW(engine, ObjectIterator, newEnumerator, (engine, engine->Iterator_Type, k, this, &elements));
        return newEnumerator;
    }    
    return ThisSuper::Iterate(kind);
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
    
    Context* ctx = engine->GetActiveContextSafe();
        
    String* comma = engine->AllocString(", ");
    String* brace_l = engine->AllocString("{ ");
    String* brace_r = engine->AllocString(" }");
    String* colon = engine->AllocString(": ");
    
    // Opening brace {
    String* res = brace_l;
    
    for (Table::Iterator i = elements.GetIterator(); i; )
    {
        Value& key = i->key;
        Value& val = i->val;
        
        String* str_key = engine->SafeToString(ctx, key);
        String* str_val = engine->SafeToString(ctx, val);  
        
        res = String::Concat(res, str_key); // key
        res = String::Concat(res, colon);   // :
        res = String::Concat(res, str_val); // value
        
        // Increment and test to see if are at the end if the Table.
        if (++i)
        {
            // If not add comma before we do the next element.
            res = String::Concat(res, comma);
        }
    }

    // } Closing brace
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
    Array* res = Array::Create(engine, engine->Array_Type, 0, 0);
    res->SetLength(elements.Count());
    size_t index = 0;
    for (Table::Iterator i = elements.GetIterator(); i; ++i)
    {
        (*res)[index++] = i->key;
    }
    return res;
}

Array* Dictionary::Values()
{
    Array* res = Array::Create(engine, engine->Array_Type, 0, 0);
    res->SetLength(elements.Count());
    size_t index = 0;
    for (Table::Iterator i = elements.GetIterator(); i; ++i)
    {
        (*res)[index++] = i->val;
    }
    return res;
}

namespace {
    int Dictionary_unzip(Context* ctx, Value& self)
    {
        GCPAUSE_NORUN(ctx->GetEngine());
        Dictionary* dict = (Dictionary*)self.val.object;
        Array* keys = dict->Keys();
        Array* vals = dict->Values();
        if (!keys || !vals)
        {
            RaiseException("Attempt to unzip Dictionary failed.");
        }
        ctx->Push(keys);
        ctx->Push(vals);
        return 2;
    }
}// namespace

void Dictionary::StaticInitType(Engine* eng)
{
    GCPAUSE_NORUN(eng);
    eng->Dictionary_Type = Type::Create(eng, eng->AllocString("Dictionary"), eng->Object_Type, Dictionary::Constructor, eng->GetWorld());
    SlotBinder<Dictionary>(eng, eng->Dictionary_Type, eng->GetWorld())
    .Method(&Dictionary::Keys,      "keys")
    .Method(&Dictionary::Values,    "values")
    .Register(Dictionary_unzip, "unzip", 0, false, true)
    ;
}

}// pika
