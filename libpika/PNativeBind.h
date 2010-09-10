/*
 *  PNativeBind.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_NATIVEBIND_HEADER
#define PIKA_NATIVEBIND_HEADER

#include "PString.h"
#include "PEngine.h"
#include "PProperty.h"
#include "PPackage.h"
#include "PNativeMethod.h"

namespace pika {

struct NativeMethodBase;

/** The struct NativeMethodBase wrapped so that the GC can manage its lifetime.
  */
struct PIKA_API NativeDef : GCObject
{
    NativeDef(NativeMethodBase* d);

    virtual ~NativeDef();

    NativeMethodBase* def;
    int retc;
};

/** A Function object that directly binds a native C/C++ function, instance method  or
  * class method to its script equivalent. 
  */
struct PIKA_API HookedFunction : Function
{
    PIKA_DECL(HookedFunction, Function)
public:
    explicit HookedFunction(Engine*    eng,
                            Type*      ptype,
                            Def*       mdef,
                            NativeDef* def,
                            ClassInfo* info,
                            Package*   pkg);
    
    explicit HookedFunction(const HookedFunction*);
    
    virtual ~HookedFunction();
    
    static HookedFunction* Create(Engine*           eng,    // Engine we belong to.
                                  Type*             type,   // Instance|Class Type we belong to.
                                  Def*              mdef,   // Definition
                                  NativeMethodBase* def,    // Native Definition
                                  ClassInfo*        info,   // ClassInfo of the native class.
                                  Package*          pkg);   // Package we reside in.
    
    /** Create a fixed argument HookedFunction. */
    template<typename AMeth>
    static HookedFunction* Bind(Engine*     eng,
                                AMeth       meth,
                                const char* name,
                                Package*    package,
                                ClassInfo*  info)
    {
        GCPAUSE_NORUN(eng);

        Package* world = eng->GetWorld();
        NativeMethodBase* def = MakeMethod(meth);

        Def* mdef = Def::CreateWith(eng, (name ? eng->AllocStringNC(name) : eng->emptyString),
            HookedFunction::Hook, def->GetArgCount(), DEF_STRICT, 0);

        HookedFunction* bm = Create(eng, eng->NativeMethod_Type, mdef, def, info, package ? package : world);
        return bm;
    }

    /** Create a static fixed argument HookedFunction. */
    template<typename AMeth>
    static HookedFunction* StaticBind(Engine*     eng,
                                      AMeth       meth,
                                      const char* name,
                                      Package*    package)
    {
        GCPAUSE_NORUN(eng);

        Package* world = eng->GetWorld();
        NativeMethodBase* def = MakeStaticMethod(meth);        
        Def* mdef = Def::CreateWith(eng, (name ? eng->AllocStringNC(name) : eng->emptyString),
            HookedFunction::StaticHook, def->GetArgCount(), DEF_STRICT, 0);
        HookedFunction* bm = Create(eng, eng->NativeFunction_Type, mdef, def, 0, package ? package : world);
        return bm;
    }

    /** Create a static variable argument HookedFunction. */
    template<typename AMeth>
    static HookedFunction* StaticBindVA(Engine*     eng,
                                        AMeth       meth,
                                        const char* name,
                                        Package*    package)
    {
        GCPAUSE_NORUN(eng);

        Package* world = eng->GetWorld();
        NativeMethodBase* def = MakeStaticMethodVA(meth);
        
        Def* mdef = Def::CreateWith(eng, (name ? eng->AllocStringNC(name) : eng->emptyString),
            HookedFunction::StaticHook, def->GetArgCount(), DEF_VAR_ARGS, 0);

        HookedFunction* bm = Create(eng, eng->NativeFunction_Type, mdef, def, 0, package ? package : world);
        return bm;
    }

    /** Create a variable argument HookedFunction. */
    template<typename AMeth>
    static HookedFunction* BindVA(Engine*     eng,
                                  AMeth       meth,
                                  const char* name,
                                  Package*    package,
                                  ClassInfo*  info)
    {
        GCPAUSE_NORUN(eng);

        Package* world = eng->GetWorld();
        NativeMethodBase* def = MakeFunctionVA(meth);
        
        Def* mdef = Def::CreateWith(eng, (name ? eng->AllocStringNC(name) : eng->emptyString),
            HookedFunction::Hook, def->GetArgCount(), DEF_VAR_ARGS, 0);

        HookedFunction* bm = Create(eng, eng->NativeMethod_Type, mdef, def, info, package ? package : world);
        return bm;
    }
    
    /** Call this function using the given C++ object 'obj'. 
      * Type checking must be done before the function is called and under most circumstances 
      * you should not directly call this function. 
      *
      * @see HookedFunction::StaticHook, HookedFunction::Hook
      */
    virtual void Invoke(void* obj, Context* ctx);
    
    /** Number of values this function will return.
      * @note Should be 1 or 0 since it binds to a native C/C++ method.
      */
    INLINE int GetRetCount() const { return ndef->retc; }
    
    static int StaticHook(Context* ctx, Value& self);
    static int Hook(Context* ctx, Value& self);    
    
    virtual String* GetDotPath();
    virtual Object* Clone();
    virtual void    MarkRefs(Collector* c);

    virtual Function* BindWith(Value&);

    NativeDef* ndef;
    ClassInfo* info;
};
/** Binds Functions,Properties and Variables to an Object.
  * requirements:
  *
  * The type AClass must has the static method StaticGetClass, most commonly obtained by
  * Adding PIKA_DECL and PIKA_IMPL to the class body and source file repspectively.
  *  
  * The Object or Type must already be created. 
  *
  * You should also stop the Collector before adding slots since multiple objects may be allocated.
  *
  * usage:
  * 
  * Package* pkg = engine->GetWorld();
  * Type* Foo_Type = Type::Create(engine, engine->AllocStringNC("Foo"), Foo_Base_Type, Foo::Constructor, pkg);
  * 
  * SlotBinder<Foo>(engine, Foo_Type, pkg)
  * .Method( &Foo::Bar, "bar")
  * .PropertyRW( "length",
  *     &Foo::GetLength, "getLength",
  *     &Foo::SetLength, "setLength")
  * ;
  *
  * Replace Foo_Base_Type with a pointer to the base class's Type object. Replace Foo::Constructor with the function that
  * contructs new instances.
  */
template<typename AClass>
struct SlotBinder
{
    SlotBinder(Engine* eng, Basic* obj, Package* pkg = 0)
            : class_info(AClass::StaticGetClass()),
            package(pkg ? pkg : eng->GetWorld()),
            engine(eng),
            object(obj)            
    {}
    
    /** Adds a native instance method. HookedFunction takes care of type checking.
      */
    template<typename AMeth>
    SlotBinder& Method(AMeth meth, const char* name)
    {
        Function* m = HookedFunction::Bind(engine, meth, name, package, class_info);
        object->AddFunction(m);
        return *this;
    }
    
    SlotBinder& Alias(const char* aliasname, const char* name)
    {
        String* name_String = engine->AllocStringNC(name);
        String* aliasname_String = engine->AllocStringNC(aliasname);

        Value res;
        if (object->GetSlot(name_String, res))
        {
            object->SetSlot(aliasname_String, res);
        }
        return *this;
    }
    
    template<typename AMeth>
    SlotBinder& StaticMethod(AMeth meth, const char* name)
    {
        Function* m = HookedFunction::StaticBind(engine, meth, name, package);
        object->AddFunction(m);
        return *this;
    }
    
    template<typename AMeth>
    SlotBinder& MethodVA(AMeth meth, const char* name)
    {
        Function* m = HookedFunction::BindVA(engine, meth, name, package, class_info);
        object->AddFunction(m);
        return *this;
    }
    
    template<typename AMeth>
    SlotBinder& StaticMethodVA(AMeth meth, const char* name)
    {
        Function* m = HookedFunction::StaticBindVA(engine, meth, name, package);
        object->AddFunction(m);
        return *this;
    }
    
    template<typename AMethRead, typename AMethWrite>
    SlotBinder& PropertyRW(const char* propName,
                           AMethRead   readMeth,
                           const char* readName,    // name of the read function (ie GetXXXX ) or null
                           AMethWrite  writeMeth,
                           const char* writeName)   // name of the write function (ie SetXXXX ) or null
    {
        String*   name = engine->AllocStringNC(propName);
        Function* rm   = HookedFunction::Bind<AMethRead>(engine,  readMeth,  readName  ? readName  : "", package, class_info);
        Function* wm   = HookedFunction::Bind<AMethWrite>(engine, writeMeth, writeName ? writeName : "", package, class_info);
        Property* p    = Property::CreateReadWrite(engine, name, rm, wm);
        
        if (readName)
            object->AddFunction(rm);

        if (writeName)
            object->AddFunction(wm);

        object->AddProperty(p);
        return *this;
    }
    
    template<typename AMethRead>
    SlotBinder& PropertyR(const char* propName,
                          AMethRead   rMeth,
                          const char* rName)
    {
        String*   name  = engine->AllocStringNC(propName);
        Function* rmeth = HookedFunction::Bind<AMethRead>(engine, rMeth, rName ? rName : "", package, class_info);
        Property* prop  = Property::CreateRead(engine, name, rmeth);

        if (rName)
            object->AddFunction(rmeth);

        object->AddProperty(prop);
        return *this;
    }
    
    template<typename AMethWrite>
    SlotBinder& PropertyW(const char* propName,
                          AMethWrite  wMeth,
                          const char* wName)
    {
        String*   name  = engine->AllocStringNC(propName);
        Function* wmeth = HookedFunction::Bind<AMethWrite>(engine, wMeth, wName ? wName : "", package, class_info);
        Property* prop  = Property::CreateWrite(engine, name, wmeth);

        if (wName)
            object->AddFunction(wmeth);

        object->AddProperty(prop);
        return *this;
    }
    
    template<typename AType>
    SlotBinder& Constant(AType val, const char* propName)
    {
        String* name = engine->AllocStringNC(propName);
        object->SetSlot(name, val, Slot::ATTR_protected);
        return *this;
    }
    
    template<typename AType>
    SlotBinder& Internal(AType val, const char* propName)
    {
        String* name = engine->AllocStringNC(propName);
        object->SetSlot(name, val, Slot::ATTR_internal | Slot::ATTR_forcewrite);
        return *this;
    }
    
    template<typename AType>
    SlotBinder& Member(AType val, const char* propName)
    {
        String* name = engine->AllocStringNC(propName);
        object->SetSlot(name, val);
        return *this;
    }
    
    SlotBinder& Register(Nativecode_t code,
                         const char*  cname,
                         u2           argc    = 0,
                         bool         varargs = true,
                         bool         strict  = false)
    {
        String* name = engine->AllocStringNC(cname);
        u4 flags = 0;
        if (varargs) flags |= DEF_VAR_ARGS;
        if (strict)  flags |= DEF_STRICT;
        Def* fn = Def::CreateWith(engine, name,
            code, argc, flags, 0);

        Function* closure = Function::Create(engine, fn, package);
        object->AddFunction(closure);
        return *this;
    }
    
    SlotBinder& RegisterMethod(Nativecode_t code,
                         const char*  cname,
                         u2           argc    = 0,
                         bool         varargs = true,
                         bool         strict  = false)
    {
        if (!object->IsDerivedFrom(Type::StaticGetClass()))
            return Register(code, cname, argc, varargs, strict);
        
        u4 flags = 0;
        if (varargs) flags |= DEF_VAR_ARGS;
        if (strict)  flags |= DEF_STRICT;
                
        String* name = engine->AllocStringNC(cname);        
        Def* fn = Def::CreateWith(engine, name,
            code, argc, flags, 0);

        Function* closure = InstanceMethod::Create(engine, 0, 0, fn, package, (Type*)object);
        object->AddFunction(closure);
        return *this;
    }
    
    SlotBinder& RegisterClassMethod(Nativecode_t code,
                         const char*  cname,
                         u2           argc    = 0,
                         bool         varargs = true,
                         bool         strict  = false)
    {
        if (!object->IsDerivedFrom(Type::StaticGetClass()))
            return Register(code, cname, argc, varargs, strict);
        
        u4 flags = 0;
        if (varargs) flags |= DEF_VAR_ARGS;
        if (strict)  flags |= DEF_STRICT;
                
        String* name = engine->AllocStringNC(cname);        
        Def* fn = Def::CreateWith(engine, name,
            code, argc, flags, 0);

        Function* closure = ClassMethod::Create(engine, 0, 0, fn, package, (Type*)object);
        object->AddFunction(closure);
        return *this;
    }
        
    ClassInfo* class_info;
    Package*   package;
    Engine*    engine;
    Basic*    object;
};

}// pika

#endif
