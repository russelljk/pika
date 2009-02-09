/*
 * PContext.h
 * See Copyright Notice in Pika.h
 */
#ifndef PIKA_CONTEXT_HEADER
#define PIKA_CONTEXT_HEADER

#ifndef PIKA_BUFFER_HEADER
#   include "PBuffer.h"
#endif

#ifndef PIKA_OBJECT_HEADER
#   include "PObject.h"
#endif

#ifndef PIKA_OPCODE_HEADER
#   include "POpcode.h"
#endif

#ifndef PIKA_FUNCTION_HEADER
#   include "PFunction.h"
#endif

namespace pika
{
class LexicalEnv;
class Function;
class Package;
class Value;
class Context;
struct UserDataInfo;

enum ScopeKind
{
    SCOPE_call,
    SCOPE_with,
    SCOPE_package,
};

struct ScopeInfo
{
    void DoMark(Collector*);
    
    LexicalEnv*  env;
    Function*    closure;
    Package*     package;
    Value        self;
    code_t*      pc;
    size_t       stackTop;
    size_t       stackBase;
    u4           argCount;
    u4           retCount;
    u4           numTailCalls;
    bool         newCall;
    ScopeKind    kind;
    LiteralPool* literals;
};

struct ExceptionBlock
{
    void DoMark(Collector*);
    
    Package* package;
    Value    self;
    size_t   scope;
    size_t   catchpc;
    size_t   sp;
};

PIKA_NODESTRUCTOR(ScopeInfo)
PIKA_NODESTRUCTOR(ExceptionBlock)

#if defined(PIKA_DLL) && defined(_WIN32)
template class PIKA_API Buffer<size_t>;
template class PIKA_API Buffer<ExceptionBlock>;
template class PIKA_API Buffer<ScopeInfo>;
template class PIKA_API Buffer<ScopeInfo>::Iterator;
#endif

typedef Buffer<size_t>          AddressStack;
typedef Buffer<ScopeInfo>       ScopeStack;
typedef ScopeStack::Iterator    ScopeIter;
typedef Buffer<ExceptionBlock>  ExceptionStack;

class ContextEnum;

/** A coroutine object.
  * Unlike the string objects bytes objects can be resized and modified without forcing a new object
  * to be created.
  */
class PIKA_API Context : public Object
{
    PIKA_DECL(Context, Object)
protected:
    friend class Engine;
    friend class ContextEnum;
    Context(Engine*, Type*);
public:
    virtual ~Context();
    
    virtual Enumerator* GetEnumerator(String*);
    virtual Object*     Clone();
    virtual void        MarkRefs(Collector*);
    
    virtual bool ToBoolean() const { return state == SUSPENDED || state == RUNNING; }
    
    enum EState
    {
        SUSPENDED, //!< Context has yielded control to another Context.
        RUNNING,   //!< Context is running.
        DEAD,      //!< Context has finished running and exited cleanly.
        INVALID,   //!< Context has not been initialized or exited abruptly.
        PREPARED,  //!< Context has been initialized and is ready to run.
    };
        
    enum EErrorResult
    {
        ER_throw,    //!< Context should re-throw the exception. Usually because there is a C++ barrier between the exception and the handler.
        ER_continue, //!< Context should continue executing.
        ER_exit,     //!< Context should stop executing and exit. Usually when the exception originates inside a root Context and no handler is provided.
    };
    
    void ReportRuntimeError(Exception::Kind kind, const char* msg, ...);
protected:
    void MakeInvalid();
    
    int AdjustArgs(Function* fun, Def* def, int param_count, u4 argc, int argdiff, bool nativecall);
    
    AddressStack   addressStack;    //!< Stack of addresses used by the ensure statement.
    ExceptionStack handlers;        //!< Stack of ExceptionBlocks used for exception handling.    
    ScopeStack     scopes;          //!< Stack of scopes
    ScopeIter      scopesTop;       //!< Top of the scopes stack, the last used scope.
    ScopeIter      scopesEnd;       //!< Last position in the scopes stack.
    ScopeIter      scopesBeg;       //!< First position in the scopes stack.
    EState         state;           //!< Current state.
    Context*       prev;            //!< Previous or calling Context.
    Value*         stack;           //!< Operand stack.
    code_t*        pc;              //!< Program counter, maybe null.
    Value*         sp;              //!< Top of the stack, the first open stack position.
    Value*         bsp;             //!< Base stack pointer. Points to the current scopes arguments and lexEnv.
    Value*         esp;             //!< Extreme stack pointer. Last allocated stack position.
    Value          self;            //!< Specified self object for the current scope.
    Function*      closure;         //!< Specified function for the current scope.
    Package*       package;         //!< Specified package for the current scope.
    LexicalEnv*    env;             //!< Lexical environment for the current scope.
    LiteralPool*   currLiterals;    //!< LiteralPool used in the current scope.
    bool           newCall;         //!< Is the current scope a result of the operator new.
    u4             argCount;        //!< Actual number of arguments stored on the stack.
    u4             retCount;        //!< Number of return values expected.
    u4             numTailCalls;    //!< Number of tail-calls since the last return.
    s4             numRuns;         //!< Number of non-inlined calls to Run() made.
    int            nativeCallDepth; //!< Number of native calls currently in the scopes stack.
    u4             callsCount;      //!< The number of inlined calls for a suspended context.
    Value          acc;             //!< The Value of the last expression executed. Used for implicit returns (ie a return with no specified expression).
protected:
    
    INLINE bool IsWithFrame()    { return ((scopesTop >  scopesBeg) && (*(scopesTop - 1)).kind == SCOPE_with);    }
    INLINE bool IsPackageFrame() { return ((scopesTop >  scopesBeg) && (*(scopesTop - 1)).kind == SCOPE_package); }
    INLINE bool IsCallFrame()    { return ((scopesTop <= scopesBeg) || (*(scopesTop - 1)).kind == SCOPE_call);    }
    
    void ArgumentTagError(u2 arg, ValueTag given, ValueTag expected);    
    
    void PushCallScope();       //!< Pushes a new call scope onto the scopes stack. Storing the current call.
    void PopCallScope();        //!< Pops a call scope off the scopes stack. Restoring the previous call.
    void PushPackageScope();    //!< Pushes a new package scope onto the scopes stack. Storing the current package.
    void PopPackageScope();     //!< Pops a package scope off the scopes stack. Restoring the previous package.
    void PushWithScope();       //!< Pushes a new with scope onto the scopes stack. Storing the current self object.
    void PopWithScope();        //!< Pops a with scope off the scopes stack. Restoring the previous self object.
    void PopScope();            //!< Pops a scope, dependent on the ScopeKind, off the top of the scopes stack.
    
    void GrowScopeStack();      //!< Increase the size of the scopes stack by a constant metric.
    
    /** Increase operand stack size.
      *
      * @param  amt Minimum amount of Values to grow by (NOT bytes).
      *
      * @warning The location of the Stack <i>may</i> change, invalidating any previous pointers or 
      * references into the stack. Store offsets not pointers to prevent accessing 
      * invalidated memory locations.
      */
    void GrowStack(size_t amt);
    
    /** Decode an return the byte operand of the given instruction. */
    PIKA_FORCE_INLINE u1 GetByteOperand(const code_t instr)  { return PIKA_GET_BYTEOF(instr);  }
    
    /** Decode an return the short operand of the given instruction. */
    PIKA_FORCE_INLINE u2 GetShortOperand(const code_t instr) { return PIKA_GET_SHORTOF(instr); }
    
    /** Create a new function closure at the current point in the script.
      * @param  def     The function definition for the closure.
      * @param  ret     The Value that will contain the closure.
      * @param  ptrself The optional pointer to the self object. Used when the function is declared with a dot-name ie <code>function foo.bar() ... end </code>.
      */
    void DoClosure(Def* def, Value& ret, Value* ptrself= 0);
    
    /* These methods each execute a certain group of opcodes. They are meant to be called from Context::Run (only!)
     * because override methods that are called will be inlined.
     */
    void    ArithOpBinary(const Opcode op, const OpOverride ovr, const OpOverride ovr_r, int& numcalls);
    void    BitOpBinary  (const Opcode op, const OpOverride ovr, const OpOverride ovr_r, int& numcalls);
    void    CompOpBinary (const Opcode op, const OpOverride ovr, const OpOverride ovr_r, int& numcalls);
    void    ArithOpUnary (const Opcode op, const OpOverride ovr, int& numcalls);    
    void    OpSuper();
    bool    DoPropertyGet(int& numcalls, Property* prop);
    bool    DoPropertySet(int& numcalls, Property* prop);
    void    OpDotGet(int& numcalls, Opcode oc);
    void    OpDotSet(int& numcalls, Opcode oc);
    bool    OpUnpack(u2);
    bool    OpBind();
    bool    OpCat(bool sp);
    bool    OpIs();
    bool    OpHas();
    EErrorResult    OpException(Exception&);
        
    void    Activate();
    void    Deactivate();
public:
    bool    SetupOverride(u2 argc, Basic* obj, OpOverride ovr, bool* res = 0);  // Generic
    bool    SetupOverrideRhs(Basic* obj, OpOverride ovr, bool* res = 0);        // Object on the right hand side
    bool    SetupOverrideLhs(Basic* obj, OpOverride ovr, bool* res = 0);        // Object on the left hand side
    bool    SetupOverrideUnary(Basic* obj, OpOverride ovr, bool* res = 0);      // Unary prefix and postfix
    
    ScopeIter GetFirstScope() { /* returns a copy */ return scopesBeg; } 
    ScopeIter GetScopeTop()   { /* returns a copy */ return scopesTop; } 
    
    INLINE bool IsSuspended() const { return state == SUSPENDED; }
    INLINE bool IsRunning()   const { return state == RUNNING;   }
    INLINE bool IsDead()      const { return state == DEAD;      }
    INLINE bool IsInvalid()   const { return state == INVALID;   }
    INLINE bool IsPrepared()  const { return state == PREPARED;   }
    
    INLINE pint_t GetState() const { return state; }
    
    virtual void Init(Context*);
    
    void Setup(Context*);
    void Call(Context* ctx, u2 exretc);
    
    static void Suspend(Context*);
    
    /** Sets up a function call using this Context. The call arguments should be push in order onto
      * the stack. Followed by the self object (or null)
      * and the function to be called. Native functions are called immediately. Bytecode-functions
      * need to be executed by calling Context::Run.
      *
      * example:
      * <pre>
      * context->CheckStackSpace(N + 2);
      * context->Push(arg1);
      * ...
      * context->Push(argN);
      * context->Push(self);
      * context->Push(method);
      * if (context->SetupCall(N, false, 1))
      *      context->Run();
      *  Value returned = context->PopTop();
      *  </pre>
      *
      * @param argc      The number of arguments pushed on the stack
      * @param tailcall  Is the call a tailcall. Should not be used under normal circumstances.
      * @param retc      The number of expected return values. Which will be returned on the stack.
      *
      * @result true if Run needs to be called because the method is a bytecode method.
      */
    bool SetupCall(u2 argc, bool tailcall = false, u2 retc = 1);
    
    /** Suspend with a specified Value. */
    void DoSuspend(Value* v, size_t amt);
    
    /** Resume a suspended Context. */
    void DoResume();
    
    static Context* Create(Engine*, Type*);
    
    /** Checks that the specified number of values can be pushed onto the stack. If there is not
      * enough room the stack will grow.
      *
      * @param amt      [in] The number of values to check for (not bytes.)
      */
    void CheckStackSpace(const u4 amt);
    
    /** Allocate the specified number of values by moving the top of the stack.
      * Raises an Exception if sp + amt exceeds the max stack size available (PIKA_MAX_OPERAND_STACK).
      */
    INLINE void StackAlloc(u2 amt)
    {
        CheckStackSpace(amt);
        sp += amt;
    }
    
    INLINE void Push(const Value& v) { *sp++ = v; }
    INLINE void Push(pint_t i)       { (sp++)->Set(i); }       //!< Push the integer onto the stack.
    INLINE void Push(preal_t r)      { (sp++)->Set(r); }       //!< Push the real onto the stack.
    INLINE void Push(Enumerator* e)  { (sp++)->Set(e); }       //!< Push the Enumerator onto the stack.
    INLINE void Push(UserData* obj)  { (sp++)->Set(obj); }     //!< Push the UserData object onto the stack.
    INLINE void Push(String* str)    { (sp++)->Set(str); }     //!< Push the String onto the stack.
    INLINE void Push(Object* obj)    { (sp++)->Set(obj); }     //!< Push the Object onto the stack.
    INLINE void PushNull()           { (sp++)->SetNull(); }    //!< Push <code>null</code> onto the stack.
    INLINE void PushBool(bool b)     { (sp++)->SetBool(b); }   //!< Push the boolean value onto the stack.
    INLINE void PushTrue()           { (sp++)->SetTrue(); }    //!< Push the boolean value <i>true</i> onto the stack.
    INLINE void PushFalse()          { (sp++)->SetFalse(); }   //!< Push the boolean value <i>false</i> onto the stack.
    
    /** Safely Push a Value onto the stack. Supports all Push overloads.
      * @see Context::Push
      */
    template<typename T>
    INLINE void SafePush(T t)
    {
        this->CheckStackSpace(1);
        this->Push(t);
    }
    
    INLINE void SafePushNull()           { this->CheckStackSpace(1); this->PushNull(); }    //!< Safely Push a <i>null</i> Value onto the stack.
    INLINE void SafePushBool(bool b)     { this->CheckStackSpace(1); this->PushBool(b); }   //!< Safely Push a boolean Value onto the stack.
    INLINE void SafePushTrue()           { this->CheckStackSpace(1); this->PushTrue(); }    //!< Safely Push the boolean Value <i>true</i> onto the stack.
    INLINE void SafePushFalse()          { this->CheckStackSpace(1); this->PushFalse(); }   //!< Safely Push the boolean Value <i>false</i> onto the stack.
    
    INLINE void Pop()       { --sp; }       //!< Pop the top Value off the stack.
    INLINE void Pop(u2 amt) { sp -= amt; }  //!< Pop multiple Values off the top of the stack.
    
    INLINE Value& Top()      { return *(sp - 1); }      //!< Returns the top Value of the stack.
    INLINE Value& Top1()     { return *(sp - 2); }      //!< Returns the Value one position below the top of the stack.
    INLINE Value& Top2()     { return *(sp - 3); }      //!< Returns the Value two positions below the top of the stack.
    INLINE Value& TopN(u4 n) { return *(sp - 1 - n); }  //!< Returns the Value an arbitrary number of positions below the top of the stack.
    
    INLINE Value& PopTop()   { return *(--sp); }    //!< Pop and return the top Value of the stack.
    
    /** Returns a local variable of the current scope.
      * @param idx  [in] Index of the local variable.
      * @result     Reference to the local variable.
      */
    INLINE Value& GetLocal(u4 idx) { return *(bsp + idx); }
    
    /** Returns an outer variable of the current scope.
      * @param idx      [in] Index of the variable.
      * @param depth    [in] Depth of the variable.    
      * @result         Reference to the outer variable.
      */
    Value& GetOuter(u2 idx, u1 depth);
    
    /** Set a local variable of the current scope.
      *
      * @param val  [in] The Value to set the local variable to.
      * @param idx  [in] Index of the local variable.
      *
      * @note No check is made to ensure that bsp[idx] is the location of
      *       a local variable.
      */
    INLINE void SetLocal(const Value& val, u4 idx) { *(bsp + idx) = val; }
    
    /** Set an outer variable of the current scope.
      *
      * @param outer    [in] Value to set the variable to.
      * @param idx      [in] Index of the variable.
      * @param depth    [in] Depth of the variable.
      */
    void SetOuter(const Value& outer, u2 idx, u1 depth);
    
    /** Return a pointer to just beyond the top element of the stack. */
    
    INLINE Value* GetStackPtr() { return sp; }
        
    INLINE size_t GetStackSize() const { return sp - stack; }
    
    /** Run the bytecode interpreter. Should be if SetupCall returns true. */
    void Run();
    
    /** Returns the self object of the current scope. */
    INLINE Value& GetSelf() { return self; }
        
    /** Set the self object for the current scope. */
    INLINE void SetSelf(Value& s) { /* No writebarrier needed since we are a Context */ self = s; }
    
    /** Returns name of the current function. */
    String* GetFunctionName();
    
    /** Returns name of the current scope's package.
      * @param fullyQualified   [in] Determines if the name contains the full path to the package (ie: World.Foo.A instead of A).
      */
    String* GetPackageName(bool fullyQualified);
    
    INLINE Package*  GetPackage()  const { return package;  }
    INLINE Function* GetFunction() const { return closure;  }
    INLINE u2        GetArgCount() const { return argCount; }
    INLINE u2        GetRetCount() const { return retCount; }
    
    /** Returns a reference to the requested argument. */
    INLINE Value& GetArg(u2 idx)
    {
        ASSERT(idx < argCount); // TODO: It may be better to throw an exception.
        return GetLocal(idx); 
    }
    
    /** Returns a pointer to the current function's arguments. */
    INLINE Value* GetArgs() { return bsp; }
        
    /** Raise an exception if the number of arguments does not match. */
    void CheckParamCount(u2 amt);
    
    /** Raise a generic argument-count exception. */
    void WrongArgCount();
    
    /** Returns a pointer of the obj_type given from the specified argument. */
    template<typename T> T* GetArgT(u2 arg);
    
    
    // Retrieve the argument.
    //
    // !!! 
    // No check is made to ensure that the index given is within the range [0 - argc).
    // !!!
    //
    // A (C++) exception is raised if the argument is not the right type
    // or it cannot be implicity converted. Real to Integer is OK but String to Integer is not.
    // Use Engine::ToTTTT to convert a value to type TTTT
    // example:
    // Value arg0 = context->GetArg(0);
    // engine->ToIntegerExplicit(context, arg0);
    //
    pint_t      GetIntArg(u2 arg);      //!< Returns the integer value of the specified argument.
    preal_t     GetRealArg(u2 arg);     //!< Returns the real value of the specified argument.
    bool        GetBoolArg(u2 arg);     //!< Returns the boolean value of the specified argument.
    String*     GetStringArg(u2 arg);   //!< Returns the string value of the specified argument.
    Object*     GetObjectArg(u2 arg);   //!< Returns the object value of the specified argument.
    Enumerator* GetEnumArg(u2 arg);     //!< Returns the enumerator value of the specified argument.
    void*       GetUserDataArg(u2 arg, UserDataInfo* info); //!< Returns the userdata value of the specified argument.
    
    /** Returns whether the argument is null.
      * @note Unlike the Get*Arg functions IsArgNull will not raise an exception
      */
    INLINE bool IsArgNull(u2 arg)
    {
        Value& v = GetArg(arg);
        return v.IsNull();
    }
    
    // Convert the argument to a particular type //////////////////////////////////////////////////
    
    pint_t  ArgToInt(u2 arg);    //!< Convert an argument to an integer and return the result.
    preal_t ArgToReal(u2 arg);   //!< Convert an argument to a real and return the result.
    bool    ArgToBool(u2 arg);   //!< Convert an argument to a boolean and return the result.
    String* ArgToString(u2 arg); //!< Convert an argument to a string and return the result.    
    /*
    
    B : convert to boolean
    b : boolean
    
    E : enumerator
    e : enumerator
    
    I : convert to integer
    i : integer
    
    O : object
    o : object
    
    R : convert to real
    r : real
    
    S : convert to string
    s : string
    
    X : skip
    x : skip
    
    For ParseArgsInPlace a negative char value (i.e. -'o') will allow null values.
    
    */    
    void ParseArgs(const char* args, u2 count, ...);
    void ParseArgsInPlace(const char* args, u2 count);
};

INLINE void Context::CheckStackSpace(const u4 amt)
{
    const Value* p = sp + amt;
    if (p >= esp)
        GrowStack(p - esp);
}

template<typename T>
INLINE T* Context::GetArgT(u2 arg)
{
    Value& v = GetArg(arg);
    
    if (!v.IsDerivedFrom(T::StaticGetClass()))
    {
        RaiseException("excepted %s for argument %d.\n", T::StaticGetClass()->GetName() , arg);
    }
    return (T*)v.val.index;
}

bool GetOverrideFrom(Engine* eng, Basic* obj, OpOverride ovr, Value& res);

}//pika

#endif
