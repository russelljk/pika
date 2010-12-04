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
    if (members && members->Exists(key))
        return true;
    Value res(NULL_VALUE);
    if (type->GetField(key, res) && res.tag == TAG_property) {
        // TODO: Check that property.reader.location == type or property.writer.location == type
        return true;
    }
    return false;
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
                                  fns[i].argc, fns[i].flags, 0, fns[i].__doc);
                                  
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
                                       0, DEF_STRICT, 0, propdef->getterdoc);
                                        
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
                                       1, DEF_STRICT, 0, propdef->setterdoc);
                                       
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
                prop = Property::CreateReadWrite(engine, strname, getter, setter, propdef->__doc);
            }
            else
            {
                prop = Property::CreateRead(engine, strname, getter, propdef->__doc);
            }
        }
        else if (setter)
        {
            prop = Property::CreateWrite(engine, strname, setter, propdef->__doc);
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

bool Object::ToBoolean()
{
    return true;
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

PIKA_DOC(Object_remove, "/(:names)"
"\n"
"Removes each of the instance variables named. Any variable name that doesn't exist will be ignored."
);

PIKA_DOC(Object_iterate, "/(set)"
"\n"
"Returns an [Iterator] that will iterate over a subset of elements. Valid values"
" include the empty [String] \"\" for both names and values, \'names\' for names and \'values\' for"
" values."
);

PIKA_DOC(Object_clone, "/()"
"\n"
"Returns a deep copy of this object."
);

PIKA_DOC(Object_init, "/(:v)"
"\n"
"Initializes a new objects. This is a convience method for derived "
"classes and is called automatically by [Type.new]. Do not call directly unless you are overriding [Object.new]."
);

PIKA_DOC(Object_toBoolean, "/()"
"\n"
"Returns a [Boolean] representation of the object.");

PIKA_DOC(Object_toString, "/()"
"\n"
"Returns a [String] representation of the object.");

PIKA_DOC(Object_rawDotRead, "/(obj, key)"
"\n"
"Returns the instance variable named |key| from |obj|. This is equivalent to the operation"
" |obj|.|key| but avoids all overrides and [Property property] reads. If |key| doesn't exist '''null''' will be returned"
"  Typically this function will only be called from inside opGet."
"[[[function opGet(key)\n"
"  return Object.rawDotRead(self, key)\n"
"end]]]"
);

PIKA_DOC(Object_rawDotWrite, "/(obj, key, val)"
"\n"
"Sets the instance variable in |obj| named |key| to the value |val|."
" This is equivalent to the operation |obj|.|key| = |val| but avoids all overrides and [Property property] reads"
" If the |key| doesn't exist it will be created."
" Typically this function will only be called from inside opSet."
"[[[function opSet(key, val)\n"
"  Object.rawDotWrite(self, key, val)\n"
"end]]]"
);

PIKA_DOC(Object_rawBracketRead, "/(obj, key)"
"\n\n"
"Returns the element named |key| from |obj|. This is equivalent to the operation"
" |obj|[|key|] but avoids all overrides and [Property property] reads. If |key| doesn't exist '''null''' will be returned"
"  Typically this function will only be called from inside opGetAt."
"[[[function opGetAt(key)\n"
"  return Object.rawBracketRead(self, key)\n"
"end]]]"
);

PIKA_DOC(Object_rawBracketWrite, "/(obj, key, val)"
"\n"
"Sets the element in |obj| named |key| to the value |val|."
"This is equivalent to the operation |obj|[|key|] = |val| but avoids all overrides and [Property property] reads"
" If the |key| doesn't exist it will be created."
" Typically this function will only be called from inside opSetAt."
"[[[function opSetAt(key, val)\n"
"  Object.rawBracketWrite(self, key, val)\n"
"end]]]"
);

PIKA_DOC(Global_import, "/(:files)"
"\n"
"This function will load both the [Script scripts] and [Module modules] whose file names are provided."
" If the file was already loaded as the result of a previous call to import, the previous import value will be returned."
" All previous imported values can be found in the [Package] [world.imports imports]."
" Failure to import a given file or a circular dependency will result in an exception being raised."
"\n"
" The full path and extension typically do not need to be provided as long as"
" the path the file resides in is inside [imports.os os.paths]."


);

PIKA_DOC(Object_class, "Object is the default [Type.base base] class for all class declarations."
" Instances can have instance variables, properties and methods of their own."
);

void Object::StaticInitType(Engine* eng)
{
    Package* Pkg_World = eng->GetWorld();
    
    // Global functions ///////////////////////////////////////////////////////
    
    static RegisterFunction GlobalFunctions[] =
    {
        { "import", Global_import, 0, DEF_VAR_ARGS, PIKA_GET_DOC(Global_import) },
    };
    
    Pkg_World->AddNative(GlobalFunctions, countof(GlobalFunctions));
    
    // Object /////////////////////////////////////////////////////////////////
    
    static RegisterFunction ObjectFunctions[] =
    {
        { "remove",         Object_remove,        0, DEF_VAR_ARGS, PIKA_GET_DOC(Object_remove) },
        { "iterate",        Object_getEnumerator, 0, DEF_VAR_ARGS, PIKA_GET_DOC(Object_iterate) },
        { "clone",          Object_clone,         0, 0,            PIKA_GET_DOC(Object_clone) },
        { OPINIT_CSTR,      Object_init,          0, DEF_VAR_ARGS, PIKA_GET_DOC(Object_init) },
        { "toString",       Object_toString,      0, 0,            PIKA_GET_DOC(Object_toString) },
    };
    
    static RegisterFunction Object_ClassMethods[] =
    {
        { "rawDotRead",      Object_rawDotRead,      2, DEF_STRICT, PIKA_GET_DOC(Object_rawDotRead) },
        { "rawDotWrite",     Object_rawDotWrite,     3, DEF_STRICT, PIKA_GET_DOC(Object_rawDotWrite) },
        { "rawBracketRead",  Object_rawBracketRead,  2, DEF_STRICT, PIKA_GET_DOC(Object_rawBracketRead) },
        { "rawBracketWrite", Object_rawBracketWrite, 3, DEF_STRICT, PIKA_GET_DOC(Object_rawBracketWrite) },
    };
    
    eng->Object_Type->EnterMethods(ObjectFunctions, countof(ObjectFunctions));
    eng->Object_Type->EnterClassMethods(Object_ClassMethods, countof(Object_ClassMethods));
    eng->Object_Type->SetDoc(eng->AllocStringNC(PIKA_GET_DOC(Object_class)));
    
    Pkg_World->SetSlot("Object", eng->Object_Type);
    
    SlotBinder<Object>(eng, eng->Object_Type)
    .Alias("__iterate", "iterate")
    .Method(&Object::ToBoolean, "toBoolean", PIKA_GET_DOC(Object_toBoolean))
    ;
    
    // Type ///////////////////////////////////////////////////////////////////
    
    Type::StaticInitType(eng);
        
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
    
    SlotBinder<Property>(eng, eng->Property_Type)
    .PropertyR("name", &Property::Name, "getName")
    .PropertyR("reader", &Property::Reader, "getReader")
    .PropertyR("writer", &Property::Writer, "getWriter")
    ;
    
    Proxy::StaticInitType(eng);
}
}// pika
