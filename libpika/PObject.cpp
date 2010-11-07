/*
 *  PObject.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PConfig.h"
#include "PValue.h"
#include "PCollector.h"
#include "PDef.h"
#include "PBasic.h"
#include "PObject.h"
#include "PType.h"
#include "PFunction.h"
#include "PEngine.h"
#include "PContext.h"
#include "PNativeBind.h"
#include "PObjectIterator.h"
#include "PProxy.h"


/////////////////////////////////////////////// pika ///////////////////////////////////////////////

namespace pika {

extern int Global_import(Context*, Value&);

///////////////////////////////////////// ObjectIterator /////////////////////////////////////////

Iterator* CreateSlotEnumerator(Engine* engine, IterateKind kind, Object* self, Table* table)
{
    Iterator* newEnumerator = 0;
    GCNEW(engine, ObjectIterator, newEnumerator, (engine, engine->Iterator_Type, kind, self, table));
    return newEnumerator;
}
////////////////////////////////////////////// Object //////////////////////////////////////////////

PIKA_IMPL(Object)

Object::~Object()
{
#if defined(PIKA_USE_TABLE_POOL) 
    if (members)
        engine->DelTable(members);
#else
    delete members;
#endif
}

Object* Object::StaticNew(Engine* eng, Type* type)
{
    Object* newObject = 0;
    GCNEW(eng, Object, newObject, (eng, type));
    return newObject;
}

void Object::MarkRefs(Collector* c)
{
    if (members) members->DoMark(c);
    if (type)    type->Mark(c);
}

String* Object::ToString()
{
    return String::ConcatSep(this->GetType()->GetName(), engine->AllocStringNC("instance"), ':');
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

Type* Object::GetType() const
{ 
    if (!type)
    {
        RaiseException(Exception::ERROR_type, "Attempt to access type of object failed.\n");
    }
    return type;
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

Value Object::ToValue()
{
    Value v(this);
    return v;
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
    if (!Members().Set(key, val, attr))
    {
        return false;
    }
    return true;
}

bool Object::GetSlot(const Value& key, Value& result)
{
    if (!members || !members->Get(key, result))
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
    return members ? members->Exists(key) : false;
}

bool Object::DeleteSlot(const Value& key)
{
    return members ? members->Remove(key) : false;
}

bool Object::CanSetSlot(const Value& key)
{
    Table::ESlotState ss = members ? members->CanSet(key) : Table::SS_nil;
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
    return Members().Set(key, val, attr | Slot::ATTR_forcewrite);
}

Object* Object::Clone()
{
    Object* newObject = 0;
    GCNEW(engine, Object, newObject, (this));
    return newObject;
}

Iterator* Object::Iterate(String* enumType)
{
    if (!members)
    {
        return Iterator::Create(engine, engine->Iterator_Type);
    }
    
    IterateKind k = IK_default;
    if (enumType == engine->values_String)
    {
        k = IK_values;
    }
    else if (enumType == engine->names_String)
    {
        k = IK_keys;
    }
    Iterator* newEnumerator = CreateSlotEnumerator(engine, k, this, this->members);
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
                                  fns[i].argc, fns[i].flags, 0);
                                  
        Function* closure = Function::Create(engine, fn, pkg);
        AddFunction(closure);
    }
}

void Object::EnterProperties(RegisterProperty* rp, size_t count, Package* pkg)
{
    GCPAUSE_NORUN(engine);
    pkg = pkg ? pkg : engine->GetWorld();
    for (RegisterProperty* propdef = rp; propdef < (rp + count); ++propdef)
    {
        String* strname = engine->AllocString(propdef->name);
        
        // Create the getter function.
        Function* getter = 0;
        if (propdef->getter)
        {
            String* getname = propdef->getterName ?
                              engine->AllocString(propdef->getterName) :
                              engine->emptyString;
                              
            Def* def = Def::CreateWith(engine, getname, propdef->getter,
                                       0, DEF_STRICT, 0);
                                        
            getter = Function::Create(engine, def, pkg);
            
            // Add the getter function if a name was provided.
            if (propdef->getterName)
            {
                AddFunction(getter);
            }
        }
        
        // Create the setter function.
        Function* setter = 0;
        if (propdef->setter)
        {
            String* setname = propdef->setterName ?
                              engine->AllocString(propdef->setterName) :
                              engine->emptyString;
                              
            Def* def = Def::CreateWith(engine, setname, propdef->setter,
                                       1, DEF_STRICT, 0);
                                       
            setter =  Function::Create(engine, def, pkg);
            
            // Add the setter function if a name was provided.
            if (propdef->setterName)
            {
                AddFunction(setter);
            }
        }
        
        Property* prop = 0;
        if (getter)
        {
            if (setter)
            {
                prop = Property::CreateReadWrite(engine, strname, getter, setter);
            }
            else
            {
                prop = Property::CreateRead(engine, strname, getter);
            }
        }
        else if (setter)
        {
            prop = Property::CreateWrite(engine, strname, setter);
        }
        
        if (prop)
        {
            AddProperty(prop);
        }
        else
        {
            ASSERT(false && "property not added");
        }
    }
}

Table& Object::Members(const Table& other)
{
    if (!members)
    {
#if defined(PIKA_USE_TABLE_POOL)    
        members = engine->NewTable(other);
#else
        members = new Table(other);
#endif
    }
    return *members;
}

Table& Object::Members()
{
    if (!members)
    {
#if defined(PIKA_USE_TABLE_POOL)    
        members = engine->NewTable();
#else
        members = new Table();
#endif
    }
    return *members;
}

namespace {

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

int Object_getEnumerator(Context* ctx, Value& self)
{
    Object* obj = self.val.object;
    String* iter_type = 0;
    u2 argc = ctx->GetArgCount();
    
    if (argc == 1)
    {
        iter_type = ctx->GetStringArg(0);
    }
    else if (argc == 0)
    {
        iter_type = ctx->GetEngine()->emptyString;
    }
    else
    {
        ctx->WrongArgCount();
    }
    
    Iterator* e = obj->Iterate(iter_type);
    
    if (e)
    {
        ctx->Push(e);
        return 1;
    }
    return 0;
}

int Object_clone(Context* ctx, Value& self)
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

int Object_init(Context* ctx, Value& self)
{
    Object* obj = self.val.object;
    obj->Init(ctx);
    ctx->Push(self);
    return 1;
}

int Object_toString(Context* ctx, Value& self)
{
    Object* obj = self.val.object;
    String* rep = obj->ToString();
    ctx->Push(rep);
    return 1;
}
// TODO { rawDotRead should return 2 values, a boolean and the slot value }
int Object_rawDotRead(Context* ctx, Value& self)
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

int Object_rawDotWrite(Context* ctx, Value& self)
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

int Object_rawBracketRead(Context* ctx, Value& self)
{
    Object* obj = ctx->GetObjectArg(0);
    Value   name = ctx->GetArg(1);
    Value   res(NULL_VALUE);
    if (obj->BracketRead(name, res))
    {
        ctx->Push(res);
        return 1;
    }
    return 0;
}

int Object_rawBracketWrite(Context* ctx, Value& self)
{
    Object* obj = ctx->GetObjectArg(0);
    Value   name = ctx->GetArg(1);
    Value   val  = ctx->GetArg(2);
    Value   res(NULL_VALUE);
    if (obj->BracketWrite(name, val))
    {
    }
    return 0;
}

int Property_toString(Context* ctx, Value& self)
{
    Engine*   eng    = ctx->GetEngine();
    Property* prop   = self.val.property;
    String*   type   = prop->GetType()->GetName();
    String*   name   = prop->Name();
    String*   result = eng->AllocStringFmt("%s %s", type->GetBuffer(), name->GetBuffer());
    ctx->Push(result);
    return 1;
}

}// namespace

void Object::Constructor(Engine* eng, Type* obj_type, Value& res)
{
    Object* obj = Object::StaticNew(eng, obj_type);
    res.Set(obj);
}

void Object::StaticInitType(Engine* eng)
{
    Package* Pkg_World = eng->GetWorld();
    
    // Global functions ///////////////////////////////////////////////////////
    
    static RegisterFunction GlobalFunctions[] =
    {
        { "import", Global_import, 0, DEF_VAR_ARGS },
    };
    
    Pkg_World->AddNative(GlobalFunctions, countof(GlobalFunctions));
    
    // Object /////////////////////////////////////////////////////////////////
    
    static RegisterFunction ObjectFunctions[] =
    {
        { "remove",         Object_remove,        0, DEF_VAR_ARGS },
        { "iterate",        Object_getEnumerator, 0, DEF_VAR_ARGS },
        { "clone",          Object_clone,         0, 0 },
        { OPINIT_CSTR,      Object_init,          0, DEF_VAR_ARGS },
        { "toString",       Object_toString,      0, 0 },
    };
    
    static RegisterFunction Object_ClassMethods[] =
    {
        { "rawDotRead",  Object_rawDotRead,  2, DEF_STRICT },
        { "rawDotWrite", Object_rawDotWrite, 3, DEF_STRICT },
        { "rawBracketRead",  Object_rawBracketRead,  2, DEF_STRICT },
        { "rawBracketWrite", Object_rawBracketWrite, 3, DEF_STRICT },
    };
    
    eng->Object_Type->EnterMethods(ObjectFunctions, countof(ObjectFunctions));
    eng->Object_Type->EnterClassMethods(Object_ClassMethods, countof(Object_ClassMethods));
    
    Pkg_World->SetSlot("Object", eng->Object_Type);
    
    // Type ///////////////////////////////////////////////////////////////////
    
    Type::StaticInitType(eng);
        
    // Property ///////////////////////////////////////////////////////////////
    
    static RegisterFunction Property_Functions[] =
    {
        { "toString", Property_toString, 0, 0 },
    };
        
    eng->Property_Type = Type::Create(eng, eng->AllocString("Property"), eng->Basic_Type, 0, Pkg_World);
    eng->Property_Type->EnterMethods(Property_Functions, countof(Property_Functions));
    eng->Property_Type->SetFinal(true);
    eng->Property_Type->SetAbstract(true);
    Pkg_World->SetSlot("Property", eng->Property_Type);
    
    Proxy::StaticInitType(eng);
}
}// pika
