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
#include "PObjectIterator.h"

namespace pika {
extern Iterator* CreateSlotEnumerator(Engine* engine, IterateKind, Object* self, Table* table);

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

LocalsObject::LocalsObject(const LocalsObject* rhs) :
    ThisSuper(rhs),
    indices(rhs->indices),
    lexEnv(rhs->lexEnv),
    function(rhs->function),
    parent(rhs->parent),
    pos(rhs->pos)
{
}

Object* LocalsObject::Clone()
{
    LocalsObject* l = 0;
    GCNEW(engine, LocalsObject, l, (this));
    return l;
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
        pint_t    parentCodeLen = parentFunc->def->bytecode->length;
        
        if (parentPos >= 0 && parentPos < parentCodeLen)
        {
            parent = Create(engine, GetType(), parentFunc, function->lexEnv, parentPos);
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
    if (!members)
        return false;
    return members->Exists(key);
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

Iterator* LocalsObject::Iterate(String* str)
{
    IterateKind k = (str == engine->values_String) ? IK_values : IK_default;
    Iterator* e = CreateSlotEnumerator(engine, k, this, &(this->indices));
    return e;
}

String* LocalsObject::ToString()
{
    return String::ConcatSep(this->GetType()->GetName(), function ? function->GetName() : engine->emptyString, ':');
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

namespace {

// LocalObject API /////////////////////////////////////////////////////////////////////////////////

int LocalsObject_getParent(Context* ctx, Value& self)
{
    if (self.IsDerivedFrom(LocalsObject::StaticGetClass()))
    {
        LocalsObject* obj = (LocalsObject*)self.val.object;    
        Object* parent = obj->GetParent();        
        if (parent)
        {
            ctx->Push(parent);
        }
        else
        {
            ctx->PushNull();
        }
        return 1;
    }
    else if (self.IsDerivedFrom(Type::StaticGetClass()))
    {
        Type* t = (Type*)self.val.object;
        ctx->Push(t->GetSuper());   
        return 1;
    }    
    RaiseException("Attempt to call LocalsObject.getParent with incorrect type.");
    return 0;    
}

int LocalsObject_getLength(Context* ctx, Value& self)
{
    LocalsObject* obj = static_cast<LocalsObject*>(self.val.object);
    pint_t sz = static_cast<pint_t>(obj->GetLength());
    ctx->Push(sz);
    return 1;    
}

int LocalsObject_getDepth(Context* ctx, Value& self)
{
    LocalsObject* obj = static_cast<LocalsObject*>(self.val.object);
    Function* fn = obj->GetFunction();
    pint_t depth = 0;
    if (fn)
    {
        Def* def = fn->GetDef();
        if (def)
        {
            def = def->parent;
            while (def)
            {
                def = def->parent;
                ++depth;
            }
        }
    }
    ctx->Push(depth);
    return 1;
}

int LocalsObject_getClosure(Context* ctx, Value& self)
{
    LocalsObject* obj = static_cast<LocalsObject*>(self.val.object);
    Function* fn = obj->GetFunction();
    if (fn)
    {
        ctx->Push(fn);
    }
    else
    {
        ctx->PushNull();
    }
    return 1;
}

}// namespace

void LocalsObject::StaticInitType(Engine* eng)
{
    static RegisterProperty localsObject_Properties[] =
    {
        { "parent",  LocalsObject_getParent,  "getParent",  0, 0, true,  0, 0 },
        { "length",  LocalsObject_getLength,  "getLength",  0, 0, false, 0, 0 },
        { "depth",   LocalsObject_getDepth,   "getDepth",   0, 0, false, 0, 0 },
        { "closure", LocalsObject_getClosure, "getClosure", 0, 0, false, 0, 0 }        
    };
    
    eng->LocalsObject_Type->EnterProperties(localsObject_Properties, countof(localsObject_Properties));
    eng->LocalsObject_Type->SetFinal(true);
    eng->LocalsObject_Type->SetAbstract(true);
}

}// pika
