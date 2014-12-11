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
#include "PObjectIterator.h"
#include "PParser.h"


namespace pika {
// Enumerates only object of a certain type
struct FilteredIterator : ObjectIterator
{
    FilteredIterator(Engine* eng, Type* typ, IterateKind k, Object* obj, Table* tab, Type* ftype)
            : ObjectIterator(eng, typ, k, obj, tab), filterType(ftype)
    {}

    virtual bool FilterValue(Value& val, Object*)
    {
        return !filterType || filterType->IsInstance(val);
    }
    
    virtual void MarkRefs(Collector* c)
    {
        ObjectIterator::MarkRefs(c);
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
        subtypes(0),
        __doc(0)
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
        subtypes(rhs->subtypes ? (Array*)rhs->subtypes->Clone() : 0),
        __doc(rhs->__doc)
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

Iterator* Type::Iterate(String* iter_type)
{
    Engine* eng = GetEngine();
    {
        GCPAUSE_NORUN(eng);
                
        Type* filter = 0;
        if (iter_type == eng->GetString("methods")) {
            filter = eng->InstanceMethod_Type;
        } else if (iter_type == eng->GetString("classmethods")) {
            filter = eng->ClassMethod_Type;
        } else if (iter_type == eng->GetString("properties")) {
            filter = eng->Property_Type;
        }
        
        if (members && filter)
        {
            FilteredIterator* filter_iter = 0;
            GCNEW(eng, FilteredIterator, filter_iter, (eng, engine->Iterator_Type, IK_default, this, this->members, filter));
            return filter_iter;
        }
    } // GCPAUSE_NORUN
    return ThisSuper::Iterate(iter_type);
}

void Type::MarkRefs(Collector* c)
{
    ThisSuper::MarkRefs(c);
    if (baseType) baseType->Mark(c);
    if (subtypes) subtypes->Mark(c);
    if (__doc) __doc->Mark(c);
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
        RaiseException("cannot create instance of abstract type %s", GetName()->GetBuffer());
    }
    else if (newfn)
    {
        newfn(engine, this, res);
    }
}

String* Type::ToString()
{
    GCPAUSE_NORUN(engine);
    return String::ConcatSep(engine->AllocString("Type"), this->GetDotName(), ':');
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
                                       0, DEF_STRICT, 0, propdef->getterdoc);
                                        
            getter = propdef->unattached ? Function::Create(engine, def, pkg) : 
                                           InstanceMethod::Create(engine, 0, 0, def, pkg, this);
            
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
                                       
            setter =  propdef->unattached ? Function::Create(engine, def, pkg) :
                                            InstanceMethod::Create(engine, 0, 0, def, pkg, this);
            
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

void Type::EnterMethods(RegisterFunction* fns, size_t count, Package* pkg)
{
    GCPAUSE_NORUN(engine);
    pkg = pkg ? pkg : this;
    for (size_t i = 0; i < count; ++i)
    {
        String* methodName = engine->AllocString(fns[i].name);
        Def* def = Def::CreateWith(engine, methodName, fns[i].code,
                                   fns[i].argc, fns[i].flags, 0, fns[i].__doc);
                                   
        Function* closure = InstanceMethod::Create(engine, 0, 0, def, pkg, this);
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
                                   fns[i].argc, fns[i].flags, 0, fns[i].__doc);
                                   
        Function* closure = ClassMethod::Create(engine, 0, 0, def, pkg, this);
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
        RaiseException("class '%s' is final and cannot be subclassed.", base->GetName()->GetBuffer());
    }
    Type* obj;
    GCNEW(eng, Type, obj, (eng, type_, name, base, fn, location));
    return obj;
}

Type* Type::NewType(String* nm, Package* l)
{
    return Type::Create(engine, nm, this, 0, l); 
}

/** Usually called inside a class block this function checks its members->type->superPackage in that order. The baseType
  * is not checked since this type is not derived from baseType, only instances need to read from the baseType.
  */
bool Type::GetGlobal(const Value& key, Value& result)
{
    return GetSlot(key, result);
}

bool Type::SetGlobal(const Value& key, Value& val, u4 attr)
{
    return Package::SetGlobal(key, val, attr);
}

String* Type::GetDoc() { return __doc ?__doc : engine->emptyString; }

void Type::SetDoc(String* s)
{
    __doc = s;
    if (__doc)
        WriteBarrier(__doc);
}

void Type::SetDoc(const char* cstr)
{
    if (!cstr) {
        __doc = 0;
    } else {
        SetDoc(engine->GetString(cstr));
    }
}

bool Type::CanSetSlot(const Value& key)
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
    if (members && members->Get(key, result))
    {
        return true;
    }
    return baseType ? baseType->GetField(key, result) : false;
}

bool Type::CanSetField(const Value& key)
{
    if (members)
    {
        Table::ESlotState ss = members->CanSet(key);
        if (Table::SS_no == ss)
        {
            return false;
        }
    }
    return baseType ? baseType->CanSetField(key) : true;
}

PIKA_DOC(Type_addMethod, "/(func)\
\n\
Adds the function, |func|, as a [InstanceMethod] of this type. The method will \
contain the same name as |func|. However, if the |func| is a native function it \
will instead be added as a regular function.\
")

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
        Function* im = InstanceMethod::Create(engine, 0, f, this);
        AddFunction(im);
    }
}

PIKA_DOC(Type_addClassMethod, "/(func)\
\n\
Adds the function, |func|, as a [ClassMethod] of this type. The method will \
contain the same name as |func|. However, if the |func| is a native function it \
will instead be added as a regular function.\
")

void Type::AddClassMethod(Function* f)
{
    if (f->IsNative())
    {
        // Native C++ functions/methods do not have dynamic binding to instance variables & methods so
        // we cannot create an InstanceMethod from this function.
        AddFunction(f);
    }
    else
    {
        // Script functions are OK since properties are dynamically bound.
        Function* im = ClassMethod::Create(engine, 0, f, this);
        AddFunction(im);
    }
}

Type* Type::CreateWith(Context* ctx, String* body, String* name, Type* base, Package* pkg)
{
    Engine* eng = ctx->GetEngine();
    if (!name) name = eng->emptyString;
    if (!base) base = eng->Object_Type;
    if (!pkg)  pkg  = eng->GetWorld();
    
    Type* type = Type::Create(eng, name, base, base->GetNewFn(), pkg);
    if (type)
    {
        Program* tree = 0;               
        ctx->Push(type);
        
        {   GCPAUSE_NORUN(eng);
            std::auto_ptr<CompileState> cs    ( new CompileState(eng) );
            std::auto_ptr<Parser>       parser( new Parser(cs.get(), 
                                                           body->GetBuffer(),
                                                           body->GetLength()) );
            
            tree = parser->DoParse();
            if (!tree)
                RaiseException(Exception::ERROR_syntax, "Attempt to generate type \"%s\" parse tree failed.\n", name->GetBuffer());
            tree->CalculateResources(0);
            
            if (cs->HasErrors())
                RaiseException(Exception::ERROR_syntax, "Attempt to compile script failed.\n");
            
            tree->GenerateCode();
            
            if (cs->HasErrors())
                RaiseException(Exception::ERROR_syntax, "Attempt to generate code failed.\n");       
            
            Function* entryPoint = Function::Create(eng, 
                                                    tree->def, // Type's body
                                                    type);     // Set the Type the package            
            // Run the script
            ctx->PushNull();        // Self object.
            ctx->Push(entryPoint);  // Class entryPoint function.
        } // GC Pause
        
        ctx->SetupCall(0);  // Call entryPoint with no arguments.
        ctx->Run();         // Call the entryPoint and initialize the Type.        
        ctx->Pop(2);         // [return value] [type]                   
    }   
    return type;
}

PIKA_DOC(Type_new, "/(*varg, **kwarg)"
"\n\n"
"This function is responsible for creating and initializing new instances."
" The [Type Type's] '''init''' function will be called with same arguments. The \
newly created instance is returned.")

int Type_new(Context* ctx, Value& self)
{
    Engine* engine = ctx->GetEngine();
    u2      argc   = ctx->GetArgCount();
    Type*   meta   = self.val.type;
    Dictionary* dict = ctx->GetKeywordArgs();
    /*
        Argument 0 name
        Argument 1 package
        Argument 2 base
        Argument 3 meta
    */
    Value kw(NULL_VALUE);
    Value vobj(NULL_VALUE);
    Value vfunc(NULL_VALUE);
    
    if (dict)
        kw.Set(dict);
      
    
    ctx->CheckStackSpace(argc + 5); // args + null + dict + obj + func + retvalue
    meta->CreateInstance(vobj);
        
    // Type::Create(eng, name, base, base->GetNewFn(), loc, meta);
    
    if (meta->GetField(Value(engine->GetOverrideString(OVR_init)), vfunc))
    {
        ctx->StackAlloc(argc);
        Pika_memcpy(ctx->GetStackPtr() - argc, ctx->GetArgs(), argc * sizeof(Value));
        ctx->PushNull(); // No Variable Argument Array
        ctx->Push(kw); // Dictionary If Present
        
        ctx->Push(vobj);
        ctx->Push(vfunc);
        
        if (ctx->OpApply((u1)argc, 1, 0, false))
        {
            ctx->Run();
        }
    }
    ctx->Push(vobj);
    
    return 1;
}

PIKA_DOC(Type_create, "/(name, base, pkg [, meta])\
\n\
Create a new Type. The |name| is the name of the type. The |base| is the base \
type of the type. The |pkg| is the parent and global scope of the type. \
Optionally |meta| will be the meta type of the class. \
")

int Type_create(Context* ctx, Value& self)
{
    Engine*  eng  = ctx->GetEngine();
    u2 argc = ctx->GetArgCount();
    
    String* name = ctx->GetStringArg(0);
    Type* base = ctx->GetArgT<Type>(1);
    Package* loc  = ctx->GetArgT<Package>(2);
    if (argc == 4) {
        Type* meta = ctx->GetArgT<Type>(3);
        Type* res  = Type::Create(eng, name, base, base->GetNewFn(), loc, meta);
        ctx->Push(res);
        return 1;
    } else if (argc == 3) {
        Type* res  = Type::Create(eng, name, base, base->GetNewFn(), loc);
        ctx->Push(res);
        return 1;        
    } else {
        ctx->WrongArgCount();
    }
    return 0;
}


PIKA_DOC(Type_createWith, "/(body [, name [, base [, pkg ]]])\
\n\
Create a new Type. The |body| is the text of the class body. The |name| is the \
name of the type. The |base| is the base type of the type. If no |base| is \
specified then [Object] will be used. Lastly |pkg| is the parent and global \
scope of the type. If not specified then |pkg| will be [world]. If you want to \
create a type without specify the body use [create].\
")

int Type_createWith(Context* ctx, Value& self)
{
    u2 argc = ctx->GetArgCount();    
    String* body = 0;
    String* name = 0;
    Type* base = 0;
    Package* pkg= 0;
    
    switch (argc)
    {
        case 4: pkg = ctx->GetArgT<Package>(3);
        case 3: base = ctx->GetArgT<Type>(2);
        case 2: name = ctx->GetStringArg(1);
        case 1: body = ctx->GetStringArg(0);          
            break;
        default:
            ctx->WrongArgCount();
            break;
    }
    Type* type = Type::CreateWith(ctx, body, name, base, pkg);
    if (type) {
        ctx->Push(type);
        return 1;
    }
    return 0;
}

void Type::Init(Context* ctx)
{
    ThisSuper::Init(ctx);
    u2 argc = ctx->GetArgCount();
    /*
        Argument 0 name
        Argument 1 package
        Argument 2 base
        Argument 3 meta
    */
    if (argc >= 4)
    {        
        if (baseType)
        {
            RaiseException("Attempt to initialize already initialized Type named: %s\n",name ? name->GetBuffer() : "<anonymous>");
        }
        baseType = ctx->GetArgT<Type>(2);
        
        WriteBarrier(baseType);
        
        if (!newfn)
        {
            newfn = baseType->newfn;
        }
        
        baseType->AddSubtype(this);
    }
}

void Type::Constructor(Engine* eng, Type* obj_type, Value& res)
{
    Object* obj = Type::Create(eng, eng->AllocString(""), 0, 0, obj_type->GetLocation(), obj_type);
    res.Set(obj);
}

PIKA_DOC(Type_getBase, "/()"
"\n"
"Returns the base class of the type."
)

PIKA_DOC(Type_getName, "/()"
"\n"
"Returns the name of the type."
)

PIKA_DOC(Type_isSubtype, "/(type)\
\n\
Returns '''true''' or '''false''' based on whether the argument, |type|, is derived from this type.")

PIKA_DOC(Type_getSubtypes, "/()\
\n\
Returns an [Array array] of all derived types or '''null''', if this type has \
no derived types.\
")

PIKA_DOC(Type_getLocation, "/()\
\n\
Returns the [Package package] that the class was declared in. This package is \
the global scope of the type.\
")

PIKA_DOC(Type_class, "Types serve as the blueprints for their instances. New \
types can be created by a class statement or using the [ClassMethod class methods] \
[create] or [createWith]. Use the methods [addMethod] and \
[addClassMethod] to add methods for instances.")

PIKA_DOC(Type_base, "The base type we inherit from.")
PIKA_DOC(Type_name, "The name of the type.")
PIKA_DOC(Type_location, "The package this type is located in.")
PIKA_DOC(Type_subtypes, "An [Array array] containing all derived types. Will be \
null if no types inherit from this type.")

void Type::StaticInitType(Engine* eng)
{
    Package* Pkg_World = eng->GetWorld();
    SlotBinder<Type>(eng, eng->Type_Type)
    .PropertyR("base", &Type::GetBase,  "getBase", PIKA_GET_DOC(Type_getBase), PIKA_GET_DOC(Type_base))
    .PropertyR("name", &Type::GetName,  "getName", PIKA_GET_DOC(Type_getName), PIKA_GET_DOC(Type_name))
    .Alias("__base", "base")
    .Alias("__name", "name")
    .PropertyR("location", &Type::GetLocation, "getLocation", PIKA_GET_DOC(Type_getLocation), PIKA_GET_DOC(Type_location))
    .PropertyR("subtypes", &Type::GetSubtypes, "getSubtypes", PIKA_GET_DOC(Type_getSubtypes), PIKA_GET_DOC(Type_subtypes))
    .Method(&Type::IsSubtype,       "isSubtype",        PIKA_GET_DOC(Type_isSubtype))
    .Method(&Type::AddMethod,       "addMethod",        PIKA_GET_DOC(Type_addMethod))
    .Method(&Type::AddClassMethod,  "addClassMethod",   PIKA_GET_DOC(Type_addClassMethod))
    ;
    
    static RegisterFunction TypeFunctions[] =
    {
        { "new", Type_new, 0, DEF_VAR_ARGS | DEF_KEYWORD_ARGS, PIKA_GET_DOC(Type_new) },
    };
    
    static RegisterProperty Type_Properties[] = 
    {
        { "new!", Type_new, 0, 0, 0, true, 0, 0 },
    };
    
    static RegisterFunction TypeClassMethods[] =
    {
        { "create",     Type_create,     4, DEF_STRICT,   PIKA_GET_DOC(Type_create) },
        { "createWith", Type_createWith, 4, DEF_VAR_ARGS, PIKA_GET_DOC(Type_createWith) },
    };
    eng->Type_Type->SetDoc(eng->GetString(PIKA_GET_DOC(Type_class)));
    eng->Type_Type->EnterProperties(Type_Properties, countof(Type_Properties));
    eng->Type_Type->EnterMethods(TypeFunctions, countof(TypeFunctions));
    eng->Type_Type->EnterClassMethods(TypeClassMethods, countof(TypeClassMethods));
    Pkg_World->SetSlot("Type", eng->Type_Type);
}

void Type::SetAllocator(Nullable<Type*> t)
{
    if (t)
    {
        newfn = t->newfn;
    }
    else
    {
        newfn = 0;
    }
}

}// pika
