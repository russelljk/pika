/*
 *  PLocalsObject.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PConfig.h"
#include "PValue.h"
#include "PCollector.h"
#include "PDef.h"
#include "PBasic.h"
#include "PEngine.h"
#include "PObject.h"
#include "PFunction.h"
#include "PEngine.h"
#include "PContext.h"
#include "PNativeBind.h"
#include "PLocalsObject.h"
#include "PEnumerator.h"

namespace pika {
extern Enumerator* CreateSlotEnumerator(Engine* engine, bool values, Basic* self, Table& table);

// LocalObject /////////////////////////////////////////////////////////////////////////////////////

PIKA_IMPL(LocalsObject)

void LocalsObject::BuildIndices()
{
    if (!function) return;
    
    //lexEnv = function->lexEnv;
    Def* def = function->def;
    
    for (size_t i = 0; i < def->localsInfo.GetSize(); ++i)
    {
        LocalVarInfo& info = def->localsInfo[i];
        
        if (info.beg <= pos && info.end > pos)
        {
            Value vval((pint_t)i);
            Value vname(info.name);
            
            indices.Set(vname, vval);
        }
    }
}

void LocalsObject::MarkRefs(Collector* c)
{
    Object::MarkRefs(c);
    
    indices.DoMark(c);
    
    if (lexEnv)   lexEnv->Mark(c);
    if (function) function->Mark(c);
    if (parent)   parent->Mark(c);
}

LocalsObject* LocalsObject::GetParent()
{
    if (!function) return NULL;
    if (parent) return parent;// We created the parent already.
    
    if (function->parent                 && // Does our function even have a parent?
        function->parent->def->mustClose && // Are the parent's local-variables' heap allocated?
        function->parent->def->bytecode)    // Is the parent function a bytecode function?
    {
        Function* parentFunc    = function->parent;
        ptrdiff_t parentPos     = function->def->bytecodePos;
        pint_t     parentCodeLen = parentFunc->def->bytecode->length;
        
        if (parentPos >= 0 && parentPos < parentCodeLen)
        {
            parent = Create(engine, GetType(), parentFunc, function->lexEnv, parentPos);
            parent->lexEnv = parentFunc->lexEnv;
        }
    }
    return parent;
}

bool LocalsObject::GetSlot(const Value& key, Value& result)
{
    if (key.IsString() && indices.Get(key, result) && result.IsInteger())
    {
        pint_t indexof = result.val.integer;
        
        if (lexEnv && indexof >= 0 && indexof < (pint_t)lexEnv->Length())
        {
            result = lexEnv->At(indexof);
            return true;
        }
    }
    return Object::GetSlot(key, result);
}

bool LocalsObject::HasSlot(const Value& key)
{
    Value result = NULL_VALUE;
    if (key.IsString() && indices.Get(key, result) && result.IsInteger())
    {
        pint_t indexof = result.val.integer;
        
        if (lexEnv && indexof >= 0 && indexof < (pint_t)lexEnv->Length())
        {     
            return true;
        }
    }
    return members.Exists(key);
}

bool LocalsObject::SetSlot(const Value& key, Value& value, u4 attr)
{
    Value result;
    if (key.IsString() && indices.Get(key, result) && result.IsInteger())
    {
        pint_t indexof = result.val.integer;
        
        if (lexEnv && indexof >= 0 && indexof < (pint_t)lexEnv->Length())
        {
            WriteBarrier(value);
            lexEnv->At(indexof) = value;
            return true;
        }
    }
    return Object::SetSlot(key, value, attr);
}

Enumerator* LocalsObject::GetEnumerator(String* str)
{
    bool use_values = str == engine->values_String;
    Enumerator* e = CreateSlotEnumerator(engine, use_values, this, this->indices);
    return e;
}

LocalsObject* LocalsObject::Create(Engine* eng, Type* type, Function* function, LexicalEnv* env, ptrdiff_t pc)
{
    LocalsObject* obj;
    PIKA_NEW(LocalsObject, obj, (eng, type, function, env, pc));
    eng->AddToGC(obj);
    return obj;
}

void LocalsObject::Constructor(Engine* eng, Type* obj_type, Value& res)
{
    RaiseException(Exception::ERROR_type, "%s cannot be constructed explicitly.\n", obj_type->GetName()->GetBuffer());
}

}// pika
