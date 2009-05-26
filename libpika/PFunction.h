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

namespace pika {
class Def;
class Function;
class Package;
class Value;
class LexicalEnv;

class PIKA_API LexicalEnv : public GCObject 
{
protected:
    LexicalEnv(bool close);
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
    
    INLINE size_t Length() const { return length; }
    INLINE Value& At(size_t idx) { return values[idx]; }
    INLINE const Value& At(size_t idx) const { return values[idx]; }
            
    void Allocate();
    void Deallocate();
        
    static LexicalEnv* Create(Engine*, bool);
private:    
    Value* values;    //!< Local variables. Points to the Context's stack if allocated is false.
    size_t length;    //!< Number of lexEnv
    bool   allocated; //!< If true values point to a heap allocated buffer otherwise they point to the Context's stack.
    bool   mustClose; //!< Do local variables need to survive beyond the function call?
};

class PIKA_API Defaults : public GCObject
{
public:
    Defaults(Value* v, size_t l);    
    virtual void MarkRefs(Collector* c);
        
    virtual ~Defaults();
    
    static Defaults* Create(Engine*, Value*, size_t);
    Value* values;    //!< Local variables. Points to the Context's stack if allocated is false.
    size_t length;    //!< Number of lexEnv
};

class PIKA_API Function : public Object 
{
    PIKA_DECL(Function, Object)
protected:
    Function(Engine*, Type*, Def*, Package*, Function*);
public:
    virtual            ~Function();
    
    /** Create a new Function object.
      * @param eng      [in] The Engine to create the object in
      * @param def      [in] The function's definition.
      * @param loc      [in] The location of this function.
      * @param parent   [in] Optional parent of the function (for bytecode functions, may be 0.)
      * @see Def::CreateWith, Def::Create
      */
    static Function*    Create(Engine* eng, Def* def, Package* loc, Function* parent = 0);
    static Function*    Create(Engine* eng, RegisterFunction* rf, Package* loc);
    virtual void        Init(Context*);
    virtual void        InitWithBody(String* body);
    
    virtual void        MarkRefs(Collector*);
    virtual Object*     Clone();
    virtual String*     ToString();
    INLINE code_t*      GetBytecode()       { return def->GetBytecode(); }
    INLINE Def*         GetDef()            { return def; }
    INLINE Function*    GetParent()         { return parent; }
    INLINE Package*     GetLocation()       { return location; }
    INLINE const Value& GetLiteral(u2 idx)  { return def->literals->Get(idx); }
    String*             GetName();
    
    INLINE  Value*      DefaultValue(size_t at) { return defaults__ ? defaults__->values+at : 0; }
    INLINE  size_t      DefaultsCount() const { return defaults__ ? defaults__->length : 0; }
    INLINE  bool        MustClose()  const { return def->mustClose; }    
    INLINE  bool        IsNative()   const { return !def->GetBytecode() && def->nativecode; }
    
    virtual void        BeginCall(Context*);    
    virtual Value       Apply(Value&, Array*);   
    int                 DetermineLineNumber(code_t* pc);
    virtual Function*   BindWith(Value&);
    bool                IsLocatedIn(Package*);
    virtual String*     GetDotPath();
    
    Defaults*    defaults__;
    LexicalEnv*  lexEnv;      //!< Lexical environment for this function. Basically the parent's locals.
    Def*         def;         //!< Function definition, may be shared with other functions.
    Function*    parent;      //!< The parent function we are defined inside (for nested functions only).
    Package*     location;    //!< Package we are declared inside of.
};

/** A function callable only by instances of the same type. */
class PIKA_API InstanceMethod : public Function 
{
    PIKA_DECL(InstanceMethod, Function)
protected:
    InstanceMethod(Engine*, Type*, Function*, Def*, Package*, Type*);
    InstanceMethod(Engine*, Function*, Type*);
public:    
    virtual ~InstanceMethod();
    
    virtual Type*   GetClassType() { return classtype; }
    virtual void    BeginCall(Context*);
    virtual void    MarkRefs(Collector*);    
    virtual String* GetDotPath();
    
    static InstanceMethod* Create(Engine*, Function*, Type*);
    static InstanceMethod* Create(Engine*, Function*, Def*, Package*, Type*);
protected:
    Type* classtype; //!< The type we belong to and whose instances may call us.
};

/** A method bound to a single Type instance. */
class PIKA_API ClassMethod : public Function 
{
    PIKA_DECL(ClassMethod, Function)
protected:
    ClassMethod(Engine*, Type*, Function*, Def*, Package*, Type*);
    ClassMethod(Engine*, Function*, Type*);    
public:
    virtual ~ClassMethod();
    
    virtual Type*   GetClassType() const { return classtype; }
    virtual void    BeginCall(Context*);
    virtual void    MarkRefs(Collector*);    
    virtual String* GetDotPath();
    
    static ClassMethod* Create(Engine*, Function*, Type*);    
    static ClassMethod* Create(Engine*, Function*, Def*, Package*, Type*);
protected:
    Type* classtype; //!< The type we belong to and the self object we are bound to.
};

/** A function bound to a single object. */
class PIKA_API BoundFunction : public Function 
{
    PIKA_DECL(BoundFunction, Function)
protected:
    BoundFunction(Engine*, Type*, Function*, Value&);
public:    
    virtual              ~BoundFunction();
    
    static BoundFunction* Create(Engine*, Function*, Value&);
    
    virtual Function*     GetBoundFunction() const { return closure;}
    virtual Value         GetBoundSelf() const { return self; }
    
    virtual void          BeginCall(Context*);
    virtual void          MarkRefs(Collector*);
protected:
    Function*   closure; //!< The bound function.
    Value       self;    //!< The bound <code>self</code> object.
};

} // pika

#endif
