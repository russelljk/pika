/*
 *  PObject.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PConfig.h"
#include "PValue.h"
#include "PCollector.h"
#include "PDef.h"
#include "PBasic.h"
#include "PEnumerator.h"
#include "PObject.h"
#include "PType.h"
#include "PFunction.h"
#include "PEngine.h"
#include "PContext.h"
#include "PNativeBind.h"
#include "PObjectEnumerator.h"

/////////////////////////////////////////////// pika ///////////////////////////////////////////////

namespace pika {

///////////////////////////////////////// ObjectEnumerator /////////////////////////////////////////

Enumerator* CreateSlotEnumerator(Engine* engine, bool values, Basic* self, Table& table)
{
    Enumerator* newEnumerator = 0;
    GCNEW(engine, ObjectEnumerator, newEnumerator, (engine, values, self, table));
    return newEnumerator;
}
////////////////////////////////////////////// Object //////////////////////////////////////////////

PIKA_IMPL(Object)

Object::~Object() {}

Object* Object::StaticNew(Engine* eng, Type* type)
{
    Object* newObject = 0;
    GCNEW(eng, Object, newObject, (eng, type));
    
    return newObject;
}

void Object::MarkRefs(Collector* c)
{
    members.DoMark(c);
    if (type) type->Mark(c);
}

String* Object::ToString()
{
    return String::ConcatSpace(this->GetType()->GetName(), engine->AllocString("instance"));
}

void Object::Init(Context* ctx) {}

bool Object::RawSet(Object* obj, Value& key, Value& val)
{
    return obj->SetSlot(key, val, Slot::ATTR_forcewrite);
}

Value Object::RawGet(Object* obj, Value& key)
{
    Value result;
    if (!obj->GetSlot(key, result))
    {
        result.SetNull();
    }
    return result;
}

void Object::AddFunction(Function* f)
{
    SetSlot(f->GetName(), f);
}

void Object::AddProperty(Property* p)
{
    // Without forcewrite a property write will fail if 
    // there is a previous property with the same name some where in the lookup chain.
    SetSlot(p->Name(), p, Slot::ATTR_forcewrite); 
}

void Object::SetType(Type* t)
{
    if (!t)
    {
        type = 0;
        return;
    }
    WriteBarrier(type = t);
}

bool Object::SetSlot(const Value& key, Value& val, u4 attr)
{
    if (!CanSetSlot(key))
    {
        if (!(attr & Slot::ATTR_forcewrite))
        {
            return false;
        }
        else
        {
            attr &= ~Slot::ATTR_forcewrite;
        }
    }
    if (key.IsCollectible())
        WriteBarrier(key);
    if (val.IsCollectible())
        WriteBarrier(val);
    if (!members.Set(key, val, attr))
    {
        return false;
    }
    return true;
}

bool Object::GetSlot(const Value& key, Value& result)
{
    if (!members.Get(key, result))
    {
        if (!type || !type->GetField(key, result))
        {
            return false;
        }
    }
    return true;
}

bool Object::HasSlot(const Value& key)
{
    return members.Exists(key);
}

bool Object::DeleteSlot(const Value& key)
{
    return members.Remove(key);
}

bool Object::CanSetSlot(const Value& key)
{
    Table::ESlotState ss = members.CanSet(key);
    switch (ss)
    {
    case Table::SS_yes: return true;
    case Table::SS_no:  return false;
    case Table::SS_nil: return (type && !type->CanSetField(key)) ? false : true;
    };
    return true;
}

bool Object::BracketRead(const Value& key, Value& result)
{
    return GetSlot(key, result); 
}

bool Object::BracketWrite(const Value& key, Value& val, u4 attr)
{ 
    if (key.IsCollectible())
        WriteBarrier(key);
    if (val.IsCollectible())
        WriteBarrier(val);
    return members.Set(key, val, attr | Slot::ATTR_forcewrite);
}

Object* Object::Clone()
{
    Object* newObject = 0;
    GCNEW(engine, Object, newObject, (this));
    return newObject;
}

Enumerator* Object::GetEnumerator(String* enumType)
{
    bool values = false;
    
    if (enumType == engine->values_String)
    {
        values = true;
    }
    else if ((enumType != engine->keys_String) &&
             (enumType != engine->emptyString))
    {
        return Basic::GetEnumerator(enumType);
    }
    
    Enumerator* newEnumerator = CreateSlotEnumerator(engine, values, this, this->members);
    return newEnumerator;
}

void Object::EnterFunctions(RegisterFunction* fns, size_t count, Package* pkg)
{
    if (!pkg) pkg = engine->GetWorld();
    
    GCPAUSE_NORUN(engine);
    
    for (size_t i = 0; i < count; ++i)
    {
        String* funcName = engine->AllocString(fns[i].name);
        Def* fn = Def::CreateWith(engine, funcName, fns[i].code,
                                  fns[i].argc, fns[i].varargs,
                                  fns[i].strict, 0);
                                  
        Function* closure = Function::Create(engine, fn, pkg);
        AddFunction(closure);
    }
}

}// pika

int Object_remove(Context* ctx, Value& self)
{
    u2      argc = ctx->GetArgCount();
    Value*  argv = ctx->GetArgs();
    Object* obj  = self.val.object;
    
    for (u2 a = 0; a < argc; ++a)
    {
        obj->DeleteSlot(argv[a]);
    }
    return 0;
}

static int Object_getEnumerator(Context* ctx, Value& self)
{
    Object* obj = self.val.object;
    String* enumtype = 0;
    u2 argc = ctx->GetArgCount();
    
    if (argc == 1)
    {
        enumtype = ctx->GetStringArg(0);
    }
    else if (argc == 0)
    {
        enumtype = ctx->GetEngine()->emptyString;
    }
    else
    {
        ctx->WrongArgCount();
    }
    
    Enumerator* e = obj->GetEnumerator(enumtype);
    
    if (e)
    {
        ctx->Push(e);
        return 1;
    }
    return 0;
}

static int Object_clone(Context* ctx, Value& self)
{
    Object* obj = self.val.object;
    Object* newobj = obj->Clone();
    
    if (newobj)
    {
        ctx->Push(newobj);
        return 1;
    }
    return 0;
}

static int Object_init(Context* ctx, Value& self)
{
    Object* obj = self.val.object;
    obj->Init(ctx);
    ctx->Push(self);
    return 1;
}

static int Object_toString(Context* ctx, Value& self)
{
    Object* obj = self.val.object;
    String* rep = obj->ToString();
    ctx->Push(rep);
    return 1;
}

static int Object_rawDotRead(Context* ctx, Value& self)
{
    Object* obj = ctx->GetObjectArg(0);
    Value   name = ctx->GetArg(1);
    Value   res(NULL_VALUE);
    if (obj->GetSlot(name, res))
    {
        ctx->Push(res);
        return 1;
    }
    return 0;
}

static int Object_rawDotWrite(Context* ctx, Value& self)
{
    Object* obj = ctx->GetObjectArg(0);
    Value   name = ctx->GetArg(1);
    Value   val  = ctx->GetArg(2);
    Value   res(NULL_VALUE);
    if (obj->SetSlot(name, val))
    {
    }
    return 0;
}

static int Enumerator_rewind(Context* ctx, Value& self)
{
    Enumerator* e = self.GetEnumerator();
    ctx->PushBool(e->Rewind());
    return 1;
}

static int Enumerator_valid(Context* ctx, Value& self)
{
    Enumerator* e = self.GetEnumerator();
    ctx->PushBool(e->IsValid());
    return 1;
}

static int Enumerator_getCurrent(Context* ctx, Value& self)
{
    Enumerator* e = self.GetEnumerator();
    Value curr;
        
    if (e->IsValid())
    {
        e->GetCurrent(curr);
        ctx->Push(curr);
        return 1;
    }
    return 0;
}

static int Enumerator_advance(Context* ctx, Value& self)
{
    Enumerator* e = self.GetEnumerator();
    e->Advance();
    return 0;
}

static int Property_toString(Context* ctx, Value& self)
{
    Engine*   eng    = ctx->GetEngine();
    Property* prop   = self.val.property;
    String*   type   = prop->GetType()->GetName();
    String*   name   = prop->Name();
    String*   result = eng->AllocStringFmt("%s %s", type->GetBuffer(), name->GetBuffer());
    ctx->Push(result);
    return 1;
}

extern int Global_import(Context*, Value&);

void Object_NewFn(Engine* eng, Type* obj_type, Value& res)
{
    Object* obj = Object::StaticNew(eng, obj_type);
    res.Set(obj);
}

extern void InitTypeAPI(Engine*);

void InitObjectAPI(Engine* eng)
{
    Package* Pkg_World = eng->GetWorld();
    
    // Global functions ///////////////////////////////////////////////////////
    
    static RegisterFunction GlobalFunctions[] =
    {
        { "import", Global_import, 0, 1, 0 },
    };
    
    Pkg_World->AddNative(GlobalFunctions, countof(GlobalFunctions));
    
    // Object /////////////////////////////////////////////////////////////////
    
    static RegisterFunction ObjectFunctions[] =
    {
        { "remove",         Object_remove,        0, 1, 0 },
        { "getEnumerator",  Object_getEnumerator, 0, 1, 0 },
        { "clone",          Object_clone,         0, 0, 0 },
        { OPINIT_CSTR,      Object_init,          0, 1, 0 },
        { "toString",       Object_toString,      0, 0, 0 },
    };
    
    static RegisterFunction Object_ClassMethods[] =
    {
        { "rawDotRead",  Object_rawDotRead,  2, 0, 1 },
        { "rawDotWrite", Object_rawDotWrite, 3, 0, 1 },
    };
    
    eng->Object_Type->EnterMethods(ObjectFunctions, countof(ObjectFunctions));
    eng->Object_Type->EnterClassMethods(Object_ClassMethods, countof(Object_ClassMethods));
    
    Pkg_World->SetSlot("Object", eng->Object_Type);
    
    // Type ///////////////////////////////////////////////////////////////////
    
    InitTypeAPI(eng);
    
    // Enumerator /////////////////////////////////////////////////////////////
    
    static RegisterFunction enumFunctions[] =
    {
        { "rewind" , Enumerator_rewind,  0, 0, 0 },
        { "valid?",  Enumerator_valid,   0, 0, 0 },
        { "advance", Enumerator_advance, 0, 0, 0 },
    };
    
    static RegisterProperty enumProperties[] =
    {
        { "value", Enumerator_getCurrent, "getValue", 0, 0 },
    };
    
    eng->Enumerator_Type = Type::Create(eng, eng->AllocString("Enumerator"), eng->Basic_Type, 0, Pkg_World);
    eng->Enumerator_Type->EnterMethods(enumFunctions, countof(enumFunctions));
    eng->Enumerator_Type->EnterProperties(enumProperties, countof(enumProperties));
    eng->Enumerator_Type->SetFinal(true);
    eng->Enumerator_Type->SetAbstract(true);
    Pkg_World->SetSlot("Enumerator", eng->Enumerator_Type);
    
    // Property ///////////////////////////////////////////////////////////////
    static RegisterFunction Property_Functions[] =
    {
        { "toString", Property_toString, 0, 0, 0 },
    };
        
    eng->Property_Type = Type::Create(eng, eng->AllocString("Property"), eng->Basic_Type, 0, Pkg_World);
    eng->Property_Type->EnterMethods(Property_Functions, countof(Property_Functions));
    eng->Property_Type->SetFinal(true);
    eng->Property_Type->SetAbstract(true);
    Pkg_World->SetSlot("Property", eng->Property_Type);
}
