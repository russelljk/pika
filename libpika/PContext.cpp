/*
 *  PContext.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PDef.h"
#include "PContext.h"
#include "PFunction.h"
#include "PLocalsObject.h"
#include "PPackage.h"
#include "PString.h"
#include "PUserData.h"
#include "PEngine.h"
#include "PDictionary.h"
#include "PNativeBind.h"
#include "PContext_Ops.inl"

#include "PGenerator.h"

namespace pika {

namespace {

const char* GetContextStateName(Context::EState state)
{
    switch (state)
    {
    case Context::SUSPENDED: return "suspended";
    case Context::RUNNING:   return "running";
    case Context::DEAD:      return "dead";
    case Context::UNUSED:    return "unused";
    default:                 return "uninitialized"; // context's state member is uninitialized garbage.
    }
}

int Context_next(Context* ctx, Value& self)
{
    Context* callee = (Context*)self.val.object;
    /*
     * call the context with the expected number of args
     */
    callee->Call(ctx, ctx->GetRetCount());
    /*
     * Calculate the actual number of values pushed onto our stack.
     * (If the context returned it will match the calling ctx's retCount otherwise it is
     * the number of values yielded.)
     */
    const Value* top = ctx->GetStackPtr() - 1;
    const Value* btm = ctx->GetArgs() + ctx->GetArgCount();
    ptrdiff_t retc = top > btm ? top - btm : 0;
    return retc;
}

PIKA_DOC(Context_setup, "/(:va)\
\n\
Setup this Context so that it can be [next called]. The arguments given will be passed to the [Function function] it was initialized with. This should be a newly created Context.")

/* TODO { Context.setup should be changed to accept keyword Dictionary. } */
int Context_setup(Context* ctx, Value& self)
{
    Context* callee = (Context*)self.val.object;
    
    callee->Setup(ctx);
    if (callee->IsDead())
    {
        const Value*    top  = callee->GetStackPtr();
        const Value*    btm  = callee->GetArgs();
        const ptrdiff_t retc = top > btm ? top - btm : 0;
        
        ctx->CheckStackSpace(retc);
        for (const Value* v = btm; v < top; ++v)
            ctx->Push(*v);
        callee->Pop(retc);
        return retc;
    }
    return 0;
}

}// namespace

void Context::Constructor(Engine* eng, Type* obj_type, Value& res)
{
    Context* new_ctx = Context::Create(eng, obj_type);
    res.Set(new_ctx);
}

void Context::StaticInitType(Engine* eng)
{
    String* Context_String = eng->AllocString("Context");
    eng->Context_Type = Type::Create(eng, Context_String, eng->Object_Type, Context::Constructor, eng->GetWorld());
    
    static RegisterFunction Context_Methods[] =
    {
        { "next",  Context_next,  0, DEF_STRICT,   0 },
        { "setup", Context_setup, 0, DEF_VAR_ARGS, PIKA_GET_DOC(Context_setup) },
    };
    
    SlotBinder<Context>(eng, eng->Context_Type)
    .Constant((pint_t)Context::SUSPENDED,           "SUSPENDED")
    .Constant((pint_t)Context::RUNNING,             "RUNNING")
    .Constant((pint_t)Context::DEAD,                "DEAD")
    .Constant((pint_t)Context::UNUSED,              "UNUSED")
    .StaticMethodVA(&Context::Suspend,              "suspend")
    .Method(&Context::ToBoolean,                    "toBoolean")    
    .PropertyR("suspended?", &Context::IsSuspended, 0)
    .PropertyR("running?",   &Context::IsRunning,   0)
    .PropertyR("dead?",      &Context::IsDead,      0)
    .PropertyR("unused?",    &Context::IsUnused,    0)
    .PropertyR("state",      &Context::GetState,    "getState")
    ;
    
    eng->Context_Type->EnterMethods(Context_Methods, countof(Context_Methods));
    eng->GetWorld()->SetSlot(Context_String, eng->Context_Type);
}

// Enumerates each yielded value for the given Context.
class ContextIterator : public Iterator
{
public:
    ContextIterator(Engine* eng, Type* ctxtype, Context* ctx)
        : Iterator(eng, ctxtype), context(ctx)
    {}
    
    virtual ~ContextIterator()
    {}
        
    virtual bool ToBoolean()
    {
        return(context && (context->GetState() == Context::SUSPENDED)) || (context->GetState() == Context::RUNNING);
    }
    
    virtual int Next(Context* caller)
    {
        if (context->GetState() == Context::SUSPENDED)
        {
            u2 const retc = caller->GetRetCount();
            context->Call(caller, retc);
            return retc;
        }
        return 0;
    }    
private:
    Context* context;
};

void ScopeInfo::DoMark(Collector* c)
{
    if (closure) closure->Mark(c);
    if (package) package->Mark(c);
    if (env)     env->Mark(c);
    if (kwargs)  kwargs->Mark(c);
    if (generator) generator->Mark(c);
    MarkValue(c, self);
}

void ExceptionBlock::DoMark(Collector* c)
{
    if (package)
        package->Mark(c);
    MarkValue(c, self);
}

bool GetOverrideFromValue(Engine* eng, Value& v, OpOverride ovr, Value& res)
{
    Type* type = eng->GetTypeOf(v);
    String* what = eng->GetOverrideString(ovr);
    Value vwhat(what);
    return type->GetField(vwhat, res);
}

bool GetFieldFromValue(Engine* eng, Value& v, String* method, Value& res)
{
    Type* type = eng->GetTypeOf(v);
    Value vwhat(method);
    return type->GetField(vwhat, res);
}

/** What we are doing is reading from obj.type.override where override is the
  * appropriate override name. We intentially skip obj's members since its
  * unlikely to be the correct function, especially when obj is a type.
  */
bool GetOverrideFrom(Engine* eng, Basic* obj, OpOverride ovr, Value& res)
{
    String* what = eng->GetOverrideString(ovr);
    Value vwhat(what);
    Type* objType = obj->GetType();
    return objType->GetField(vwhat, res);
}

Iterator* GetIteratorFrom(Context* ctx, Value& val, String* kind)
{
    if (!(val.tag >= TAG_basic))
        return 0;
    
    Engine* eng = ctx->GetEngine();
    Value result(NULL_VALUE);
    
    if (GetOverrideFrom(eng, val.val.basic, OVR_iterate, result))
    {            
        ctx->Push(kind ? kind : eng->emptyString);
        ctx->Push(val);
        ctx->Push(result);
        if (ctx->SetupCall(1))
        {
            ctx->Run();
        }

        result = ctx->PopTop();
        if (eng->Iterator_Type->IsInstance(result))
        {
            return (Iterator*)result.val.object;
        }
    }
    return 0;
}

PIKA_IMPL(Context)

void Context::PushCallScope()
{
    ScopeInfo& currA = *scopesTop;
    
    currA.kwargs = kwargs;
    currA.env = env;
    currA.pc = pc;
    currA.self = self;
    currA.stackTop = sp - stack;
    currA.stackBase = bsp - stack;
    currA.closure = closure;
    currA.generator = generator;
    currA.argCount = argCount;
    currA.retCount = retCount;
    currA.numTailCalls = numTailCalls;
    currA.package = package;
    currA.kind = SCOPE_call;
    if (++scopesTop >= scopesEnd)
        GrowScopeStack();
}

void Context::PushWithScope()
{
    ScopeInfo& currA = *scopesTop;
    
    // We only care about the self and kind fields but we want to null
    // anything the gc may want to mark.
    currA.env       = 0;
    currA.closure   = 0;
    currA.generator = 0;
    currA.package   = 0;
    currA.self = self;       // all we care about.
    currA.kind = SCOPE_with;
    
    if (++scopesTop >= scopesEnd)
        GrowScopeStack();
}

void Context::PopCallScope()
{
    if (scopesTop == scopesBeg)
        ReportRuntimeError(Exception::ERROR_system, "Scope stack underflow.");
        
    --scopesTop;
    ScopeInfo& currA = *scopesTop;
    
    ASSERT(currA.kind == SCOPE_call);
    kwargs = currA.kwargs;
    env = currA.env;
    pc = currA.pc;
    sp = stack + currA.stackTop;
    bsp = stack + currA.stackBase;
    self = currA.self;
    closure = currA.closure;
    generator = currA.generator;
    argCount = currA.argCount;
    retCount = currA.retCount;
    numTailCalls = currA.numTailCalls;
    package = currA.package;
}

void Context::PopWithScope()
{
    if (scopesTop == scopesBeg)
        ReportRuntimeError(Exception::ERROR_system, "Scope stack underflow.");
        
    --scopesTop;
    const ScopeInfo& currA = *scopesTop;
    
    // popping anything else off will mess with the current interpreter state.
    self = currA.self;
}

void Context::PopScope()
{
    if (scopesTop == scopesBeg)
        ReportRuntimeError(Exception::ERROR_system, "Scope stack underflow.");
        
    ScopeInfo& currA = *(scopesTop - 1);
    switch (currA.kind)
    {
    case SCOPE_call:    PopCallScope();    break;
    case SCOPE_with:    PopWithScope();    break;
    case SCOPE_package: PopPackageScope(); break;
    }
}

void Context::PushPackageScope()
{
    ScopeInfo& currA = *scopesTop;
    
    currA.env       = 0;
    currA.closure   = 0;
    currA.generator = 0;    
    currA.package = package; // all we care about.
    currA.kind    = SCOPE_package;
    currA.self.SetNull();
    
    if (++scopesTop >= scopesEnd)
        GrowScopeStack();
}

void Context::PopPackageScope()
{
    if (scopesTop == scopesBeg)
        ReportRuntimeError(Exception::ERROR_system, "Scope stack underflow.");
        
    --scopesTop;
    const ScopeInfo& currA = *scopesTop;
    package = currA.package; // popping anything else off will mess with the current interpreter state.
}

Iterator* Context::Iterate(String* iterkind)
{
    if (iterkind != engine->emptyString)
    {
        return ThisSuper::Iterate(iterkind);
    }
    ContextIterator* c;
    PIKA_NEW(ContextIterator, c, (engine, engine->Iterator_Type, this));
    engine->AddToGC(c);
    return c;
}

Context::Context(Engine* eng, Type* obj_type)
    : ThisSuper(eng, obj_type),
      state(UNUSED),
      prev(0),
      stack(0),
      pc(0),
      sp(0),
      bsp(0),
      esp(0),
      closure(0),
      generator(0),
      package(0),
      env(0),
      kwargs(0),
      argCount(0),
      retCount(1),
      numTailCalls(0),
      numRuns(0),
      nativeCallDepth(0),
      acc(NULL_VALUE)
{
    scopes.Resize(PIKA_INIT_SCOPE_STACK);
    scopesTop = scopes.Begin();
    scopesBeg = scopes.Begin();
    scopesEnd = scopes.End();
    callsCount = 1;
    self.SetNull();
    
    sp = bsp = stack = (Value*)Pika_malloc(sizeof(Value) * PIKA_INIT_OPERAND_STACK);
    esp = stack + PIKA_INIT_OPERAND_STACK;
}

void Context::Reset()
{
    ASSERT(esp);
    sp = bsp = stack;
    
    // Reset scope stack.
    scopes.Resize(PIKA_INIT_SCOPE_STACK);
    scopesTop = scopes.Begin();
    scopesBeg = scopes.Begin();
    scopesEnd = scopes.End();
    
    addressStack.Resize(0);
    handlers.Resize(0);
    
    acc = NULL_VALUE;
    callsCount      = 1;
    closure         = 0;
    package         = 0;
    env             = 0;
    kwargs          = 0;
    state           = UNUSED;
    prev            = 0;
    pc              = 0;
    argCount        = 0;
    retCount        = 1;
    numTailCalls    = 0;
    numRuns         = 0;
    nativeCallDepth = 0;
    self.SetNull();
    
    // TODO { Should we clean out this Context's members? }
    //if (members) 
    //    members->Clear(); 
}

Context::~Context() { Pika_free(stack); }

void Context::GrowStack(size_t min_amt)
{
    size_t current_amt   = esp - stack;
    size_t requested_amt = min_amt + current_amt;
    size_t doubled_amt   = (size_t)(current_amt * PIKA_STACK_GROWTH_RATE);
    size_t new_amt       = Max(requested_amt, doubled_amt);
    new_amt = Clamp<size_t>(new_amt, current_amt, PIKA_MAX_OPERAND_STACK);
    
    if (new_amt < requested_amt)
        ReportRuntimeError(Exception::ERROR_system, "Operand stack overflow");
        
    size_t savedsp  = sp  - stack;
    size_t savedbsp = bsp - stack;
    
    stack = (Value*)Pika_realloc(stack, new_amt * sizeof(Value));
    
    // All previous activation's need their lexEnv set to the new stack pointer,
    // otherwise they might point to a invalid block of memory.
    
    for (ScopeIter currap = scopesBeg; currap < scopesTop; ++currap)
    {
        Function* fun = (*currap).closure;
        
        if (!fun)
            continue;
            
        Def* def = fun->GetDef();
        
        if (def->nativecode)
            continue;
            
        if ((*currap).env)
            (*currap).env->Set(stack + (*currap).stackBase, def->numLocals);
    }
    sp  = stack + savedsp;
    bsp = stack + savedbsp;
    esp = stack + new_amt;
    
    // current activation needs its lexEnv set to the new stack pointer
    
    if (env)
        env->Set(bsp, env->Length());
}

void Context::DoSuspend(Value* v, size_t amt)
{
    if (IsRunning()) state = SUSPENDED;
    
    if (prev)
    {
        // If someone is expecting the yield values ...
        if (prev->closure)
        {
            // Copy all the values produced from our stack to theirs.
            prev->CheckStackSpace(amt);
            Value* vend = v + amt;
            for (Value* curr = v; curr != vend; curr++)
            {
                prev->Push(*curr);
            }
        }
        // Resume our calling Context.
        Context* next = prev;
        prev = 0;
        return next->DoResume();
    }
    else
    {
        // Nobody called so we can deactivate this context.
        // note: we can still be resumed if this context did not end with a return.
        Deactivate();
    }
}

void Context::DoResume()
{
    if (state == SUSPENDED)
    {
        // Start running this context again.
        return Run();
    }
    else if (state == RUNNING)
    {
        // Activate our self before we continue execution.
        return Activate();
    }
}

void Context::OpCompBinary(const Opcode op, const OpOverride ovr, const OpOverride ovr_r, int& numcalls)
{
    Value& b = Top();
    Value& a = Top1();
    bool res = false;
    int const btag = b.tag;
    
    switch (a.tag)
    {
    case TAG_null:
    case TAG_boolean:
        break;
    case TAG_integer:
    {
        if (btag == TAG_real)
        {
            // convert a to a Real
            a.Set((preal_t)a.val.integer);
            switch (op)
            {
            case OP_lt:  res =  le_num(a.val.real, b.val.real); break;
            case OP_gt:  res =  gr_num(a.val.real, b.val.real); break;
            case OP_lte: res = lte_num(a.val.real, b.val.real); break;
            case OP_gte: res = gte_num(a.val.real, b.val.real); break;
            default: break;
            }
            a.SetBool(res);
            Pop();
            return;
        }
        else if (btag != TAG_integer)
        {
            break;
        }
        
        switch (op)
        {
        case OP_lt:  res =  le_num(a.val.integer, b.val.integer); break;
        case OP_gt:  res =  gr_num(a.val.integer, b.val.integer); break;
        case OP_lte: res = lte_num(a.val.integer, b.val.integer); break;
        case OP_gte: res = gte_num(a.val.integer, b.val.integer); break;
        default: break;
        }
        
        a.SetBool(res);
        Pop();
        return;
    }
    case TAG_real:
    {
        if (btag == TAG_integer)
        {
            // convert b to a Real
            b.Set((preal_t)b.val.integer);
        }
        else if (btag != TAG_real)
        {
            break;
        }
        
        switch (op)
        {
        case OP_lt:  res =  le_num(a.val.real, b.val.real); break;
        case OP_gt:  res =  gr_num(a.val.real, b.val.real); break;
        case OP_lte: res = lte_num(a.val.real, b.val.real); break;
        case OP_gte: res = gte_num(a.val.real, b.val.real); break;
        default: break;
        }
        
        a.SetBool(res);
        Pop();
        return;
    }
    case TAG_string:
    {
        if (btag != TAG_string)
            break;
        switch (op)
        {
        case OP_lt:  res =  le_num(*a.val.str, *b.val.str); break;
        case OP_gt:  res =  gr_num(*a.val.str, *b.val.str); break;
        case OP_lte: res = lte_num(*a.val.str, *b.val.str); break;
        case OP_gte: res = gte_num(*a.val.str, *b.val.str); break;
        default: break;
        }
        
        a.SetBool(res);
        Pop();
        return;
    }
    case TAG_userdata:
    case TAG_object:
    {
        Basic* obj = a.val.basic;
        bool called = false;
        
        if (SetupOverrideLhs(obj, ovr, &called))
            ++numcalls;
        if (called)
            return;
    }
    break;
    }
    
    if (btag == TAG_object || btag == TAG_userdata)
    {
        Basic* obj = b.val.basic;
        bool called = false;
        
        if (SetupOverrideRhs(obj, ovr_r, &called))
            ++numcalls;
        if (called)
            return;
    }
    
    ReportRuntimeError(Exception::ERROR_type,
                       "Operator %s not defined between types %s %s.",
                       engine->GetOverrideString(ovr)->GetBuffer(),
                       engine->GetTypenameOf(a)->GetBuffer(),
                       engine->GetTypenameOf(b)->GetBuffer());
                       
}

INLINE void Context::OpArithUnary(const Opcode op, const OpOverride ovr, int& numcalls)
{
    Value& a = Top();
    if (a.tag == TAG_integer)
    {
        switch (op)
        {
        case OP_inc: inc_num(a.val.integer); break;
        case OP_dec: dec_num(a.val.integer); break;
        case OP_pos: pos_num(a.val.integer); break;
        case OP_neg: neg_num(a.val.integer); break;
        default: break;
        }
    }
    else if (a.tag == TAG_real)
    {
        switch (op)
        {
        case OP_inc: inc_num(a.val.real); break;
        case OP_dec: dec_num(a.val.real); break;
        case OP_pos: pos_num(a.val.real); break;
        case OP_neg: neg_num(a.val.real); break;
        default: break;
        }
    }
    else if (a.tag >= TAG_basic)
    {
        Basic* bas = a.val.basic;
        if (SetupOverrideUnary(bas, ovr))
            ++numcalls;
    }
    else
    {
        ReportRuntimeError(Exception::ERROR_type,
                           "Operator %s not defined for object of type %s.",
                           engine->GetOverrideString(ovr)->GetBuffer(),
                           engine->GetTypenameOf(a)->GetBuffer());
    }
}

INLINE void Context::OpBitBinary(const Opcode op, const OpOverride ovr, const OpOverride ovr_r, int& numcalls)
{
    Value& b = Top();
    Value& a = Top1();
    
    if (a.tag == TAG_integer && b.tag == TAG_integer)
    {
        switch (op)
        {
        case OP_bitand: band_num(a.val.integer, b.val.integer); break;
        case OP_bitor:  bor_num (a.val.integer, b.val.integer); break;
        case OP_bitxor: bxor_num(a.val.integer, b.val.integer); break;
        case OP_lsh:    lsh_num (a.val.integer, b.val.integer); break;
        case OP_rsh:    rsh_num (a.val.integer, b.val.integer); break;
        case OP_ursh:   ursh_num(a.val.integer, b.val.integer); break;
        default: break;
        }
        Pop();
        return;
    }
    
    // Check for override from the lhs operand
    if (a.tag >= TAG_basic)
    {
        Basic* basic = a.val.basic;
        bool res = false;
        
        if (SetupOverrideLhs(basic, ovr, &res))
            ++numcalls;
        if (res)
            return;
    }
    
    // Check for override from the rhs operand
    if (b.tag >= TAG_basic)
    {
        Basic* basic = b.val.basic;
        bool    res = false;
        
        if (SetupOverrideRhs(basic, ovr_r, &res))
            ++numcalls;
        if (res)
            return;
    }
    
    ReportRuntimeError(Exception::ERROR_runtime,
                       "Operator %s not defined for between objects of type %s and %s.",
                       engine->GetOverrideString(ovr)->GetBuffer(),
                       engine->GetTypenameOf(a)->GetBuffer(),
                       engine->GetTypenameOf(b)->GetBuffer());
    return;
}

INLINE void Context::OpArithBinary(const Opcode op, const OpOverride ovr, const OpOverride ovr_r, int& numcalls)
{
    Value& b = Top();
    Value& a = Top1();
    
    switch (a.tag)
    {
    case TAG_integer:
    {
        switch (b.tag)
        {
        case TAG_integer:
        {
            // integer op integer
            switch (op)
            {
            case OP_add: add_num(a.val.integer, b.val.integer); break;
            case OP_sub: sub_num(a.val.integer, b.val.integer); break;
            case OP_mul: mul_num(a.val.integer, b.val.integer); break;
            case OP_div:
            {
#       if defined(USE_INTEGER_DIVISION)
                div_num(a.val.integer, b.val.integer);
#       else
                pint_t const ia = a.val.integer;
                pint_t const ib = b.val.integer;
#       ifndef NO_DIVIDEBYZERO_ERROR
                if (ib == 0)
                {
                     RaiseException(Exception::ERROR_dividebyzero, "Divide by zero while attempting to find "PINT_FMT" divided by "PINT_FMT".", ia, ib);
                }
#       endif                
                if (ia % ib == 0)
                {
                    a.val.integer = ia / ib;
                }
                else
                {
                    preal_t const ra = (preal_t)a.val.integer;
                    preal_t const rb = (preal_t)b.val.integer;
                    a.Set((preal_t)(ra / rb));
                }
#       endif
                break;
            }
            case OP_idiv: div_num(a.val.integer, b.val.integer); break;
            case OP_mod:  mod_num(a.val.integer, b.val.integer); break;
            case OP_pow:  
                {
                    preal_t ai = a.val.integer;
                    preal_t bi = b.val.integer;
                    
                    
                    pow_num(ai, bi);
                    
                    if (Pika_RealIsInteger(ai))
                    {
                        a.Set((pint_t)ai);
                    }
                    else
                    {
                        a.Set((preal_t)ai);
                    }
                }
            break;
            default: break;
            }
            Pop();
            return;
        }
        case TAG_real:
        {
            // integer op real
            a.Set((preal_t)a.val.integer);
            switch (op)
            {
            case OP_add:  add_num(a.val.real, b.val.real); break;
            case OP_sub:  sub_num(a.val.real, b.val.real); break;
            case OP_mul:  mul_num(a.val.real, b.val.real); break;
            case OP_div:  div_num(a.val.real, b.val.real); break;
            case OP_idiv:
                div_num(a.val.real, b.val.real);
                a.Set(Pika_RealToInteger(a.val.real)); // TODO: convert integer to real
                break;
            case OP_mod:  mod_num(a.val.real, b.val.real); break;
            case OP_pow:  pow_num(a.val.real, b.val.real); break;
            default: break;
            }
            Pop();
            return;
        }
        case TAG_string:
        {
            if (op == OP_mul)
            {
                pint_t i = a.val.integer;
                String* str = b.val.str;
                String* res = String::Multiply(str, i);
                a.Set(res);
                Pop();
                return;
            }
        }
        break; // TAG_string
        }
    }
    break;
    case TAG_real:
    {
        switch (b.tag)
        {
        case TAG_integer:
        {
            // real op integer
            b.Set((preal_t)b.val.integer);
            switch (op)
            {
            case OP_add: add_num(a.val.real, b.val.real); break;
            case OP_sub: sub_num(a.val.real, b.val.real); break;
            case OP_mul: mul_num(a.val.real, b.val.real); break;
            case OP_div: div_num(a.val.real, b.val.real); break;
            case OP_idiv:
                div_num(a.val.real, b.val.real);
                a.Set(Pika_RealToInteger(a.val.real));
                break;
            case OP_mod: mod_num(a.val.real, b.val.real); break;
            case OP_pow: pow_num(a.val.real, b.val.real); break;
            default: break;
            }
            Pop();
            return;
        }
        case TAG_real:
        {
            // real op real
            switch (op)
            {
            case OP_add: add_num(a.val.real, b.val.real); break;
            case OP_sub: sub_num(a.val.real, b.val.real); break;
            case OP_mul: mul_num(a.val.real, b.val.real); break;
            case OP_div: div_num(a.val.real, b.val.real); break;
            case OP_idiv:
                div_num(a.val.real, b.val.real);
                a.Set(Pika_RealToInteger(a.val.real));
                break;
            case OP_mod: mod_num(a.val.real, b.val.real); break;
            case OP_pow: pow_num(a.val.real, b.val.real); break;
            default: break;
            }
            Pop();
            return;
        }
        }
    }
    break; // TAG_real
    }
    // Check for override from the lhs operand
    if (a.tag >= TAG_basic)
    {
        Basic* basic = a.val.basic;
        bool res = false;
        
        if (SetupOverrideLhs(basic, ovr, &res)) {
            ++numcalls;
        }
        if (res)
            return;
    }
    
    if (b.tag >= TAG_basic)
    {
        Basic* basic = b.val.basic;
        bool res = false;
        if (SetupOverrideRhs(basic, ovr_r, &res)) {
            ++numcalls;
        }
        if (res) return;
    }
    
    // We do not reach this point unless the operation is undefined between the
    // lhs and rhs operands.
    
    ReportRuntimeError(Exception::ERROR_runtime,
                       "Operator %s not defined between types %s and %s.",
                       engine->GetOverrideString(ovr)->GetBuffer(),
                       engine->GetTypenameOf(a)->GetBuffer(),
                       engine->GetTypenameOf(b)->GetBuffer());
}

bool Context::OpApply(u1 argc, u1 retc, u1 kwargc, bool tailcall)
{
    /* [  arg 0  ]
       [  .....  ] < regular arguments, [0 to argc]
       [  arg N  ]
       [  key 0  ]
       [  val 0  ]
       [  .....  ] < keyword arg pairs, [0 to kwargc]
       [  key N  ]
       [  val N  ]
       [  [...]  ] < variable argument array
       [  {...}  ] < keyword argument dictionary
       [ selfobj ] < self object
       [ frame   ] < function to be called
                   < sp
    
    ---------------------------------------------------------------------------
    
    First variable arguments.
    
    Then for any missing arument from M to N attempt to read the dictionary
    if that fails at any point raise an execption.
    
    if argdiff >= 0
      if function.is_va
         
         array = [] if argdiff == 0 else args[function.param_count:argdiff]
         if vararg
           array.append( vararg )
         push array
       else
         raise exception TooManyArgs
        
        
    if function is vararg
      push the vararg array or []
    if the function is        
    */
    Value frame = PopTop();
    Value selfobj = PopTop();
    Value kw_arg = PopTop();
    Value var_arg = PopTop();    
    
    size_t amt = 0; // stack space we need before we start pushing
    
    // Check that null or and Array was pushed.
    Array* array = 0;
    if (!var_arg.IsNull())
    {
        if (engine->Array_Type->IsInstance(var_arg))
        {
            array = static_cast<Array*>(var_arg.val.object);            
            size_t array_size = array->GetLength();
            
            if (array_size > PIKA_MAX_ARGS)
            {
                RaiseException(Exception::ERROR_type, "Attempt to apply variable argument call with an Array with too many members.");
            }
            
            argc += (u1)array_size;
            amt += array_size;            
        } 
        else
        {
            RaiseException(Exception::ERROR_type, "Attempt to apply invalid variable argument to a function call.");
        }
    }
    
    size_t dict_size = 0; // number of elements in the dictionary.
    Dictionary* dict = 0;
    if (!kw_arg.IsNull())
    {
        if (engine->Dictionary_Type->IsInstance(kw_arg))
        {
            dict = static_cast<Dictionary*>(kw_arg.val.object);
            dict_size = dict->Elements().Count();
            
            if (dict_size > PIKA_MAX_KWARGS)
            {
                RaiseException(Exception::ERROR_type, "Attempt to apply keyword argument call with a Dictionary with too many members.");
            }
            
            amt += (dict_size * 2); // need to account for both keys and values
        }
        else
        {
            RaiseException(Exception::ERROR_type, "Attempt to apply invalid keyword argument to a function call.");
        }
    }
    
    CheckStackSpace(amt + closure->def->stackLimit);
    
    Value* old_sp = sp;
    StackAlloc(amt);
    
    Value* start = sp - amt;
    
    if (array)
    {
        if (kwargc)
        {
            // Need to copy the keyword arguments up toward the top of the stack,
            // but leaving enough room for the dictionary's elements.
            Value* copy_from = old_sp - (kwargc * 2);
            Value* copy_to = sp;
            start = copy_from;
            while (copy_from < old_sp)
            {
                *copy_to++ = *copy_from++;
            }
        }
        
        // Now copy the Array elements to the start of the space we allocated.
        Buffer<Value>::Iterator end_iter = array->GetElements().End();
        for (Buffer<Value>::Iterator iter = array->GetElements().Begin(); iter != end_iter; ++iter)
        {
            *start = *iter;
            start++;
        }
    }
    
    if (dict)
    {
        start += kwargc * 2; // skip past explicitly specified keyword arguments.
        for (Table::Iterator iter = dict->Elements().GetIterator(); iter; ++iter)
        {
            // Copy both key and value.
            *start++ = iter->key;
            *start++ = iter->val;
        }
    }
    
    // No reason this should differ.
    ASSERT(start == sp);
    
    Push(selfobj); // Self object
    Push(frame);   // Function object
    
    kwargc += (u1)dict_size; // Need update after we unload the dictionary. So don't move it!
    
    return SetupCall(argc, retc, kwargc, tailcall);
}

int Context::AdjustArgs(Function* fun, Def* def, int const param_count,
                        u4 const argc, int const argdiff, bool const nativecall)
{
    int resultArgc = argc;
    
    if (def->isStrict && nativecall)
    {
        if (argdiff > 0)
        {
            String* dotname = fun->GetLocation()->GetDotName();
            RaiseException(Exception::ERROR_runtime,
                           "Too many arguments for function '%s.%s'. Expected exactly %d %s but was given %d.",
                           dotname->GetBuffer(),
                           def->name->GetBuffer(),
                           param_count,
                           param_count == 1 ? "argument" : "arguments",
                           argc);
        }
        else
        {
            String* dotname = fun->GetLocation()->GetDotName();
            RaiseException(Exception::ERROR_runtime,
                           "Too few arguments for function '%s.%s'. Expected exactly %d %s but was given %d.",
                           dotname->GetBuffer(),
                           def->name->GetBuffer(),
                           param_count,
                           param_count == 1 ? "argument" : "arguments",
                           argc);
        }
    }
    else if (argdiff > 0)
    {
        // Too many arguments.
        // ------------------
        if (!nativecall)
        {
            if (def->isVarArg)
            {
                GCPAUSE_NORUN(engine);
                
                Array *v = Array::Create(engine, 0, argdiff, (GetStackPtr() - argdiff));
                Pop(argdiff);
                Push(v);            
                resultArgc = param_count;  
                ++resultArgc;
            }
            else
            {
                String* dotname = fun->GetLocation()->GetDotName();
                RaiseException(Exception::ERROR_runtime,
                           "Too many arguments for function '%s.%s'. Expected exactly %d %s but was given %d.",
                           dotname->GetBuffer(),
                           def->name->GetBuffer(),
                           param_count,
                           param_count == 1 ? "argument" : "arguments",
                           argc);
            }
        }
    }
    else
    {
        Object* ArgNotDefined = engine->ArgNotDefined;
        /* Too few arguments.
         * --------------------------------------------------------------------
         * We want to push null for all arguments unspecified and
         * without a default argument.
         *  
         * Then push the default values for all argument with default
         * values which were unspecified.
         *  
         * Lastly we want to push an empty vector if it is a bytecode
         * variadic function.
         */        
        int  argstart = argc;
        Defaults* defaults = fun->defaults;
                
        if (defaults)
        {
            size_t const numDefaults = defaults->Length();
            int const numRegularArgs = param_count - (int)numDefaults;
            
            int const amttopush = numRegularArgs - argstart;
            int const defstart  = Clamp<int>(-amttopush, 0, (int)numDefaults);
            
            // Arguments with no default value will be set to null.
            for (int p = 0; p < amttopush; ++p)
            {
                Push(ArgNotDefined);
            }
            
            // Arguments with a default value will be set to their default value.
            for (size_t a = defstart; a < numDefaults; ++a)
            {
                Value& defval = defaults->At(a);
                Push(defval);
            }
        }
        else
        {
            int const numRegularArgs = param_count;
            int const amttopush = numRegularArgs - argstart;
            for (int p = 0; p < amttopush; ++p)
            {
                Push(ArgNotDefined);
            }
        }
        
        resultArgc = param_count;
                
        // Create an empty variable arguments Array if needed.
        if (def->isVarArg && !nativecall)
        {
            GCPAUSE_NORUN(engine);            
            Array* v = Array::Create(engine, engine->Array_Type, 0, 0);
            Push(v);
            ++resultArgc;
        }
    }   
    return resultArgc;
}

bool Context::SetupCall(u2 argc, u2 retc, u2 kwargc, bool tailcall)
{
    /*  
        [ ...   ]
        [ arg 0 ]
        [ ...   ]
        [ arg N ] 
        [ kw  0 ]
        [ ...   ]
        [ kw  N ]
        [ self  ]
        [ func  ] < sp
    */
    Value frameVar = PopTop();
    Value selfVar  = PopTop();
    /*  
        [ ...   ]
        [ arg 0 ]
        [ ...   ]
        [ arg N ] 
        [ kw  0 ]
        [ ...   ]
        [ kw  N ]  < sp
    */    
    // frameVar is a derived from type Function.
    if (engine->Function_Type->IsInstance(frameVar))
    {
        u2 const provided_argc = argc;
        Function* fun = frameVar.val.function;
        Def* def = fun->GetDef();
        
        if (kwargc)
        {
            /* Copy keyword argument pairs to a temporary buffer so
             * that we can manipulate the arguments at the top of
             * the stack.
             */
            size_t const kw_total_values = kwargc*2;
            keywords.Resize(kw_total_values);
            Pika_memcpy(keywords.GetAt(0), GetStackPtr()-kw_total_values, kw_total_values*sizeof(Value));
            Pop(kw_total_values);
        }        
        /*  
            [ ...   ]
            [ arg 0 ]
            [ ...   ]
            [ arg N ] < sp
        */
        // Adjust argc to match the definition's param_count.
        int  const param_count = def->numArgs;
        int  const argdiff     = (int)argc - param_count;
        bool const nativecall  = (def->nativecode != 0);
        
        if (argdiff != 0)
        {
            // Incorrect argument count.
            argc = AdjustArgs(fun, def, param_count, argc, argdiff, nativecall);
        }        
        else if (def->isVarArg && !nativecall)
        {
            // We have the correct amount of arguments, however, we need to add
            // a empty varag function.
            GCPAUSE_NORUN(engine);            
            Array* v = Array::Create(engine, engine->Array_Type, 0, 0);
            Push(v);
            ++argc;
        }
        
        Dictionary* dict = 0;
        if (kwargc)
        {            
            if (def->isKeyword)
            {
                GCPAUSE_NORUN(engine);
                dict = Dictionary::Create(engine, engine->Dictionary_Type);
                if (!nativecall)
                {
                    Push(dict);
                    ++argc;
                }
            }
            /* Time to deal with the keyword arguments. */             
            Value* args_start = sp - argc;
            Table& kwords = def->kwargs;
            
            for (Buffer<Value>::Iterator curr = keywords.Begin(); curr != keywords.End(); curr+=2)
            {
                Value idx(NULL_VALUE);
                if (kwords.Get(*curr, idx))
                {
                    // TODO: Check that argument is not double specified BUT
                    //       We need a way to make sure it wasn't set as a default value.                    
                    ASSERT(idx.tag == TAG_integer);
                    pint_t the_index = idx.val.integer;
                    ASSERT(the_index < argc);
                    *(args_start + the_index) = *(curr + 1);
                }
                else if (dict)
                {
                    dict->BracketWrite(*curr, *(curr+1));
                }
            }
        }
        else if (def->isKeyword && !nativecall)
        {
            GCPAUSE_NORUN(engine);
            Dictionary* dict = Dictionary::Create(engine, engine->Dictionary_Type);
            Push(dict);
            ++argc;
        }
        
        // Check that there aren't any gaps in the arguments provided.
        for (Value* a = (sp-argc+provided_argc); a < sp; ++a)
        {
            const size_t index = (a - (sp-argc));
            
            if (a->IsObject() && *a == engine->ArgNotDefined)
            {
                String* dotname = fun->GetLocation()->GetDotName();
                if (!nativecall)
                {
                    size_t args_info_start = (PIKA_MAX_ARGS + 1);
                    size_t num_locals = def->localsInfo.GetSize();
                    
                    for (size_t li = 0; li < num_locals; ++li)
                    {
                        if (def->localsInfo[li].type == LVT_parameter)
                        {
                            args_info_start = li;
                            break;
                        }
                    }
                    
                    if (args_info_start < def->localsInfo.GetSize())
                    {
                        LocalVarInfo& linfo = def->localsInfo[args_info_start + index];
                        String* name = linfo.name;
                        RaiseException(Exception::ERROR_runtime,
                            "Argument '%s' not specified when calling function '%s.%s'",
                            name->GetBuffer(),
                            dotname->GetBuffer(),
                            def->name->GetBuffer()
                        );
                    }
                    else
                    {
                        RaiseException(Exception::ERROR_runtime,
                            "Positional argument '%d' not specified when calling function '%s.%s'",
                            index,
                            dotname->GetBuffer(),
                            def->name->GetBuffer()
                        );
                    }
                }
            }
        }
        
        Value* newsp;
        Value* argv;
        
        if (tailcall)
        {
            Value* oldargs = bsp;
            Value* newargs = sp - argc;
            Value* nalimit = sp;
            
            while (newargs < nalimit)
            {
                *oldargs++ = *newargs++;
            }
            
            newsp = sp = oldargs;
            
            // the new sp is one past the old one
            
            newsp += 1;
            
            // pop off all the arguments
            
            Pop(argc);
            argv = sp;
            numTailCalls++;
        }
        else
        {
            // the new sp is one past the old one
            newsp = sp + 1;
            
            // pop off all the arguments
            Pop(argc);
            argv = sp;
            
            PushCallScope();
            numTailCalls = 0;
            retCount = retc ? retc : 1;
        }
        
        // Set up the new scope.
        
        // Make room for local vars.
        sp = newsp + (def->numLocals - param_count);
        
        // Initialize all local (non-parameter) variables to null.
        for (Value* p = newsp; p < sp; ++p) { p->SetNull(); }
        generator = 0;
        kwargs    = dict;
        bsp       = argv;
        self      = selfVar;
        pc        = def->GetBytecode();
        closure   = fun;
        package   = fun->GetLocation();
        argCount  = argc;
        acc.SetNull();
        ASSERT(closure);
        ASSERT(package);
        /*
            [ ...          ]
            [ argument 0   ] < bsp
            [ ...          ]
            [ argument N   ]
            [ local 0      ]
            [ ...          ]
            [ local N      ]
            [ return value ] < rsp
            [              ] < sp
        */
        if (nativecall)
        {
            // Calling a native C function
            
            // No lexical environment for native calls.
            env = 0;
            
            // Make sure we are not mindlessly calling native functions.
            if (++nativeCallDepth > (s4)PIKA_MAX_NATIVE_RECURSION)
            {
                ReportRuntimeError(Exception::ERROR_runtime,
                                   "Native recursion limit of %d reached.",
                                   PIKA_MAX_NATIVE_RECURSION);
            }
            // Reserve enough space for an operator override or a (small) method call.
            CheckStackSpace(Max<ptrdiff_t>((ptrdiff_t)retCount - (ptrdiff_t)(sp - bsp), 0) + PIKA_NATIVE_STACK_EXTRA);
            
#ifndef PIKA_NO_HOOKS
            if (engine->HasHook(HE_nativeCall))
            {
                engine->CallHook(HE_nativeCall, (void*)this);
            }
#endif
            closure->BeginCall(this);
            
            int numret = def->nativecode(this, self);                        
                        
            OpReturn(numret);
            --nativeCallDepth;
            return false;
        }
        else
        {
            // Calling a bytecode function
            
            // Check that the stack has enough room for operator overload calls and the number of return values specified.
            CheckStackSpace(Max<ptrdiff_t>((ptrdiff_t)retCount - (ptrdiff_t)(sp - bsp), (ptrdiff_t)def->stackLimit) + PIKA_OPERAND_STACK_EXTRA);
            
            // If this function was tagged because its locals need to survive beyond its lifetime.
            
            closure->BeginCall(this);
            
            if (closure->MustClose())
            {
                //ASSERT(env == closure->lexEnv);
                //LexicalEnv* oldEnv = env;
                env = LexicalEnv::Create(engine, true); // ... create the lexical environment (ie the args + locals).
                env->Set(bsp, def->numLocals);          // ... right now they point to the stack.
            }
            else
            {
                env = 0; // Locals not used beyond the functions lifetime.
            }
#ifndef PIKA_NO_HOOKS
            if (engine->HasHook(HE_call))
            {
                engine->CallHook(HE_call, (void*)this);
            }
#endif
            if (def->isGenerator) {
                Generator* old_gen = generator;
                generator = Generator::Create(engine, engine->Generator_Type, closure);
                Push(generator);
                OpYield(1);
                generator = old_gen;
                return false;
            } else {
                return true;
            }
        }
    }
    else
    {
        // object or userdata that may contain a call override method.
        if (frameVar.tag >= TAG_basic)
        {
            if (frameVar.IsDerivedFrom(Generator::StaticGetClass()))
            {
                if (argc != 0 || kwargc != 0) {
                    ReportRuntimeError(Exception::ERROR_runtime,
                        "Attempt resume a generator by calling it with positional and/or keyword arguments.");
                }
                Generator* gen = static_cast<Generator*>(frameVar.val.object);
                gen->Resume(this, retc ? retc : 1, tailcall);
                return true;
            }
            else
            {
                Push(frameVar);
                return SetupOverride(argc, retc, kwargc, tailcall, frameVar.val.basic, OVR_call);
            }
        }
        else
        {
            ReportRuntimeError(Exception::ERROR_runtime,
                               "Attempt to invoke object of type %s.",
                               engine->GetTypenameOf(frameVar)->GetBuffer());
        }
    }
    return false;
}

void Context::CreateEnv()
{
    if (closure && closure->def && !env)
    {
        env = LexicalEnv::Create(engine, true);
        env->Set(bsp, closure->def->numLocals);         
    }
}

void Context::CreateEnvAt(ScopeIter iter)
{
    if (!(iter <= scopesTop && iter >= scopesBeg))
        return;
    
    if (iter->closure && iter->closure->def && !iter->env)
    {
        iter->env = LexicalEnv::Create(engine, true);
        iter->env->Set(stack + iter->stackBase, iter->closure->def->numLocals);
    }
}

bool Context::SetupOverride(u2 argc, u2 retc, u2 kwargc, bool tailcall, Basic* obj, OpOverride ovr, bool* res)
{
    ASSERT(obj);
    Value meth;
    meth.SetNull();
    if (GetOverrideFrom(engine, obj, ovr, meth))
    {
        Push(meth);
        if (res)
        {
            *res = true;
        }
        return SetupCall(argc, retc, kwargc, tailcall);
    }
    else if (!res)
    {
        String* strname = obj->GetType()->GetName();
        ReportRuntimeError(Exception::ERROR_runtime,
                           "Operator %s not defined for object of type %s.",
                           engine->GetOverrideString(ovr)->GetBuffer(),
                           strname->GetBuffer());
    }
    else
    {
        *res = false;
    }
    return false;
}

bool Context::SetupOverrideRhs(Basic* obj, OpOverride ovr, bool* res)
{
    ASSERT(obj);
    
    Value meth;
    meth.SetNull();
    
    if (GetOverrideFrom(engine, obj, ovr, meth))
    {
        //  [  lhs   ] argument 0
        //  [  obj   ] self
        Push(meth);
        //  [  lhs   ] argument 0
        //  [  obj   ] self
        //  [ meth   ] method
        if (res)
        {
            *res = true;
        }
        return SetupCall(1);
    }
    else if (!res)
    {
        String* strname = obj->GetType()->GetName();
        ReportRuntimeError(Exception::ERROR_runtime,
                           "Operator %s not defined for object of type %s.",
                           engine->GetOverrideString(ovr)->GetBuffer(),
                           strname->GetBuffer());
    }
    else
    {
        *res = false;
    }
    return false;
}

bool Context::SetupOverrideLhs(Basic* obj, OpOverride ovr, bool* res)
{
    ASSERT(obj);
    
    Value meth;
    meth.SetNull();
    
    if (GetOverrideFrom(engine, obj, ovr, meth))
    {
        //  [  obj   ] self object
        //  [  rhs   ]
        // swap lhs and rhs so that obj will be at the top of the stack.
        Swap(Top(), Top1());
        //  [  rhs   ] argument 0
        //  [  obj   ] self object
        Push(meth);
        //  [  rhs   ] argument 0
        //  [  obj   ] self object
        //  [ meth   ] method
        if (res)
        {
            *res = true;
        }
        return SetupCall(1);
    }
    else if (!res)
    {
        String* strname = obj->GetType()->GetName();
        ReportRuntimeError(Exception::ERROR_runtime,
                           "Operator %s not defined for object of type %s.",
                           engine->GetOverrideString(ovr)->GetBuffer(),
                           strname->GetBuffer());
    }
    else
    {
        *res = false;
    }
    return false;
}

bool Context::SetupOverrideUnary(Basic* obj, OpOverride ovr, bool* res)
{
    ASSERT(obj);
    
    Value meth;
    meth.SetNull();
    
    if (GetOverrideFrom(engine, obj, ovr, meth))
    {
        // [   obj  ]
        Push(meth);
        // [   obj  ]
        // [  meth  ]
        if (res)
        {
            *res = true;
        }
        return SetupCall(0);
    }
    else if (!res)
    {
        String* strname = obj->GetType()->GetName();
        ReportRuntimeError(Exception::ERROR_runtime,
                           "Operator %s not defined for object of type %s.",
                           engine->GetOverrideString(ovr)->GetBuffer(),
                           strname->GetBuffer());
    }
    else
    {
        *res = false;
    }
    return false;
}

void Context::OpSuper()
{
    Function* prop = closure;
    String* methname = prop->GetDef()->name;
    bool found = false;
    
    if (closure && closure->IsDerivedFrom(InstanceMethod::StaticGetClass()))
    {
        InstanceMethod* im = (InstanceMethod*)closure;
        Type* t = im->GetClassType();
        
        if (t)
        {
            Type* supertype = t->GetBase();
            Value res;
            Value name(methname);
            
            if (supertype && supertype->GetField(name, res))
            {
                Push(res);
                found = true;
            }
        }
    }
    if (!found)
    {
        const char* type = closure->GetType()->GetName()->GetBuffer();
        ReportRuntimeError(Exception::ERROR_type, "could not find super of %s \"%s\".", type, methname ? methname->GetBuffer() : 0);
    }
}
void Context::OpUsing()
{
    // TODO: If OnUse raises an exception what about OnDispose.
    
    Value& v = Top();
    PushWithScope();
    self = v;                
    Pop();
    
    if (v.tag >= TAG_basic)
    {
        Value res(NULL_VALUE);
        if (v.val.basic->GetSlot(engine->OpUse_String, res))
        {
            Push(v);
            Push(res);
            
            if (SetupCall(0))
            {
                Run();
            }
            Value& res = PopTop(); // result & exitwith string
            if (!res.IsNull())
                self = res;
        }
    }
}

void Context::Activate()   { engine->ChangeContext(this); }

void Context::Deactivate() { engine->ChangeContext(0); }

void Context::ReportRuntimeError(Exception::Kind kind, const char* msg, ...)
{
    static const size_t BUFSZ = 1024;
    char buffer[BUFSZ + 1];
    
    va_list args;
    va_start(args, msg);
    
    Pika_vsnprintf(buffer, BUFSZ, msg, args);
    buffer[BUFSZ] = '\0';
    
    va_end(args);
    code_t* err_pc = pc;
    Function* err_fn = closure;
    
    if (kind == Exception::ERROR_assert) {
        if (scopesTop > scopesBeg) {
            ScopeIter iter = scopesTop;
            --iter;
            err_pc = iter->pc;
            err_fn = iter->closure;
        }
    }
    
    if (!err_fn)
    {
        RaiseException(kind, buffer);
    }
    else
    {
        String* name = err_fn->GetDotPath();
        if (!err_pc)
        {
            RaiseException(kind,
                           "In function '%s': \"%s\"",
                           name->GetBuffer(),
                           buffer);
        }
        else
        {
            int line = err_fn->DetermineLineNumber(err_pc);
            
            RaiseException(kind,
                           "At line %d in function '%s': \"%s\"",
                           line,
                           name->GetBuffer(),
                           buffer);
        }
    }
}

void Context::OpDotSet(Opcode oc, OpOverride ovr)
{
    // [ ...      ]
    // [ value    ]
    // [ object   ]
    // [ property ]< Top
    
    // We want to set object.property = value
    
    Value& prop = Top();
    Value& obj  = Top1();
    Value& val  = Top2();
    
    if (obj.tag >= TAG_basic)
    {
        // Call the opSet override method if present.
    
        Value setfn(NULL_VALUE);
        
        if (GetOverrideFrom(engine, obj.val.basic, ovr, setfn))
        {
            Swap(prop, obj);
            Swap(obj, val);
            
            Push(setfn);
            
            if (SetupCall(2))
            {
                Run();
            }
            
            Pop();
            return;
        }  
        
        bool success = true;
        if (oc == OP_subset)
        {
            if (!(success = obj.val.basic->BracketWrite(prop, val)))
            {
                //  Member cannot be written.
                ReportRuntimeError(Exception::ERROR_runtime,
                                   "Attempt to set [ '%s' ] of '%s'.",
                                   engine->SafeToString(this, prop)->GetBuffer(),
                                   engine->GetTypenameOf(obj)->GetBuffer());
                return;
            }
        }
        else
        {
            success = obj.val.basic->SetSlot(prop, val);
        }
        
        if (!success)
        {
            // If we couldn't write it might be because prop is a property.
            
            Value res;
            if (obj.val.basic->GetSlot(prop, res) && res.tag == TAG_property)
            {
                // res is a Property; So we auto-magically call it.
                
                Property* pprop = res.val.property;
                
                // IF the Property supports writing.
                if (pprop && pprop->CanWrite())
                {
                    //  [ ....  ]
                    //  [ value ]
                    //  [ obj   ]
                    //  [ prop  ]< Top
                    
                    prop.Set(pprop->Writer());
                    
                    //  [ ....            ]
                    //  [ value           ]
                    //  [ obj             ]
                    //  [ setter function ]< Top
                    
                    if (SetupCall(1))
                    {
                        Run();
                    }
                    
                    //  [ ....  ]
                    //  [ value ]< Top
                    
                    Pop();
                    
                    //  [ .... ]< Top
                }
                else
                {
                    //  Property is read only
                    ReportRuntimeError(Exception::ERROR_runtime,
                                       "Attempt to set property '%s' of '%s'. Property does not support writing.",
                                       engine->SafeToString(this, prop)->GetBuffer(),
                                       engine->GetTypenameOf(obj)->GetBuffer());
                    return;
                }
            }
            else
            {
                //  Member cannot be written.
                ReportRuntimeError(Exception::ERROR_runtime,
                                   "Attempt to set member '%s' of '%s'.",
                                   engine->SafeToString(this, prop)->GetBuffer(),
                                   engine->GetTypenameOf(obj)->GetBuffer());
                return;
            }
            return;
        }
        else
        {
            //  Success.
            Pop(3);
        }
    }
    else
    {
        ReportRuntimeError(Exception::ERROR_runtime,
                           "Attempt to set member '%s' of '%s'.",
                           engine->SafeToString(this, prop)->GetBuffer(),
                           engine->GetTypenameOf(obj)->GetBuffer());
        return;
    }
}

void Context::OpDotGet(int& numcalls, Opcode oc, OpOverride ovr)
{
    // [ ...      ]
    // [ object   ]
    // [ property ]< Top
    //
    // we want to push object.property
    
    Value& prop = Top();
    Value& obj  = Top1();
    Value  res(NULL_VALUE);
    
    if (obj.tag < TAG_basic)
    {
        // Null, Integer, Real, Boolean
        
        Type* value_type = 0;
        
        switch (obj.tag)
        {
        case TAG_null:    value_type = engine->Null_Type;    break;
        case TAG_boolean: value_type = engine->Boolean_Type; break;
        case TAG_integer: value_type = engine->Integer_Type; break;
        case TAG_real:    value_type = engine->Real_Type;    break;
        default:
        {
            ReportRuntimeError(Exception::ERROR_runtime,
                               "Attempt to read member '%s' from object of type '%s'.",
                               engine->SafeToString(this, prop)->GetBuffer(),
                               engine->GetTypenameOf(obj)->GetBuffer());
            return;
        }
        }
        
        if (!value_type->GetField(prop, res))
        {
            if (res.tag == TAG_property)
            {
                // The result is a Property
                // So we need to call its accessor function.
                
                Property* property = res.val.property;
                
                // If the Property supports reading
                if (property && property->CanRead())
                {
                    //  [ .... ]
                    //  [ obj  ]
                    //  [ prop ]< Top
                    
                    Pop();
                    
                    //  [ .... ]
                    //  [ obj  ]< Top
                    
                    Push(property->Reader());
                    
                    //  [ .... ]
                    //  [ obj  ]
                    //  [ getter function ]< Top
                    
                    if (SetupCall(0))
                    {
                        ++numcalls;
                    }
                    //  Stack current   | Stack after return
                    //------------------+-------------------------------------------
                    //  [ ..... ] < Top | [ ..... ]
                    //                  | [ value ]
                }
                
            }
            else
            {
#   if defined( PIKA_ALLOW_MISSING_SLOTS )
                res.SetNull();
#   else

                ReportRuntimeError(Exception::ERROR_runtime,
                                   "Attempt to read member '%s' of '%s'.",
                                   engine->SafeToString(this, prop)->GetBuffer(),
                                   engine->SafeToString(this, obj)->GetBuffer());
                return;
#   endif
            }
        }
    }
    else
    {
        // Object, Property, String
        
        // Call the opGet override method if present.
        Basic* basic = obj.val.basic;
        bool success;
        if (oc == OP_subget)
        {
            success = basic->BracketRead(prop, res);
        }
        else
        {
            success = basic->GetSlot(prop, res);
        }
        
        if (!success)
        {
            if (obj.tag >= TAG_basic)
            {
                Value getfn;
                getfn.SetNull();
                
                if (GetOverrideFrom(engine, obj.val.basic, ovr, getfn))
                {
                    Swap(prop, obj);
                    Push(getfn);
                    if (SetupCall(1))
                    {
                        ++numcalls;
                    }
                    return;
                }
            }
#   if defined( PIKA_ALLOW_MISSING_SLOTS )
            res.SetNull();
#   else
            ReportRuntimeError(Exception::ERROR_runtime,
                               "Attempt to read missing member '%s' of '%s'.",
                               engine->SafeToString(this, prop)->GetBuffer(),
                               engine->SafeToString(this, obj)->GetBuffer());
#   endif
            return;
        }
    }
    
    if (res.tag == TAG_property && oc != OP_subget)
    {
        // The result is a Property
        // So we need to call its accessor function.
        
        Property* property = res.val.property;
        
        // If the Property supports reading
        if (property && property->CanRead())
        {
            //  [ .... ]
            //  [ obj  ]
            //  [ prop ]< Top
            
            Pop();
            
            //  [ .... ]
            //  [ obj  ]< Top
            
            Push(property->Reader());
            
            //  [ .... ]
            //  [ obj  ]
            //  [ getter function ]< Top
            
            if (SetupCall(0))
            {
                ++numcalls;
            }
            //  Stack current   | Stack after return
            //------------------+-------------------------------------------
            //  [ ..... ] < Top | [ ..... ]
            //                  | [ value ]
        }
        else
        {
            // Property is write only.
            ReportRuntimeError(Exception::ERROR_runtime,
                               "Attempt to get property '%s' from '%s'. Property does not support reading.",
                               property->Name()->GetBuffer(),
                               engine->SafeToString(this, obj)->GetBuffer());
            return;
        }
    }
    else
    {
        obj = res;
        Pop();
    }
}

bool Context::DoPropertyGet(int& numcalls, Property* prop)
{
    if (!prop->CanRead())
        return false;
        
    Push(prop->Reader());
    
    //  [ .... ]
    //  [ obj  ]
    //  [ getter function ]< Top
    
    if (SetupCall(0))
    {
        ++numcalls;
    }
    //  Stack current   | Stack after return
    //------------------+-------------------------------------------
    //  [ ..... ] < Top | [ ..... ]
    //                  | [ value ]
    return true;
}

bool Context::DoPropertySet(Property* prop)
{
    if (!prop->CanWrite())
        return false;
        
    //  [ ....  ]
    //  [ value ]
    //  [ obj   ]
    //          < Top
    
    Push(prop->Writer());
    
    //  [ ....            ]
    //  [ value           ]
    //  [ obj             ]
    //  [ setter function ]
    //                      < Top
    
    if (SetupCall(1))
    {
        Run();
    }
    
    //  [ ....  ]
    //  [ value ]< Top
    
    Pop();
    
    //  [ .... ]< Top
    return true;
}

void Context::OpReturn(u4 retc)
{
    u4 const expectedRetc = retCount;
    
    if (expectedRetc > 1 && retc == 1)
    {
        const size_t btm  = this->GetStackSize();
        if (OpUnpack(expectedRetc)) {
            this->Run();
            const size_t top = this->GetStackSize();
            size_t amt_returned = top > btm ? top - btm : 0;
            if (amt_returned != expectedRetc) {
                ReportRuntimeError(Exception::ERROR_runtime, "Expected %u return values but %u values were returned.", expectedRetc, amt_returned);
            }
        }
        retc = expectedRetc;
    }
    
    Value* const top = GetStackPtr();
                
    if (env && !env->IsAllocated())
        env->EndCall(); 
        
    PopCallScope();

    CopyReturnValues(expectedRetc, retc, top);
}

void Context::OpYield(u4 yldc)
{
    u4 const expectedRetc = retCount;
    
    if (expectedRetc > 1 &&  yldc == 1)
    {
        const size_t btm  = this->GetStackSize();
        if (OpUnpack(expectedRetc)) {
            this->Run();
            const size_t top = this->GetStackSize();
            size_t amt_yielded = top > btm ? top - btm : 0;
            if (amt_yielded != expectedRetc) {
                ReportRuntimeError(Exception::ERROR_runtime, "Expected %u return values but %u values were yielded.", expectedRetc, amt_yielded);
            }
        }
        yldc = expectedRetc;
    }
    size_t pos = sp - stack;
    sp -= yldc;
    generator->Yield(this);
    
    Value* const top = stack + pos;    
    CopyReturnValues(expectedRetc, yldc, top);
}

void Context::CopyReturnValues(u4 const expectedRetc, u4 const retc, Value const* top)
{
    if (expectedRetc != retc)
    {
        if (expectedRetc == 1)
        {
            if (retc == 0)
            {
                this->PushNull();
            }
            else
            {
                GCPAUSE_NORUN(engine);
                
                Value const* elements = top - retc;
                Object* array = Array::Create(engine, engine->Array_Type, retc, elements);
                this->Push(array);
            }
        }
        else
        {
            ReportRuntimeError(Exception::ERROR_runtime, "Expected %u return values but received %u.", expectedRetc, retc);
        }
    }
    else
    {
        /* Expected number of return values is less-than or equal to the number
         * Provided.
         * 
         * If we expect less return values (N) than given only the last N are
         * provided.
         */
        Value const* startPtr = top - retc;
        Value const* endPtr = startPtr + expectedRetc;
        for (Value const* curr = startPtr; curr != endPtr; ++curr)
        {
            Push(*curr);
        }
    }
}

bool Context::OpUnpack(u4 expected)
{
    Value t = Top();
    Value meth(NULL_VALUE);        
    if (GetOverrideFromValue(engine, t, OVR_unpack, meth))
    {
        CheckStackSpace(3 + expected);
        Push((pint_t)expected);
        Push(t);
        Push(meth);
        return SetupCall(1, expected);
    }
    else if (t.IsDerivedFrom(Array::StaticGetClass()))
    {
        Pop();
        Array* v = (Array*)t.val.object;
        Value* start = GetStackPtr();
        StackAlloc(expected);
        
        if (expected != v->GetLength())
        {
            ReportRuntimeError(Exception::ERROR_runtime, "Expected %u unpacked values but received %u.", expected, v->GetLength());
        }
        
        Pika_memcpy(start, v->GetAt(0), expected * sizeof(Value));
        return false;
    }
    ReportRuntimeError(Exception::ERROR_runtime,
                       "Attempt to unpack %u elements from object of type '%s'.",
                       expected,
                       engine->GetTypenameOf(t)->GetBuffer());
    return false;
}// OpUnpack

bool Context::OpBind()
{
    Value& b = Top();
    Value& a = Top1();
    
    // bind b.a 
    // bind b[a]
    
    if (a.tag >= TAG_basic)
    {
        if (a.IsDerivedFrom(Function::StaticGetClass()))
        {
            Function* c = ((Function*)a.val.function)->BindWith(b);
            Pop();
            Top().Set(c);
            return false;
        }
        else
        {
            Basic* basic = a.val.basic;
            OpOverride ovr = OVR_bind;
            bool res = false;
            
            if (SetupOverrideLhs(basic, ovr, &res))
            {
                return true;
            }
            else if (res)
            {
                return false;
            }
        }
    }
    
    if (b.tag >= TAG_basic)
    {
        Basic* basic = b.val.basic;
        OpOverride ovr = OVR_bind_r;
        bool res = false;
        
        if (SetupOverrideRhs(basic, ovr, &res))
        {
            return true;
        }
        else if (res)
        {
            return false;
        }
    }
    ReportRuntimeError(Exception::ERROR_runtime,
                       "Unsupported operands for bind operator; types '%s' and '%s' used.",
                       engine->GetTypenameOf(a)->GetBuffer(),
                       engine->GetTypenameOf(b)->GetBuffer());
    return false;
}// OpBind

bool Context::OpCat(bool sp)
{
    Value& b = Top();
    Value& a = Top1();
    
    if (a.tag == TAG_object)
    {
        Object* obj = a.val.object;
        OpOverride ovr = sp ? OVR_catsp : OVR_cat;
        bool res = false;
        if (SetupOverrideLhs(obj, ovr, &res))
        {
            return true;
        }
        else if (res)
        {
            return false;
        }
    }
    if (b.tag == TAG_object)
    {
        Object* obj = b.val.object;
        OpOverride ovr = sp ? OVR_catsp_r : OVR_cat_r;
        bool res = false;
        if (SetupOverrideRhs(obj, ovr, &res))
        {
            return true;
        }
        else if (res)
        {
            return false;
        }
    }
    String* stra = engine->ToString(this, a);
    String* strb = engine->ToString(this, b);
    
    Value res;
    res.tag = TAG_string;
    
    if (stra->GetLength() == 0)
    {
        res.val.str = strb;
    }
    else if (strb->GetLength() == 0)
    {
        res.val.str = stra;
    }
    else
    {
        res.val.str = sp ? String::ConcatSpace(stra, strb) : String::Concat(stra, strb);
    }
    Pop();
    Top() = res;
    return false;
}// OpCat

bool Context::OpIs()
{
    Value& b = Top();
    Value& a = Top1();
    
    if (b.IsDerivedFrom(Type::StaticGetClass()))
    {
        Type* typeObj = static_cast<Type*>(b.val.object);
        
        if (a.tag >= TAG_basic)
        {
            Basic* obja  = a.val.basic;
            Type*  typea = obja->GetType();
            
            for (Type* curr = typea; curr; curr = curr->GetBase())
            {
                if (curr == typeObj)
                {
                    return true;
                    
                }
            }
        }
        else
        {
            switch (a.tag)
            {
            case TAG_null:    return(typeObj == engine->Null_Type);
            case TAG_boolean: return(typeObj == engine->Boolean_Type);
            case TAG_integer: return(typeObj == engine->Integer_Type);
            case TAG_real:    return(typeObj == engine->Real_Type);
            default:          return false;
            }
        }
    }
    return false;
}

bool Context::OpHas()
{
    Value& b = Top();
    Value& a = Top1();
    Basic* obj = 0;
    switch (a.tag)
    {
    case TAG_null:    obj = engine->Null_Type;    break;
    case TAG_boolean: obj = engine->Boolean_Type; break;
    case TAG_integer: obj = engine->Integer_Type; break;
    case TAG_real:    obj = engine->Real_Type;    break;
    default:          obj = a.val.basic;
    }
    if (obj && obj->HasSlot(b))
        return true;
    return false;
}

#include "PContext_Run.inl"

void Context::GrowScopeStack()
{
    size_t apidx     = scopes.IndexOf(scopesTop);
    size_t currsz    = scopes.GetSize();
    size_t desiredsz = currsz * 2;
    
    desiredsz = Clamp<size_t>(desiredsz, currsz, PIKA_MAX_SCOPE_STACK);
    
    if (desiredsz <= currsz)
    {
        RaiseException("Scope stack overflow. Recursion limit of %d reached.", PIKA_MAX_SCOPE_STACK);
    }
    
    scopes.Resize(desiredsz);
    scopesTop = scopes.At(apidx);
    scopesBeg = scopes.Begin();
    scopesEnd = scopes.End();
}

Value& Context::GetOuter(u2 idx, u1 depth)
{
    Function* curr = closure;
    int n = depth;
    
    while (--n) // Must use prefix operator--.
    {
        ASSERT(curr);
        curr = curr->parent;
    }
    ASSERT(curr);
#if !defined (PIKA_CHECK_LEX_ENV)
    if ( !curr || !curr->lexEnv )
    {
        // Shouldn't happen under properly compiled script.
        RaiseException(Exception::ERROR_index, "Attempt to access invalid bound variable of depth %d and index of %d.",
                       depth,
                       idx);
    }
    else if (curr->lexEnv->Length() <= idx)
    {
        RaiseException(Exception::ERROR_index, "Attempt to access invalid bound variable of index %d at depth %d.",
                       depth,
                       idx);
    }
#endif
    return curr->lexEnv->At(idx);
}

void Context::SetOuter(const Value& outer, u2 idx, u1 depth)
{
    Value& outv = GetOuter(idx, depth);
    outv = outer;
}

void Context::DoClosure(Def* fun, Value& retframe, Value* mself)
{
    ASSERT(fun);
    GCPAUSE(engine);
    
    Function* f = 0;
    ScopeInfo& currA = *(scopesTop - 1);
    if (fun->name == 0 || fun->name == engine->emptyString)
    {
        f = Function::Create(engine, fun, package, closure);
    }
    else if (mself && (mself->IsDerivedFrom(Type::StaticGetClass())))
    {
        f = ClassMethod::Create(engine, 0, closure, fun, package, mself->IsNull() ? self.val.type : mself->val.type);
    }
    else if (currA.kind == SCOPE_package && package->IsDerivedFrom(Type::StaticGetClass()))
    {
        f = InstanceMethod::Create(engine, 0, closure, fun, package, (Type*)package);
    }
    else
    {
        f = Function::Create(engine, fun, package, closure);
    }
    ASSERT(f);
    f->lexEnv = env;
    retframe.Set(f);
}

Context* Context::Create(Engine* eng, Type* obj_type)
{
    Context* newco = 0;
    PIKA_NEW(Context, newco, (eng, obj_type));
    eng->GetGC()->ForceToGray(newco);
    return newco;
}

Object* Context::Clone()
{
    return 0;
}

void Context::Init(Context* ctx)
{
    if (ctx->GetArgCount() != 1)
        RaiseException("Context.init requires exactly 1 argument %d given.", ctx->GetArgCount());
    if (state != UNUSED)
        RaiseException("Cannot initialize a context that has been used.");
    Value& f = ctx->GetArg(0);
    WriteBarrier(f);
    Push(f);
}

void Context::Setup(Context* ctx)
{
    if (this == ctx)
    {
        this->ReportRuntimeError(Exception::ERROR_runtime, "A context cannot call itself.");
        return;
    }
    
    /*  Stack:
     *  [ ...      ]
     *  [ function ]
     *              < sp
     *
     *  The function we are using as an entry-point should be pushed onto the stack.
     */
    if (sp <= bsp)
    {
        RaiseException("Context not initialized. Please pass the function to execute to the context's constructor.");
        return;
    }
    
    if (IsUnused())
    {
        prev = ctx;
        
        Value fn = PopTop();  
        /*  Stack:
         *  [ ... ]
         *         < sp
         */
        u2 argc = ctx->GetArgCount();
        
        CheckStackSpace(argc + 3);
        
        Value* argv = ctx->GetArgs();
        
        // push the arguments.
        for (u2 a = 0; a < argc; ++a)
        {
            Value& v = argv[a];
            WriteBarrier(v);
            Push(v);
        }
        
        PushNull();
        Push(fn);      // re-push function
        
        /*  Stack:
         *  [ ..... ]
         *  [ arg 0 ]
         *  [ ..... ]
         *  [ arg N ]
         *  [ null  ]
         *  [ fn    ]
         *           < sp
         */
        if (SetupCall(argc))
        {
            // Put the Context into a suspended state so that any subsequent calls to Context.Call
            // will succeed.
            callsCount = 1;
            state = SUSPENDED;
        }
        else
        {
            state = DEAD;
        }
    }
    else
    {
        RaiseException("Attempt to setup a context that is '%s'.",
                       GetContextStateName(this->state));
        return;
    }
}

void Context::Call(Context* ctx, u2 rc)
{
    if (this == ctx)
    {
        RaiseException("context cannot call itself.");
        return;
    }
    this->retCount = rc;
    this->prev = ctx;
    
    if (IsSuspended())
    {
        return DoResume();
    }
    else
    {
        RaiseException("Attempt to call or resume a context that is '%s'.",
                       GetContextStateName(this->state));
        return;
    }
}

void Context::Suspend(Context* ctx)
{
    if (!ctx->prev)
    {
        ctx->ReportRuntimeError(Exception::ERROR_runtime,
                                "cannot yield from this context: nothing to yield to.");
    }
    
    if (ctx->nativeCallDepth > 1) // 1 is for this function call
    {
        ctx-> ReportRuntimeError(Exception::ERROR_runtime,
                                 "cannot yield across a native call.");
    }
    
    u4 count = ctx->GetArgCount();
    ctx->DoSuspend(ctx->GetArgs(), count);
}

void Context::MarkRefs(Collector* c)
{
    ThisSuper::MarkRefs(c);
    
    ScopeIter a = scopes.Begin();
    Buffer<ExceptionBlock>::Iterator e = handlers.Begin();
    
    if (prev)
    {
        prev->Mark(c);
    }
    
    // Mark previous scopes.
    
    for (; a != scopesTop; ++a)
    {
        a->DoMark(c);
        
        // TODO: Would it be faster to just mark the 'ENTIRE' stack from top to bottom?
        //       What I mean is there ever a gap between scope stack ranges?
        
        if (a->kind == SCOPE_call)
        {
            MarkValues(c, stack + a->stackBase, stack + a->stackTop);
        }
    }
    
    // Mark exception handlers.
    
    for (; e != handlers.End(); ++e)
    {
        e->DoMark(c);
    }
    
    // Mark current scope.
    
    if (env)       env->Mark(c);
    if (closure)   closure->Mark(c);
    if (generator) generator->Mark(c);
    if (package)   package->Mark(c);
    if (kwargs)    kwargs->Mark(c);

    MarkValue(c, acc);
    MarkValue(c,  self);
    MarkValues(c, bsp, sp);
}

String* Context::GetFunctionName()
{
    return (closure) ? closure->GetDef()->name : engine->emptyString;
}

String* Context::GetPackageName(bool fullyQualified)
{
    return (fullyQualified) ? package->GetDotName() : package->GetName();
}

void Context::CheckArgCount(u2 amt)
{
    if (argCount != amt)
    {
        String* funname = GetFunctionName();
        String* dotname = GetPackageName(true);
        
        if (argCount > amt)
        {
            RaiseException(Exception::ERROR_runtime,
                           "Too many arguments for function '%s.%s'. Expected %d %s but was given %d.",
                           dotname->GetBuffer(),
                           funname->GetBuffer(),
                           amt,
                           amt == 1 ? "argument" : "arguments",
                           argCount);
        }
        else
        {
            RaiseException(Exception::ERROR_runtime,
                           "Too few arguments for function '%s.%s'. Expected %d %s but was given %d.",
                           dotname->GetBuffer(),
                           funname->GetBuffer(),
                           amt,
                           amt == 1 ? "argument" : "arguments",
                           argCount);
        }
    }
}

void Context::WrongArgCount()
{
    String* funname = GetFunctionName();
    String* dotname = GetPackageName(true);
    
    RaiseException(Exception::ERROR_runtime,
                   "Incorrect number of arguments (%d) given for function '%s.%s'.",
                   argCount,
                   dotname->GetBuffer(),
                   funname->GetBuffer());
}

void Context::ArgumentTagError(u2 arg, ValueTag given, ValueTag expected)
{
    String* funname = GetFunctionName();
    String* dotname = GetPackageName(true);
    
    RaiseException(Exception::ERROR_runtime,
                   "Incorrect type '%s' for function %s.%s, argument (%d). Expecting type '%s'.",
                   GetTypeString(given),
                   dotname->GetBuffer(),
                   funname->GetBuffer(),
                   arg,
                   GetTypeString(expected));
}

pint_t Context::GetIntArg(u2 arg)
{
    Value& v = GetArg(arg);
    
    if (v.tag != TAG_integer)
    {
        if (v.tag == TAG_real && Pika_RealIsInteger(v.val.real))
        {
            return Pika_RealToInteger(v.val.real);
        }
        ArgumentTagError(arg, (ValueTag)v.tag, TAG_integer);
    }
    
    return v.val.integer;
}

preal_t Context::GetRealArg(u2 arg)
{
    Value& v = GetArg(arg);
    
    if (v.tag != TAG_real)
    {
        if (v.tag == TAG_integer)
        {
            return (preal_t)v.val.integer;
        }
        ArgumentTagError(arg, (ValueTag)v.tag, TAG_real);
    }
    
    return v.val.real;
}

bool Context::GetBoolArg(u2 arg)
{
    Value& v = GetArg(arg);
    
    if (v.tag != TAG_boolean)
    {
        ArgumentTagError(arg, (ValueTag)v.tag, TAG_boolean);
    }
    return v.val.index ? true : false;
}

String* Context::GetStringArg(u2 arg)
{
    Value& v = GetArg(arg);
    
    if (v.tag != TAG_string)
    {
        ArgumentTagError(arg, (ValueTag)v.tag, TAG_string);
    }
    return v.val.str;
}

Object* Context::GetObjectArg(u2 arg)
{
    Value& v = GetArg(arg);
    
    if (v.tag != TAG_object)
    {
        ArgumentTagError(arg, (ValueTag)v.tag, TAG_object);
    }
    return v.val.object;
}

Property* Context::GetPropertyArg(u2 arg)
{
    Value& v = GetArg(arg);
    
    if (v.tag != TAG_property)
    {
        ArgumentTagError(arg, (ValueTag)v.tag, TAG_property);
    }
    return v.val.property;
}

void* Context::GetUserDataArg(u2 arg, UserDataInfo* info)
{
    Value& v = GetArg(arg);
    
    if (v.tag != TAG_userdata)
    {
        ArgumentTagError(arg, (ValueTag)v.tag, TAG_userdata);
    }
    else if (v.val.userdata->GetInfo() != info)
    {
        ReportRuntimeError(Exception::ERROR_type, "incorrect type '%s' for argument %d. expecting type '%s'.", v.val.userdata->GetInfo()->name, info->name, arg);
    }
    return v.val.userdata->GetData();
}

pint_t Context::ArgToInt(u2 arg)
{
    Value& v = GetArg(arg);
    
    if (v.tag != TAG_integer)
    {
        Value temp = v;
        if (!engine->ToIntegerExplicit(this, temp))
        {
            ArgumentTagError(arg, (ValueTag)temp.tag, TAG_integer);
        }
        return temp.val.integer;
    }
    return v.val.integer;
}

preal_t Context::ArgToReal(u2 arg)
{
    Value& v = GetArg(arg);
    
    if (v.tag != TAG_real)
    {
        Value temp = v;
        if (!engine->ToRealExplicit(this, temp))
        {
            ArgumentTagError(arg, (ValueTag)temp.tag, TAG_real);
        }
        return temp.val.real;
    }
    return v.val.real;
}

bool Context::ArgToBool(u2 arg)
{
    Value& v = GetArg(arg);
    
    if (v.tag != TAG_boolean)
    {
        Value temp = v;
        return engine->ToBoolean(this, temp);
    }
    return v.val.index ? true : false;
}

String* Context::ArgToString(u2 arg)
{
    Value& v = GetArg(arg);
    
    if (v.tag != TAG_string)
    {
        Value temp = v;
        return engine->ToString(this, temp);
    }
    return v.val.str;
}

void Context::ParseArgs(const char *args, u2 count, ...)
{
    if (count > argCount)
    {
        CheckArgCount(count);
        return;
    }
    
    const char* currArg = args;
    const char* endArg  = args + count;
    Value*      currVar = GetArgs();
    
    va_list va;
    va_start(va, count);
    
    while (currArg < endArg)
    {
        char a = *currArg++;
        switch (a)
        {
            /* ================ Boolean ================ */
        case 'B':
        case 'b':
        {
            if (currVar->tag != TAG_boolean)
            {
                u2 argpos = (u2)(currVar - bsp);
                
                if (a == 'B')
                {
                    bool bres = engine->ToBoolean(this, *currVar);
                    currVar->SetBool(bres);
                }
                else
                {
                    ArgumentTagError(argpos, (ValueTag)currVar->tag, TAG_boolean);
                }
            }
            bool *bp = va_arg(va, bool*);
            *bp = currVar->val.index ? true : false;
        }
        break;
        /* ================ c string ================ */
        case 'C':
        case 'c':
        {
            if (currVar->tag != TAG_string)
            {
                u2 argpos = (u2)(currVar - bsp);
                
                if (a == 'C')
                {
                    String* str = engine->ToString(this, *currVar);
                    
                    if (!str)
                        ArgumentTagError(argpos, (ValueTag)currVar->tag, TAG_string);
                    else
                        currVar->Set(str);
                }
            }
            const char** strpp = va_arg(va, const char**);
            *strpp = currVar->val.str->GetBuffer();
        }
        break;
        /* ================ Integer ================ */
        case 'I':
        case 'i':
        {
            if (currVar->tag != TAG_integer)
            {
                u2 argpos = (u2)(currVar - bsp);
                
                if (a == 'I' && !engine->ToIntegerExplicit(this, *currVar))
                {
                    ArgumentTagError(argpos, (ValueTag)currVar->tag, TAG_integer);
                }
            }
            
            pint_t* intp = va_arg(va, pint_t*);
            *intp = currVar->val.integer;
        }
        break;
        /* ================ Object ================ */
        case 'O':
        case 'o':
        {
            if (currVar->tag != TAG_object)
            {
                u2 argpos = (u2)(currVar - bsp);
                ArgumentTagError(argpos, (ValueTag)currVar->tag, TAG_object);
            }
            
            Object** objpp = va_arg(va, Object**);
            *objpp = currVar->val.object;
        }
        break;
        /* ================ Real ================ */
        case 'R':
        case 'r':
        {
            if (currVar->tag != TAG_real)
            {
                u2 argpos = (u2)(currVar - bsp);
                
                if (a == 'r' && !engine->ToRealExplicit(this, *currVar))
                {
                    ArgumentTagError(argpos, (ValueTag)currVar->tag, TAG_real);
                }
            }
            
            preal_t* fltp = va_arg(va, preal_t*);
            *fltp = currVar->val.real;
        }
        break;
        
        /* ================ String ================ */
        case 'S':
        case 's':
        {
            if (currVar->tag != TAG_string)
            {
                u2 argpos = (u2)(currVar - bsp);
                
                if (a == 'S')
                {
                    String* str = engine->ToString(this, *currVar);
                    
                    if (!str)
                        ArgumentTagError(argpos, (ValueTag)currVar->tag, TAG_string);
                    else
                        currVar->Set(str);
                }
            }
            
            String** strpp = va_arg(va, String**);
            *strpp = currVar->val.str;
        }
        break;
        
        case 'U':
        case 'u':
        {
            if (currVar->tag != TAG_userdata)
            {
                u2 argpos = (u2)(currVar - bsp);
                ArgumentTagError(argpos, (ValueTag)currVar->tag, TAG_userdata);
            }
            
            UserData** userdatapp = va_arg(va, UserData**);
            *userdatapp = currVar->val.userdata;
        }
        break;
        
        case 'X': // skip
        case 'x': // skip
            break;
        };
        
        currVar++;
    }
    va_end(va);
}

void Context::ParseArgsInPlace(const char *args, u2 count)
{
    if (count > argCount)
    {
        CheckArgCount(count);
        return;
    }
    
    const char* currArg = args;
    const char* endArg  = args + count;
    Value*      currVar = GetArgs();
    
    while (currArg < endArg)
    {
        char a = *currArg++;
        
        if (a < 0)
        {
            if (currVar->tag == TAG_null)
            {
                currVar++;
                a = *currArg++;
                continue;
            }
            a = -a;
        }
        
        switch (a)
        {
        
            /* Boolean */
        case 'B':
        case 'b':
        {
            if (currVar->tag != TAG_boolean)
            {
                u2 argpos = (u2)(currVar - bsp);
                Value v = *currVar;
                bool res = false;
                
                if ((a & 0x7F) == 'B')
                {
                    res = engine->ToBoolean(this, v);
                }
                else
                {
                    ArgumentTagError(argpos, (ValueTag)currVar->tag, TAG_boolean);
                }
                
                currVar = bsp + argpos;
                currVar->SetBool(res);
            }
        }
        break;
       
        /* Integer */
        case 'I':
        case 'i':
        {
            if (currVar->tag != TAG_integer)
            {
                u2 argpos = (u2)(currVar - bsp);
                
                Value v = *currVar;
                
                if (a == 'I')
                {
                    if (!engine->ToInteger(v))
                    {
                        ArgumentTagError(argpos, (ValueTag)currVar->tag, TAG_integer);
                    }
                }
                else
                {
                    ArgumentTagError(argpos, (ValueTag)currVar->tag, TAG_integer);
                }
                currVar = bsp + argpos;
                *currVar = v;
            }
        }
        break;
        
        /* Object */
        case 'O':
        case 'o':
        {
            if (currVar->tag != TAG_object)
            {
                u2 argpos = (u2)(currVar - bsp);
                ArgumentTagError(argpos, (ValueTag)currVar->tag, TAG_object);
            }
        }
        break;
        
        /* Real */
        case 'R':
        case 'r':
        {
            if (currVar->tag != TAG_real)
            {
                u2 argpos = (u2)(currVar - bsp);
                
                Value v = *currVar;
                
                if (a == 'r')
                {
                    if (!engine->ToReal(v))
                    {
                        ArgumentTagError(argpos, (ValueTag)currVar->tag, TAG_real);
                    }
                }
                else
                {
                    ArgumentTagError(argpos, (ValueTag)currVar->tag, TAG_real);
                }
                
                currVar = bsp + argpos;
                *currVar = v;
            }
        }
        break;
        
        /* String */
        case 'S':
        case 's':
        {
            if (currVar->tag != TAG_string)
            {
                u2 argpos = (u2)(currVar - bsp);
                Value v = *currVar;
                String* str = 0;
                
                if (a == 'S')
                {
                    str = engine->ToString(this, v);
                    
                    if (!str)
                    {
                        ArgumentTagError(argpos, (ValueTag)currVar->tag, TAG_string);
                    }
                }
                else
                {
                    ArgumentTagError(argpos, (ValueTag)currVar->tag, TAG_string);
                }
                
                currVar = bsp + argpos;
                currVar->Set(str);
            }
        }
        break;
        
        /* UserData */
        case 'U':
        case 'u':
        {
            if (currVar->tag != TAG_userdata)
            {
                u2 argpos = (u2)(currVar - bsp);
                ArgumentTagError(argpos, (ValueTag)currVar->tag, TAG_userdata);
            }
        }
        break;
        
        /* Skip */
        case  'X':
        case  'x':
            break;
        };
        
        currVar++;
    }
}

void Context::Traceback()
{
    GCPAUSE_NORUN(this->engine);
    
    if (!this->closure)
        return;
    this->PushCallScope();
    std::cout << "Traceback for context " << "<0x"<< (size_t)(this) << ">" << " (most recent call first):" << std::endl;
    for (ScopeIter iter = this->scopesTop-1; iter > this->scopesBeg; iter--)
    {
        if (iter->closure)
        {
            const bool is_native = iter->closure->IsNative();
            const char* name = iter->closure->GetDotPath()->GetBuffer();
            std::cout << "\t* In "<< (is_native ? "Native " : "") <<"function \'" << name << "\'";
            if (!is_native && iter->pc)
            {
                int const line_no = iter->closure->DetermineLineNumber(iter->pc);
                std::cout << ", at line " << line_no;
                
                String* file_name_string = iter->closure->GetFileName();
                if (file_name_string->GetLength()) {
                    const char* file_name = file_name_string->GetBuffer();
                    std::cout << " in file \'" << file_name << "\'";
                }                
            }
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;
    this->PopCallScope();
}

bool Context::HandleException(Value& thrown, bool& inlineThrow) {
    ScriptException exception(thrown);
    
    switch (OpException(exception, false))
    {
    case ER_throw:
        inlineThrow = true; // The exception handler inside run should re-throw this exception.
        throw exception;    // "pass" it off to the exception handler ... avert your eyes **yuck**
    case ER_continue:
    {                    
        return false;
    }
    case ER_exit:
        return true;             // We are dead, nothing to do but exit.
    default:
        inlineThrow = true;
        throw exception;
    }
}

Context::EErrRes Context::OpException(Exception& e, bool caught)
{
    Exception::Kind ekind = e.kind;
    
    if (handlers.GetSize() != 0)
    {
        Value thrown;
        Engine::CollectorPauseNoRun gpc(engine);
        
        if (ekind == Exception::ERROR_script)
        {
            ScriptException eng = static_cast<ScriptException&>(e);
            thrown = eng.var;
        }
        else
        {
            Type* error_type = e.GetErrorType(this->engine);
            if (error_type)
            {
                error_type->CreateInstance(thrown);
            }
            else
            {
                switch (e.kind)
                {
                case Exception::ERROR_syntax:      engine->SyntaxError_Type->CreateInstance(thrown);       break;
                case Exception::ERROR_runtime:     engine->RuntimeError_Type->CreateInstance(thrown);      break;
                case Exception::ERROR_arithmetic:  engine->ArithmeticError_Type->CreateInstance(thrown);   break;
                case Exception::ERROR_underflow:   engine->UnderflowError_Type->CreateInstance(thrown);    break;
                case Exception::ERROR_overflow:    engine->OverflowError_Type->CreateInstance(thrown);     break;
                case Exception::ERROR_dividebyzero:engine->DivideByZeroError_Type->CreateInstance(thrown); break;
                case Exception::ERROR_index:       engine->IndexError_Type->CreateInstance(thrown);        break;
                case Exception::ERROR_type:        engine->TypeError_Type->CreateInstance(thrown);         break;
                case Exception::ERROR_system:      engine->SystemError_Type->CreateInstance(thrown);       break;
                case Exception::ERROR_assert:      engine->AssertError_Type->CreateInstance(thrown);       break;
                default:                           engine->Error_Type->CreateInstance(thrown);
                }
            }
            const char* const msg = e.GetMessage();
            String* const errorStr = msg ? engine->GetString(msg) : engine->emptyString;
            
            if (thrown.IsObject() && thrown.val.object)
            {
                Object* error_obj = thrown.val.object;
                error_obj->SetSlot(engine->message_String, errorStr);
                error_obj->SetSlot(engine->GetString("name"), engine->GetString(e.GetErrorKind()));
            }
            else
            {
                thrown.Set(errorStr);
            }
        }
        
        ExceptionBlock& eb = handlers.Back();
        size_t curra = scopes.IndexOf(scopesTop);
        
        ASSERT(curra >= eb.scope);
        
        if (curra != eb.scope)
        {
            ASSERT(curra);
            
            // Close the current frame b/c we are exiting it?
            
            if (env)
            {
                env->EndCall();
            }
            if (generator)
            {
                generator->Return();
            }
            
            // Check if the current scope is native. If so then its stack has
            // already been unwound.
            /*
            ===============================
            A           B           C
            [c++]       [pika]      [c++]       <- currently_native is true iff the top scope is C++            -------------------------------
            [c++]       [pika]      [pika]      <- If we encounter a non-native scope we set it false
            [c++]       [pika]      [c++]       <- If we encounter a C++ scope when the previous was non-native we need to throw an exception
            [pika]      [pika]      [pika]      <- In that case if doesn't matter what comes next we just need to raise an exception and let it pick up at an ealier point.
                                    [...]
            
            ER_continue ER_continue ER_throw
                       
            
            */
            bool currently_native = closure && closure->def->nativecode;
            
            PopScope();
            
            // Close every frame up-to but not including the frame
            // were the catch handler lives.
            
            for (size_t a = curra - 1; a > eb.scope; --a)
            {
                if (scopes[a].kind == SCOPE_call && scopes[a].closure)
                {
                    // Make sure no scopes between the current one and the try block
                    // contains a native function call.
                    
                    if (scopes[a].pc == 0)
                    {
                        // Native function.
                        // If it was caught by Context::Run, then the C++ stack
                        // has already unwound.
                        // TODO: only decrement numRuns when previous call was non-native???
                                                
                        if (!currently_native || !caught) {
                                --numRuns;
                                return ER_throw;
                        } else {
                            --nativeCallDepth;
                        }
                    }
                    else if (scopes[a].env)
                    {
                        currently_native = false;
                        scopes[a].env->EndCall();
                    }
                    else {
                        currently_native = false;
                    }
                    
                    if (scopes[a].generator)
                    {
                        scopes[a].generator->Return();
                    }
                }
                // Unwind.
                PopScope();
            }
            ASSERT(scopes.IndexOf(scopesTop) == eb.scope);
            
            pc      = closure->GetBytecode() + eb.catchpc;
            sp      = stack + eb.sp;
            package = eb.package;
            self    = eb.self;
            
            if (env && !env->IsAllocated())
            {
                env->Set(bsp, closure->def->numLocals);
            }
        }
        else
        {
            // Catch handler is in the same scope as the exception.
            pc      = closure->GetBytecode() + eb.catchpc;
            sp      = stack + eb.sp;
            package = eb.package;
            self    = eb.self;
        }
        
        // set the thrown object for the catch block.
        SafePush(thrown);
        handlers.Pop();
        
        return ER_continue;
    }
    else
    {
        // No exception handler so we need to kill this context and either pass the
        // exception on to our parent or quit.
        
        state = UNUSED;
        Deactivate();
        --numRuns;
        
        this->Traceback();
        
        if ((prev && prev->IsRunning()) || (numRuns > 0))
        {
            // We can't handle the exception here so we pass it on.
            return ER_throw;
        }
        else
        {
            // Let the user know the exception was unhandled and exit this Context.
            const char* msg = e.GetMessage();
            if (msg)
            {
                // If the exception was raised inside a script
                if (e.kind == Exception::ERROR_script)
                {
                    ScriptException& se = static_cast<ScriptException&>(e);
                    
                    // If they used an instance of an Error derived class.
                    if (engine->Error_Type->IsInstance(se.var))
                    {
                        String* typestr = engine->GetTypenameOf(se.var);
                        // unhandled TYPE -- MESSAGE\n
                        std::cerr << "*** Unhandled " << typestr->GetBuffer() << " -- " << msg << std::endl;
                    }
                    // A non-error values was raised.
                    else
                    {
                        // unhandled exception -- MESSAGE
                        std::cerr << "*** Unhandled exception -- " << msg << std::endl;
                    }
                }
                else
                {
                    std::cerr << e.GetErrorKind() << " -- " << msg << std::endl;
                }
            }
            else
            {
                std::cerr << "*** Unhandled exception ***" << std::endl;
            }
        }
        return ER_exit;
    }
}

}// pika
