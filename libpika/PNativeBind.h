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
    static int BoundHook(Context* ctx, Value& self);        
    static int StaticBoundHook(Context* ctx, Value& self);
    virtual String* GetDotPath();
    virtual Object* Clone();
    virtual void    MarkRefs(Collector* c);

    virtual Function* BindWith(Value&);

    NativeDef* ndef;
    ClassInfo* info;
};

struct PIKA_API BoundHookedFunction : HookedFunction
{
    PIKA_DECL(BoundHookedFunction, HookedFunction)
public:
    BoundHookedFunction(HookedFunction* hk, Value& self) : HookedFunction(hk), selfobj(self) {}
    explicit BoundHookedFunction(const BoundHookedFunction*);
    virtual ~BoundHookedFunction() {}
    virtual Object* Clone();
    virtual void MarkRefs(Collector* c);
    virtual void BeginCall(Context* ctx);
    Value selfobj;
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
    
    /** Adds a native instance method.
      *
      * usage:
      * Method(&Class::Method, "name")
      *
      * @param meth Pointer to the C++ method.
      * @param name Name of the method, does not need to be the same as the C++ method.
      */
    template<typename AMeth>
    SlotBinder& Method(AMeth meth, const char* name)
    {
        Function* m = HookedFunction::Bind(engine, meth, name, package, class_info);
        object->AddFunction(m);
        return *this;
    }
    
    /** Create an alias between two slots.
      *
      * usage:
      * Alias("aliasName", "name")
      * 
      * @param aliasname Name of the alias
      * @param name      Name of the original slot we are aliasing.
      */
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
    
    /** Adds a native instance method.
      *
      * usage:
      * StaticMethod(&Class::Method, "name")
      * StaticMethod(StaticFunction, "name")
      *
      * @param meth Pointer to the C++ static method or function.
      * @param name Name of the method, does not need to be the same as the C++ method.
      */    
    template<typename AMeth>
    SlotBinder& StaticMethod(AMeth meth, const char* name)
    {
        Function* m = HookedFunction::StaticBind(engine, meth, name, package);
        object->AddFunction(m);
        return *this;
    }
    
    /** Adds a variable argument, native instance method. Should be of the form void MethodName(Context* ctx) {...}.
      *
      * usage:
      * MethodVA(&Class::Method, "name")
      *
      * @param meth Pointer to the C++ method.
      * @param name Name of the method, does not need to be the same as the C++ method.
      */     
    template<typename AMeth>
    SlotBinder& MethodVA(AMeth meth, const char* name)
    {
        Function* m = HookedFunction::BindVA(engine, meth, name, package, class_info);
        object->AddFunction(m);
        return *this;
    }
    
    /** Adds a variable argument static method. Should be of the form void MethodName(Context* ctx) {...}.
      *
      * usage:
      * StaticMethodVA(&Class::Method, "name")
      * StaticMethodVA(StaticFunction, "name")
      *
      * @param meth Pointer to the C++ static method or function.
      * @param name Name of the method, does not need to be the same as the C++ method.
      */ 
    template<typename AMeth>
    SlotBinder& StaticMethodVA(AMeth meth, const char* name)
    {
        Function* m = HookedFunction::StaticBindVA(engine, meth, name, package);
        object->AddFunction(m);
        return *this;
    }
    
    /** Add a read/write property to the object. You can optionally add the name 
      * for the getter and setter methods which will then be added to the object
      * along side the property.
      * 
      * usage:
      * PropertyRW("name", &Class::Reader, "readName", &Class::Writer, "writeName")
      *
      * @param  propName    The name of the property
      * @param  readMeth    Pointer to the properties getter function.
      * @param  readName    Name of the getter function. If null the getter function will not be added.
      * @param  writeMeth   Pointer to the properties setter function.
      * @param  writeName   Name of the setter function. If null the setter function will not be added.
      */
    template<typename AMethRead, typename AMethWrite>
    SlotBinder& PropertyRW(const char* propName,
                           AMethRead   readMeth,
                           const char* readName,    // name of the read function or null
                           AMethWrite  writeMeth,
                           const char* writeName)   // name of the write function or null
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
    
    /** Add a read-only property to the object. You can optionally add the name 
      * for the getter method which will then be added to the object along side
      * the property.
      * 
      * usage:
      * PropertyR("name", &Class::Reader, "readName")
      * 
      * @param  propName    The name of the property
      * @param  rMeth       Pointer to the properties getter function.
      * @param  rName       Name of the getter function. If null the getter function will not be added.
      */
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
    
    /** Add a write-only property to the object. You can optionally add the name 
      * for the setter method which will then be added to the object along side
      * the property.
      *
      * usage:
      * PropertyW("name", &Class::Writer, "writeName")
      * 
      * @param  propName    The name of the property
      * @param  rMeth       Pointer to the properties getter function.
      * @param  rName       Name of the getter function. If null the getter function will not be added.
      */
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
    
    /** Add a constant to the object. The constant will be protected, which means
      * it cannot be modified at runtime.
      *
      * usage:
      * Constant(value, "name")
      * 
      * @param  val      The constant value.
      * @param  propName The name of the constant.
      */
    template<typename AType>
    SlotBinder& Constant(AType val, const char* propName)
    {
        String* name = engine->AllocStringNC(propName);
        object->SetSlot(name, val, Slot::ATTR_protected);
        return *this;
    }
    
    /** Add an internal value to the object. The constant will be internal, which means
      * it cannot be modified, enumerated or deleted.
      *
      * usage:
      * Internal(value, "name")
      *  
      * @param  val      The constant value.
      * @param  propName The name of the constant.
      */    
    template<typename AType>
    SlotBinder& Internal(AType val, const char* propName)
    {
        String* name = engine->AllocStringNC(propName);
        object->SetSlot(name, val, Slot::ATTR_internal | Slot::ATTR_forcewrite);
        return *this;
    }
    
    /** Add a member variable to the object.
      *
      * usage:
      * Member(value, "name")
      *  
      * @param  val      The constant value.
      * @param  propName The name of the constant.
      */    
    template<typename AType>
    SlotBinder& Member(AType val, const char* propName)
    {
        String* name = engine->AllocStringNC(propName);
        object->SetSlot(name, val);
        return *this;
    }
    
    /** Register a native script Function.
      *
      * usage:
      * Register(func_ptr, "name", ArgCount, true, false)
      * 
      * @param  code    Pointer to the Function.
      * @param  cname   The name of the Function.
      * @param  argc    Expected argument count.
      * @param  varargs Function accepts a variable number of arguments.
      * @param  strict  Argument count must be exact.
      */ 
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
    
    /** Register a native script InstanceMethod.
      *
      * usage:
      * RegisterMethod(func_ptr, "name", ArgCount, true, false)
      *  
      * @param  code    Pointer to the function.
      * @param  cname   The name of the InstanceMethod.
      * @param  argc    Expected argument count.
      * @param  varargs InstanceMethod accepts a variable number of arguments.
      * @param  strict  Argument count must be exact.
      */ 
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
    
    /** Register a native script ClassMethod.
      *
      * usage:
      * RegisterClassMethod(func_ptr, "name", ArgCount, true, false)
      *  
      * @param  code    Pointer to the function.
      * @param  cname   The name of the ClassMethod.
      * @param  argc    Expected argument count.
      * @param  varargs ClassMethod accepts a variable number of arguments.
      * @param  strict  Argument count must be exact.
      */ 
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
        
    ClassInfo* class_info;  //!< ClassInfo, used for type checks when a C++ method is called.
    Package* package;       //!< Package all function are located inside.
    Engine* engine;         //!< Pointer to the Engine.
    Basic* object;          //!< Object we are binding slots to.
};

}// pika

#endif
