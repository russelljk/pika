/*
 *  PType.cpp
 *  See Copyright Notice in pika.h
 */
#include "PConfig.h"
#include "PEngine.h"
#include "PContext.h"
#include "PString.h"
#include "PProperty.h"
#include "PType.h"
#include "PArray.h"
#include "PFunction.h"
#include "PNativeBind.h"
#include "PObjectEnumerator.h"

namespace pika {

struct FilterEnum : ObjectEnumerator
{
    FilterEnum(Engine* eng, bool values, Basic* obj, Table& tab, Type* ftype)
            : ObjectEnumerator(eng, values, obj, tab), filterType(ftype)
    {}
    
    virtual bool FilterValue(Value& val, Basic*)
    {
        return !filterType || filterType->IsInstance(val);
    }
    
    virtual void MarkRefs(Collector* c)
    {
        ObjectEnumerator::MarkRefs(c);
        if (filterType)
        {
            filterType->Mark(c);
        }
    }
private:
    Type* filterType;
};

/////////////////////////////////////////////// Type ///////////////////////////////////////////////

PIKA_IMPL(Type)

Type::Type(Engine*    eng,
           Type*      obj_type,
           String*    n,
           Type*      base,
           Type_NewFn newFn,
           Package*   loc)
        : Package(eng, obj_type, n, loc),
        baseType(base),
        newfn(newFn),
        final(false),
        abstract(false),
        subtypes(0)
{
    if (baseType)
    {
        if (!newfn)
        {
            newfn = baseType->newfn;
        }
        baseType->AddSubtype(this);
    }
}

Type::Type(const Type* rhs)
        : ThisSuper(rhs),
        baseType(rhs->baseType),
        newfn(rhs->newfn),
        final(rhs->final),
        abstract(rhs->abstract),
        subtypes(rhs->subtypes ? (Array*)rhs->subtypes->Clone() : 0)
{
    if (baseType)
    {
        baseType->AddSubtype(this);
    }
}

Type::~Type() {}

Object* Type::Clone()
{
    GCPAUSE_NORUN(engine);
    Type* obj_type = 0;
    GCNEW(engine, Type, obj_type, (this));
    return obj_type;
}

Enumerator* Type::GetEnumerator(String* enumtype)
{
    Engine* eng = GetEngine();
    {
        GCPAUSE_NORUN(eng);
        
        
        Type* filter = 0;
        if (enumtype == eng->AllocString("methods"))
        {
            filter = eng->InstanceMethod_Type;
        }
        else if (enumtype == eng->AllocString("classmethods"))
        {
            filter = eng->ClassMethod_Type;
        }
        else if (enumtype == eng->AllocString("properties"))
        {
            filter = eng->Property_Type;
        }
        if (filter)
        {
            FilterEnum* filterenum = 0;
            GCNEW(eng, FilterEnum, filterenum, (eng, true, this, this->members, filter));
            return filterenum;
        }
    } // GCPAUSE_NORUN
    return ThisSuper::GetEnumerator(enumtype);
}

void Type::MarkRefs(Collector* c)
{
    ThisSuper::MarkRefs(c);
    if (baseType) baseType->Mark(c);
    if (subtypes) subtypes->Mark(c);
}

bool Type::IsSubtype(Type* stype)
{
    Type* t = stype;
    while (t)
    {
        if (t == this)
        {
            return true;
        }
        t = t->baseType;
    }
    return false;
}

bool Type::IsInstance(Basic* inst)
{
    Type* t = inst->GetType();
    return IsSubtype(t);
}

bool Type::IsInstance(Value& inst)
{
    Type* t = 0;
    
    if (inst.tag >= TAG_basic)
    {
        Basic* obj = inst.val.basic;
        t = obj->GetType();
    }
    else
    {
        switch (inst.tag)
        {
        case TAG_null:    t = engine->Null_Type;    break;
        case TAG_boolean: t = engine->Boolean_Type; break;
        case TAG_integer: t = engine->Integer_Type; break;
        case TAG_real:    t = engine->Real_Type;    break;
        default:
            return false;
        }
    }
    return IsSubtype(t);
}

void Type::CreateInstance(Value& res)
{
    if (IsAbstract())
    {
        RaiseException("cannot create instance of type %s", GetName()->GetBuffer());
    }
    else if (newfn)
    {
        newfn(engine, this, res);
    }
}

String* Type::ToString()
{
    GCPAUSE_NORUN(engine);
    return String::ConcatSpace(engine->AllocString("Type"), this->GetDotName());
}

void Type::EnterProperties(RegisterProperty* rp, size_t count, Package* pkg)
{
    GCPAUSE_NORUN(engine);
    pkg = pkg ? pkg : this;
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
                                       0, false, true, 0);
                                       
            getter = InstanceMethod::Create(engine, 0, def, pkg, this);
            
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
                                       1, false, true, 0);
                                       
            setter = InstanceMethod::Create(engine, 0, def, pkg, this);
            
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

void Type::EnterMethods(RegisterFunction* fns, size_t count, Package* pkg)
{
    GCPAUSE_NORUN(engine);
    pkg = pkg ? pkg : this;
    for (size_t i = 0; i < count; ++i)
    {
        String* methodName = engine->AllocString(fns[i].name);
        Def* def = Def::CreateWith(engine, methodName, fns[i].code,
                                   fns[i].argc, fns[i].varargs,
                                   fns[i].strict, 0);
                                   
        Function* closure = InstanceMethod::Create(engine, 0, def, pkg, this);
        AddFunction(closure);
    }
}

void Type::EnterClassMethods(RegisterFunction* fns, size_t count, Package* pkg)
{
    GCPAUSE_NORUN(engine);
    pkg = pkg ? pkg : this;
    for (size_t i = 0; i < count; ++i)
    {
        String* methodName = engine->AllocString(fns[i].name);
        Def* def = Def::CreateWith(engine, methodName, fns[i].code,
                                   fns[i].argc, fns[i].varargs,
                                   fns[i].strict, 0);
                                   
        Function* closure = ClassMethod::Create(engine, 0, def, pkg, this);
        AddFunction(closure);
    }
}

Type* Type::Create(Engine* eng, String* name, Type* base, Type_NewFn fn, Package* location)
{
    GCPAUSE_NORUN(eng);
    ASSERT(eng->Type_Type);
    Type*   meta_obj   = 0;
    String* meta_name  = String::Concat(name, eng->AllocString("-Type"));
    Type*   meta_super = base ? base->GetType() : eng->Type_Type;
    Type*   meta_type  = eng->Type_Type->GetType();
    
    // Create the metatype
    GCNEW(eng, Type, meta_obj, (eng, meta_type, meta_name, meta_super, meta_type ? meta_type->newfn : 0, location));
    return Create(eng, name, base, fn, location, meta_obj);
}

Type* Type::Create(Engine* eng, String* name, Type* base, Type_NewFn fn, Package* location, Type* type_)
{
    GCPAUSE_NORUN(eng);
    if (base && base->IsFinal())
    {
        RaiseException("class '%s' cannot be subclassed.", base->GetName()->GetBuffer());
    }
    Type* obj;
    GCNEW(eng, Type, obj, (eng, type_, name, base, fn, location));
    return obj;
}

bool Type::GetGlobal(const Value& key, Value& result) {return GetSlot(key, result);}

bool Type::SetGlobal(const Value& key, Value& val, u4 attr) {return SetSlot(key, val, attr);}

void Type::AddSubtype(Type* subtype)
{
    GCPAUSE_NORUN(engine);
    if (!subtypes)
    {
        subtypes = Array::Create(engine, engine->Array_Type, 0, 0);
        WriteBarrier(subtypes);
    }
    Value v(subtype);
    subtypes->Push(v);
}

bool Type::GetField(const Value& key, Value& result)
{
    if (members.Get(key, result))
    {
        return true;
    }
    return baseType ? baseType->GetField(key, result) : false;
}

bool Type::CanSetField(const Value& key)
{
    if (!members.CanSet(key))
    {
        return false;
    }
    return baseType ? baseType->CanSetField(key) : true;
}

void Type::AddMethod(Function* f)
{
    if (f->IsNative())
    {
        // Native C++ functions/methods do have dynamic binding to instance variables/methods so
        // we cannot create an InstanceMethod from this function.
        AddFunction(f);
    }
    else
    {
        // Script functions are OK since properties are dynamically bound.
        Function* im = InstanceMethod::Create(engine, f, this);
        AddFunction(im);
    }
}

void Type::AddClassMethod(Function* f)
{
    if (f->IsNative())
    {
        // Native C++ functions/methods do have dynamic binding to instance variables/methods so
        // we cannot create an InstanceMethod from this function.
        AddFunction(f);
    }
    else
    {
        // Script functions are OK since properties are dynamically bound.
        Function* im = ClassMethod::Create(engine, f, this);
        AddFunction(im);
    }
}

}// pika

static int Type_alloc(Context* ctx, Value& self)
{
    Engine* engine = ctx->GetEngine();
    u2      argc   = ctx->GetArgCount();
    Type*   type   = self.val.type;
    
    Value vobj(NULL_VALUE);
    Value vfunc(NULL_VALUE);
    
    type->CreateInstance(vobj);
    if (type->GetField(Value(engine->GetOverrideString(OVR_init)), vfunc))
    {
        ctx->StackAlloc(argc);
        Pika_memcpy(ctx->GetStackPtr() - argc, ctx->GetArgs(), argc * sizeof(Value));
        ctx->Push(vobj);
        ctx->Push(vfunc);
        
        if (ctx->SetupCall(argc, false, 1))
        {
            ctx->Run();
        }
    }
    ctx->Push(vobj);
    return 1;
}

static int Type_create(Context* ctx, Value& self)
{
    Engine*  eng  = ctx->GetEngine();
    String*  name = ctx->GetStringArg(0);
    Type*    base = ctx->GetArgT<Type>(1);
    Package* loc  = ctx->GetArgT<Package>(2);
    Type*    meta = ctx->GetArgT<Type>(3);
    Type*    res  = Type::Create(eng, name, base, base->GetNewFn(), loc, meta);
    
    ctx->Push(res);
    return 1;
}

void InitTypeAPI(Engine* eng)
{
    Package* Pkg_World = eng->GetWorld();
    SlotBinder<Type>(eng, eng->Type_Type)
    .PropertyR("base",     &Type::GetBase,     "getBase")
    .PropertyR("name",     &Type::GetName,     "getName")
    .PropertyR("location", &Type::GetLocation, "getLocation")
    .PropertyR("subtypes", &Type::GetSubtypes, "getSubtypes")
    .Method(&Type::AddMethod, "addMethod")
    .Method(&Type::AddClassMethod, "addClassMethod")
    ;
    
    static RegisterFunction TypeFunctions[] =
    {
        { "newInstance", Type_alloc,  0, 1, 0 },
    };
    
    static RegisterFunction TypeClassMethods[] =
    {
        { "create", Type_create, 4, 0, 1 },
    };
    
    eng->Type_Type->EnterMethods(TypeFunctions, countof(TypeFunctions));
    eng->Type_Type->EnterClassMethods(TypeClassMethods, countof(TypeClassMethods));
    Pkg_World->SetSlot("Type", eng->Type_Type);
}
