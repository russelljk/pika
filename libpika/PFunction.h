/*
 * PFunction.h
 * See Copyright Notice in Pika.h
 */
#ifndef PIKA_FUNCTION_HEADER
#define PIKA_FUNCTION_HEADER

#include "PLiteralPool.h"
#ifndef PIKA_DEF_HEADER
#   include "PDef.h"
#endif

namespace pika 
{
/*
    TODO: Seperate Function from NativeFunction.
    TODO: Add NativeClassMethod
    TODO: Add NativeInstanceMethod
*/
class Def;
class Function;
class Package;
class Value;
class LexicalEnv;

class PIKA_API LexicalEnv : public GCObject 
{
protected:
    LexicalEnv(LexicalEnv*, bool close);
public:
    virtual ~LexicalEnv();

    virtual void MarkRefs(Collector* c);

    INLINE void Set(Value* v, size_t l)
    {
        ASSERT( !allocated );
        allocated = false;
        values    = v;
        length    = l;
    }

    void EndCall();

    INLINE bool IsValid()     const { return values != 0; }
    INLINE bool IsAllocated() const { return allocated;   }

    void Allocate();
    void Deallocate();

    static LexicalEnv* Create(Engine*, LexicalEnv*, bool);

    Value* values;    // Local variables. Points to the Context's stack if allocated = false.
    size_t length;    // Number of lexEnv
    bool   allocated; // If true values point to a heap allocated buffer otherwise they point to the Context's stack.
    bool   mustClose; // Do local variables need to survive beyond the function call?
};

class PIKA_API Function : public Object 
{
    PIKA_DECL(Function, Object)
protected:
    Function(Engine*, Type*, Def*, Package*, Function*);
public:
    virtual ~Function();

    /** Create a new Function object.
     *  @param eng      [in] The Engine to create the object in
     *  @param def      [in] The function's definition.
     *  @param loc      [in] The location of this function.
     *  @param parent   [in] Optional parent of the function (for bytecode functions, may be 0.)
     *  @see Def::CreateWith, Def::Create
     */
    static Function* Create(Engine* eng, Def* def, Package* loc, Function* parent = 0);

    virtual void    Init(Context*);
    virtual void    InitWithBody(String* body);
    
    virtual void        MarkRefs(Collector*);
    virtual Object*     Clone();
    virtual String*     ToString();
    INLINE code_t*      GetBytecode()       { return def->GetBytecode(); }
    INLINE Def*         GetDef()            { return def; }
    INLINE Function*    GetParent()         { return parent; }
    INLINE Package*     GetLocation()       { return location; }
    INLINE const Value& GetLiteral(u2 idx)  { return def->literals->Get(idx); }

    String* GetName();
    
    INLINE  bool        MustClose()  const { return def->mustClose; }    
    INLINE  bool        IsNative()   const { return !def->GetBytecode() && def->nativecode; }
    virtual bool        IsFunction() const { return true; }
    
    virtual void        BeginCall(Context*);
    virtual Value       Apply(Value&, Nullable<Array*>);

    void                SetLocals(Context*, Value*);
    
    int                 DetermineLineNumber(code_t* pc);

    virtual Function*   BindWith(Value&);

    bool                IsLocatedIn(Package*);

    virtual String*     GetDotPath();
    
    u4           numDefaults; //!< Default values count.
    Value*       defaults;    //!< Default values for parameters.
    LexicalEnv*  lexEnv;
    Def*         def;
    Function*    parent;
    Package*     location;
};

INLINE void Function::SetLocals(Context*, Value* v)
{
    ASSERT(lexEnv) ;
    ASSERT(!lexEnv->IsAllocated());

    lexEnv->Set(v, def->numLocals);
}

/*  --------------
    InstanceMethod
    --------------
    An instance method that can only be called by instances of the class this
    method belong to (including derived classes.)
*/
class PIKA_API InstanceMethod : public Function 
{
    PIKA_DECL(InstanceMethod, Function)
public:
    InstanceMethod(Engine*, Type*, Function*, Def*, Package*, Type*);
    InstanceMethod(Engine*, Function*, Type*);
    
    virtual ~InstanceMethod();

    virtual Type*   GetClassType() { return classtype; }
    virtual void    BeginCall(Context*);
    virtual void    MarkRefs(Collector*);    
    virtual String* GetDotPath();

    static InstanceMethod* Create(Engine*, Function*, Type*);
    static InstanceMethod* Create(Engine*, Function*, Def*, Package*, Type*);
protected:
    Type* classtype;
};

/*  -----------
    ClassMethod
    -----------
    A method bound to a single Type.
*/
class PIKA_API ClassMethod : public Function 
{
    PIKA_DECL(ClassMethod, Function)
public:
    ClassMethod(Engine*, Type*, Function*, Def*, Package*, Type*);
    virtual ~ClassMethod();
    
    virtual Type*   GetClassType() const { return classtype; }
    virtual void    BeginCall(Context*);
    virtual void    MarkRefs(Collector*);    
    virtual String* GetDotPath();
    
    static ClassMethod* Create(Engine*, Function*, Def*, Package*, Type*);
protected:
    Type* classtype;
};

/*  -----------
    BoundFunction
    -----------
    A function bound to a single object.
*/
class PIKA_API BoundFunction : public Function 
{
    PIKA_DECL(BoundFunction, Function)
public:
    BoundFunction(Engine*, Type*, Function*, Value&);
    virtual ~BoundFunction();

    static BoundFunction*   Create(Engine*, Function*, Value&);
    
    virtual Function* GetBoundFunction() const { return closure;}
    
    virtual Value GetBoundSelf() const { return self; }
    
    virtual void BeginCall(Context*);
    virtual void MarkRefs(Collector*);
protected:
    Function* closure;
    Value self;
};

} // pika

#endif
