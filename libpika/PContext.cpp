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
#include "PEnumerator.h"
#include "PValueEnumerator.h"
#include "PNativeBind.h"
#include <iostream>

const char* GetContextStateName(Context::EState state)
{
    switch (state)
    {
    case Context::SUSPENDED: return "suspended";
    case Context::RUNNING:   return "running";
    case Context::DEAD:      return "dead";
    case Context::INVALID:   return "invalid";
    default:                 return "uninitialized"; // context's state member is uninitialized garbage.
    }
}

#include "PContext_ops.inl"

void Context_NewFn(Engine* eng, Type* obj_type, Value& res)
{
    Context* new_ctx = Context::Create(eng, obj_type);
    res.Set(new_ctx);
}

int Context_call(Context* ctx, Value& self)
{
    Context* callee = (Context*)self.val.object;
    
    // call the context with the expected number of args
    callee->Call(ctx, ctx->GetRetCount());
    
    // Calculate the actual number of values pushed onto our stack.
    // (If the context returned it will match the calling ctx's retCount otherwise it is
    // the number of values yielded.)
    const Value* top = ctx->GetStackPtr() - 1;
    const Value* btm = ctx->GetArgs() + ctx->GetArgCount();
    ptrdiff_t retc = top > btm ? top - btm : 0;
    return retc;
}

int Context_Setup(Context* ctx, Value& self)
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

void InitContextAPI(Engine* eng)
{
    String* Context_String = eng->AllocString("Context");
    eng->Context_Type = Type::Create(eng, Context_String, eng->Object_Type, Context_NewFn, eng->GetWorld());
    
    static RegisterFunction Context_Methods[] =
    {
        {"call",  Context_call,  0, 0, 1 },
        {"setup", Context_Setup, 0, 1, 0 },
    };
        
    SlotBinder<Context>(eng, eng->Context_Type)
    .Constant((pint_t)Context::SUSPENDED,   "SUSPENDED")
    .Constant((pint_t)Context::RUNNING,     "RUNNING")
    .Constant((pint_t)Context::DEAD,        "DEAD")
    .Constant((pint_t)Context::INVALID,     "INVALID")
    .StaticMethodVA(&Context::Suspend,      "suspend")
    .Method(&Context::IsSuspended,          "suspended?")
    .Method(&Context::IsRunning,            "running?")
    .Method(&Context::IsDead,               "dead?")
    .Method(&Context::IsInvalid,            "invalid?")
    .Method(&Context::ToBoolean,            "toBoolean")
    .PropertyR("state", &Context::GetState, "getState")
    ;
    eng->Context_Type->EnterMethods(Context_Methods, countof(Context_Methods));
    eng->GetWorld()->SetSlot(Context_String, eng->Context_Type);
}

PIKA_FORCE_INLINE void MarkValues(Collector* c, Value *begin, Value* end)
{
    for (Value *curr = begin; curr < end; ++curr)
    {
        MarkValue(c, *curr);
    }
}

namespace pika
{

// Enumerates each yielded value for the given Context.
class ContextEnum : public Enumerator
{
public:
    ContextEnum(Engine* eng, Context* ctx)
            : Enumerator(eng), val(NULL_VALUE), context(ctx), prev(ctx->prev)
    {}
    
    virtual ~ContextEnum()
    {}
    
    virtual bool Rewind()
    {
        val.SetNull();
        Advance();
        return IsValid();
    }
    
    virtual bool IsValid()
    {
        return(context && (context->GetState() == Context::SUSPENDED)) || (context->GetState() == Context::RUNNING);
    }
    
    virtual void GetCurrent(Value& curr)
    {
        curr = val;
    }
    
    virtual void Advance()
    {
        if (IsValid())
        {
            context->Call(prev, 1);
            val = prev->PopTop();
        }
    }
private:
    Value val;
    Context* context;
    Context* prev;
};

void ScopeInfo::DoMark(Collector* c)
{
    if (closure) closure->Mark(c);
    if (package) package->Mark(c);
    if (env)     env->Mark(c);
    
    MarkValue(c, self);
}

void ExceptionBlock::DoMark(Collector* c)
{
    if (package)
        package->Mark(c);
    MarkValue(c, self);
}

bool GetOverrideFrom(Engine* eng, Basic* obj, OpOverride ovr, Value& res)
{
    String* what = eng->GetOverrideString(ovr);
    Value vwhat(what);
    Type* objType = obj->GetType();
    return objType->GetField(vwhat, res); // XXX: Get Override
}

PIKA_IMPL(Context)

PIKA_FORCE_INLINE void Context::PushCallScope()
{
    ScopeInfo& currA = *scopesTop;
    
    currA.env = env;
    currA.pc = pc;
    currA.self = self;
    currA.stackTop = sp - stack;
    currA.stackBase = bsp - stack;
    currA.closure = closure;
    currA.newCall = newCall;
    currA.argCount = argCount;
    currA.retCount = retCount;
    currA.numTailCalls = numTailCalls;
    currA.package = package;
    currA.kind = SCOPE_call;
    if (++scopesTop >= scopesEnd)
        GrowScopeStack();
}

/** Pushes a with scope. Can be a class or a with statement. */
void Context::PushWithScope()
{
    ScopeInfo& currA = *scopesTop;
    
    // We only care about the self and kind fields but we want to null
    // anything the gc may want to mark.
    currA.env = 0;
    currA.closure = 0;
    currA.package = 0;
    currA.self = self;       // all we care about.
    currA.kind = SCOPE_with;
    
    if (++scopesTop >= scopesEnd)
        GrowScopeStack();
}

PIKA_FORCE_INLINE void Context::PopCallScope()
{
    if (scopesTop == scopesBeg)
        ReportRuntimeError(Exception::ERROR_system, "Scope stack underflow.");
        
    --scopesTop;
    ScopeInfo& currA = *scopesTop;
    
    ASSERT(currA.kind == SCOPE_call);
    
    env = currA.env;
    pc = currA.pc;
    sp = stack + currA.stackTop;
    bsp = stack + currA.stackBase;
    self = currA.self;
    closure = currA.closure;
    newCall = currA.newCall;
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
    
    currA.env     = 0;
    currA.closure = 0;
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

Enumerator* Context::GetEnumerator(String*)
{
    ContextEnum* c;
    PIKA_NEW(ContextEnum, c, (engine, this));
    engine->AddToGC(c);
    return c;
}

Context::Context(Engine* eng, Type* obj_type)
        : ThisSuper(eng, obj_type),
        state(INVALID),
        prev(0),
        stack(0),
        pc(0),
        sp(0),
        bsp(0),
        esp(0),
        closure(0),
        package(0),
        env(0),
        newCall(false),
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

void Context::MakeInvalid()
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
    
    callsCount      = 1;
    closure         = 0;
    package         = 0;
    env             = 0;
    currLiterals    = 0;
    state           = INVALID;
    prev            = 0;
    pc              = 0;
    newCall         = false;
    argCount        = 0;
    retCount        = 1;
    numTailCalls    = 0;
    numRuns         = 0;
    nativeCallDepth = 0;
    self.SetNull();
//  members.Clear(); // TODO: Should we clean out this Context's members ?
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
    //
    // TODO { What about closures formed before grow stack and then returned
    //        from the parent's function afterwards. We need to test this
    //        situtation. }
    
    for (ScopeIter currap = scopesBeg; currap < scopesTop; ++currap)
    {
        Function* fun = (*currap).closure;
        
        if (!fun)
            continue;
            
        Def* def = fun->def;
        
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
        env->Set(bsp, env->length);
}

void Context::DoSuspend(Value* v, size_t amt)
{
    if (IsRunning()) state = SUSPENDED;
    
    if (prev)
    {
        // If someone is expecting the values we produced.
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

INLINE void Context::CompOpBinary(const Opcode op, const OpOverride ovr, const OpOverride ovr_r, int& numcalls)
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
    
    ReportRuntimeError(Exception::ERROR_runtime,
                       "Operator %s not defined between types %s %s.",
                       engine->GetOverrideString(ovr)->GetBuffer(),
                       engine->GetTypenameOf(a)->GetBuffer(),
                       engine->GetTypenameOf(b)->GetBuffer());
                       
}

INLINE void Context::ArithOpUnary(const Opcode op, const OpOverride ovr, int& numcalls)
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
        }
    }
    else if (a.tag == TAG_object)
    {
        Object* obj = a.val.object;
        if (SetupOverrideUnary(obj, ovr))
            ++numcalls;
    }
    else
    {
        ReportRuntimeError(Exception::ERROR_runtime,
                           "Operator %s not defined for object of type %s.",
                           engine->GetOverrideString(ovr)->GetBuffer(),
                           engine->GetTypenameOf(a)->GetBuffer());
    }
}

INLINE void Context::BitOpBinary(const Opcode op, const OpOverride ovr, const OpOverride ovr_r, int& numcalls)
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
        }
        Pop();
        return;
    }
    
    // Check for override from the lhs operand
    if (a.tag == TAG_object)
    {
        Object* obj = a.val.object;
        bool    res = false;
        
        if (SetupOverrideLhs(obj, ovr, &res))
            ++numcalls;
        if (res)
            return;
    }
    
    // Check for override from the rhs operand
    if (b.tag == TAG_object)
    {
        Object* obj = b.val.object;
        bool    res = false;
        
        if (SetupOverrideRhs(obj, ovr_r, &res))
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

INLINE void Context::ArithOpBinary(const Opcode op, const OpOverride ovr, const OpOverride ovr_r, int& numcalls)
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
            case OP_pow:  pow_num(a.val.integer, b.val.integer); break;
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
                a.Set(RealToInteger(a.val.real)); // TODO: convert integer to real
                break;
            case OP_mod:  mod_num(a.val.real, b.val.real); break;
            case OP_pow:  pow_num(a.val.real, b.val.real); break;
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
                a.Set(RealToInteger(a.val.real));
                break;
            case OP_mod: mod_num(a.val.real, b.val.real); break;
            case OP_pow: pow_num(a.val.real, b.val.real); break;
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
                a.Set(RealToInteger(a.val.real));
                break;
            case OP_mod: mod_num(a.val.real, b.val.real); break;
            case OP_pow: pow_num(a.val.real, b.val.real); break;
            }
            Pop();
            return;
        }
        }
    }
    break; // TAG_real
    
    case TAG_string:
    {
        if (b.tag == TAG_integer && op == OP_mul)
        {
            pint_t i = b.val.integer;
            String* str = a.val.str;
            String* res = String::Multiply(str, i);
            a.Set(res);
            Pop();
            return;
        }
    }
    break;
    case TAG_object:
    {
        Object* obj = a.val.object;
        bool res = false;
        if (SetupOverrideLhs(obj, ovr, &res))
        {
            ++numcalls;
        }
        if (res) return;
    }
    break;
    }
    
    if (b.tag == TAG_object)
    {
        Object* obj = b.val.object;
        bool res = false;
        if (SetupOverrideRhs(obj, ovr_r, &res))
        {
            ++numcalls;
        }
        if (res) return;
    }
    
    // We do not reach this point unless the operation is undefined between the
    // lhs and rhs operands.
    
    ReportRuntimeError(Exception::ERROR_runtime,
                       "Operator %s not defined between objects of type %s and %s.",
                       engine->GetOverrideString(ovr)->GetBuffer(),
                       engine->GetTypenameOf(a)->GetBuffer(),
                       engine->GetTypenameOf(b)->GetBuffer());
}

int Context::AdjustArgs(Function* fun, Def* def, int param_count, u4 argc, int argdiff, bool nativecall)
{
    int resultArgc = argc;
    // For strict native methods this will raise an exception.
#if defined(PIKA_STRICT_ARGC)
    if (true)
#else
    if (def->isStrict && nativecall)
#endif
    {
        if (argdiff > 0)
        {
            String* dotname = fun->GetLocation()->GetDotName();
            RaiseException(Exception::ERROR_runtime,
                           "Too many arguments for function '%s.%s'. Expected  %d %s but was given %d.",
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
    
    if (argdiff > 0)
    {
        // Too many arguments.
        // ------------------
        
        if (!def->isVarArg)
        {
            // Not variadic so we pop off the last 'argdiff' arguments.
            Pop(argdiff);
            resultArgc = param_count;
        }
        else if (!nativecall)
        {
            GCPAUSE_NORUN(engine);
            // Bytecode variadic function:
            // Create the varargs Array. We only do this for byte-code
            // functions since native functions have direct access to the stack.
            
            Array *v = Array::Create(engine, 0, argdiff + 1, (GetStackPtr() - argdiff - 1));
            Pop(argdiff);
            Top().Set(v);
            resultArgc = param_count;
        }
        // Otherwise the function is a variadic native function.
        // We just keep the arguments given to us.
    }
    else
    {
        // Too few arguments.
        // ------------------
        
        // We want to push null for all arguments unspecified and
        // without a default argument.
        //
        // Then push the default values for all argument with default
        // values which were unspecified.
        //
        // Lastly we want to push an empty vector if it is a bytecode
        // variadic function.
        
        // Bytecode function with a varags parameter.
        
        bool adjustForVarArgs = def->isVarArg && !nativecall;
        int  argstart         = argc;
        
        // Number of arguments for which no default value was specified.
        int numRegularArgs = (adjustForVarArgs) ?
                             (param_count - (int)fun->numDefaults - 1)  : // Remove the vararg parameter from the regular arg count.
                             (param_count - (int)fun->numDefaults);
                             
        int const amttopush = numRegularArgs - argstart;
        int const defstart  = Clamp<int>(-amttopush, 0, (int)fun->numDefaults);
        
        // Arguments with no default value will be set to null.
        for (int p = 0; p < amttopush; ++p)
        {
            PushNull();
        }
        
        // Arguments with a default value will be set to their default value.
        for (u4 a = defstart; a < fun->numDefaults; ++a)
        {
            Push(fun->defaults[a]);
        }
        
        // Create an empty variable arguments Array if needed.
        if (adjustForVarArgs)
        {
            GCPAUSE_NORUN(engine);
            
            Array* v = Array::Create(engine, engine->Array_Type, 0, 0);
            Push(v);
        }
        resultArgc = param_count;
    }
    
    return resultArgc;
}

bool Context::SetupCall(u2 argc, bool tailcall, u2 retc)
{
    //  [ previous calls... ]
    //  [ argument 0        ]
    //  [ ...               ]
    //  [ argument argc-1   ]
    //  [ self or null      ]
    //  [ function          ] < sp
    
    Value frameVar = PopTop();
    Value selfVar  = PopTop();
    
    //  [ ...   ]
    //  [ arg 0 ]
    //  [ ...   ]
    //  [ arg N ] < sp
    
    // frame variable is a derived from type Function.
    
    if (engine->Function_Type->IsInstance(frameVar))
    {
        Function* fun = frameVar.val.function;
        Def* def = fun->def;
        
        // Adjust argc to match the definition's param_count.
        int  param_count = def->numArgs;
        int  argdiff     = (int)argc - param_count;
        bool nativecall  = (def->nativecode != 0);
        
        // Incorrect argument count.
        if (argdiff != 0)
        {
            argc = AdjustArgs(fun, def, param_count, argc, argdiff, nativecall);
        }
        else if (def->isVarArg && !nativecall && param_count)
        {
            GCPAUSE_NORUN(engine);
            
            Value* singlearg = GetStackPtr() - 1;
            Array* v = Array::Create(engine, 0, 1, singlearg);
            Top().Set(v);
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
        
        bsp       = argv;
        self      = selfVar;
        pc        = def->GetBytecode();
        closure   = fun;
        package   = fun->GetLocation();
        newCall   = false;
        argCount  = argc;
        acc.SetNull();
        ASSERT(closure);
        ASSERT(package);
        
        //  [ ...          ]
        //  [ argument 0   ] < bsp
        //  [ ...          ]
        //  [ argument N   ]
        //  [ local 0      ]
        //  [ ...          ]
        //  [ local N      ]
        //  [ return value ] < rsp
        //  [              ] < sp
        
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
            int expret = retCount;
            
            --nativeCallDepth;
            
            Value* returnPtr = GetStackPtr();
            
            ASSERT((returnPtr - numret) > (bsp + argCount));
            
            PopCallScope(); // Restore the previous scope
            
            if (numret >= expret)
            {
                // More than or the same # of return values
                
                // Push the last <expret> values that were returned.
                Value* startPtr = returnPtr - numret;
                Value* endPtr = startPtr + expret;
                for (Value* curr = startPtr; curr != endPtr; ++curr)
                {
                    Push(*curr);
                }
            }
            else
            {
                // Less than expected # of return values
                
                // Push all return values given.
                Value* lastPtr = returnPtr - numret;
                for (Value* curr = lastPtr; curr < returnPtr; ++curr)
                {
                    Push(*curr);
                }
                // Pad with nulls.
                int retdiff = expret - numret;
                for (int i = 0; i < retdiff; ++i)
                {
                    PushNull();
                }
            }
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
                env = LexicalEnv::Create(engine, closure->lexEnv, true); // ... create the lexical environment (ie the args + locals).
                env->Set(bsp, def->numLocals);                           // ... for now they point to our stack.
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
            return true;
        }
    }
    else
    {
        // object or userdata that may contain a call override method.
        if (frameVar.tag == TAG_object || frameVar.tag == TAG_userdata)
        {
            Push(frameVar);
            return SetupOverride(argc, frameVar.val.basic, OVR_call);
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

bool Context::SetupOverride(u2 argc, Basic* obj, OpOverride ovr, bool* res)
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
        return SetupCall(argc);
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

/** Finds the super method of the method currently being executed.
  * @note   Result is returned on the stack.
  */
void Context::OpSuper()
{
    Function* prop     = closure;
    String*   methname = prop->GetDef()->name;
    bool      found    = false;
    
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
        PushNull();
    }
}

/** Make this Context the active one. */
void Context::Activate()   { engine->ChangeContext(this); }

/** Remove the current active Context. */
void Context::Deactivate() { engine->ChangeContext(0); }

/** Raises a formatted engine exception. If a script is being executed the
  * line and function are reported, otherwise a generic exception is raised.
  *
  * @param msg          [in] printf style message to use in the exception.
  */
void Context::ReportRuntimeError(Exception::Kind kind, const char* msg, ...)
{
    static const size_t BUFSZ = 1024;
    char buffer[BUFSZ + 1];
    
    va_list args;
    va_start(args, msg);
    
    Pika_vsnprintf(buffer, BUFSZ, msg, args);
    buffer[BUFSZ] = '\0';
    
    va_end(args);
    
    if (!closure)
    {
        RaiseException(kind, buffer);
    }
    else
    {
        String* name = closure->GetDotPath();
        if (!pc)
        {
            RaiseException(kind,
                           "In function '%s': \"%s\"",
                           name->GetBuffer(),
                           buffer);
        }
        else
        {
            int line = closure->DetermineLineNumber(pc);
            
            RaiseException(kind,
                           "At line %d in function '%s': \"%s\"",
                           line,
                           name->GetBuffer(),
                           buffer);
        }
    }
}

/** Sets an object's slot.
  *
  * @param numcalls         [in|out] Reference to the integer that holds the number of inlined calls made.
  * @param oc               [in]     Current opcode.
  */
void Context::OpDotSet(int& numcalls, Opcode oc)
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
        
        if (oc != OP_setglobal && obj.tag == TAG_object)
        {
            Value setfn;
            setfn.SetNull();
            
            if (GetOverrideFrom(engine, obj.val.object, OVR_set, setfn))
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
        }
        
        if (!obj.val.basic->SetSlot(prop, val))
        {
            // If we couldn't write it might be because prop is a property.
            
            Value res;
            if (obj.val.basic->GetSlot(prop, res) && res.tag == TAG_property)
            {
                // res is a Property; So we auto-magically call it.
                
                Property* pprop = res.val.property;
                
                // IF the Property supports writing.
                if (pprop && pprop->CanSet())
                {
                    //  [ ....  ]
                    //  [ value ]
                    //  [ obj   ]
                    //  [ prop  ]< Top
                    
                    prop.Set(pprop->GetSetter());
                    
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
                                       "Attempt to set property '%s' for '%s'. Property does not support writing.",
                                       engine->ToString(this, prop)->GetBuffer(),
                                       engine->ToString(this, obj)->GetBuffer());
                    return;
                }
            }
            else
            {
                //  Member cannot be written.
                ReportRuntimeError(Exception::ERROR_runtime,
                                   "Attempt to set member '%s' for '%s'.",
                                   engine->ToString(this, prop)->GetBuffer(),
                                   engine->ToString(this, obj)->GetBuffer());
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
                           "Attempt to set member '%s' for '%s'.",
                           engine->ToString(this, prop)->GetBuffer(),
                           engine->ToString(this, obj)->GetBuffer());
        return;
    }
}


// Reads a member variable from an object. Calls any overrides or properties
// as needed. Depending on the object in question, a missing member may
// cause an exception.

void Context::OpDotGet(int& numcalls, Opcode oc)
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
                               engine->ToString(this, prop)->GetBuffer(),
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
                if (property && property->CanGet())
                {
                    //  [ .... ]
                    //  [ obj  ]
                    //  [ prop ]< Top
                    
                    Pop();
                    
                    //  [ .... ]
                    //  [ obj  ]< Top
                    
                    Push(property->GetGetter());
                    
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
            else if (oc == OP_pushglobal)
            {
#   if defined( PIKA_ALLOW_MISSING_GLOBALS )
                res.SetNull();
#   else
                ReportRuntimeError(Exception::ERROR_runtime,
                                   "Attempt to read global variable '%s'.",
                                   engine->ToString(this, prop)->GetBuffer());
                return;
#   endif
            }
            else
            {
#   if defined( PIKA_ALLOW_MISSING_SLOTS )
                res.SetNull();
#   else
                ReportRuntimeError(Exception::ERROR_runtime,
                                   "Attempt to read member '%s' of '%s'.",
                                   engine->ToString(this, prop)->GetBuffer(),
                                   engine->ToString(this, obj)->GetBuffer());
                return;
#   endif
            }
        }
    }
    else
    {
        // Object, Property, Enumerator, String
        
        // Call the opGet override method if present.
        Basic* basic = obj.val.basic;
        if (!basic->GetSlot(prop, res))
        {
            if (oc == OP_pushglobal)
            {
#   if PIKA_GLOBALS_MUST_EXIST
                res.SetNull();
#   else
                ReportRuntimeError(Exception::ERROR_runtime,
                                   "Attempt to read missing global variable '%s'.",
                                   engine->ToString(this, prop)->GetBuffer());
                return;
#   endif
            }
            else
            {
                if (obj.tag == TAG_object)
                {
                    Value getfn;
                    getfn.SetNull();
                    
                    if (GetOverrideFrom(engine, obj.val.object, OVR_get, getfn))
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
                                   engine->ToString(this, prop)->GetBuffer(),
                                   engine->ToString(this, obj)->GetBuffer());
#   endif
                return;
            }
        }
    }
    
    if (res.tag == TAG_property)
    {
        // The result is a Property
        // So we need to call its accessor function.
        
        Property* property = res.val.property;
        
        // If the Property supports reading
        if (property && property->CanGet())
        {
            //  [ .... ]
            //  [ obj  ]
            //  [ prop ]< Top
            
            Pop();
            
            //  [ .... ]
            //  [ obj  ]< Top
            
            Push(property->GetGetter());
            
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
                               property->GetName()->GetBuffer(),
                               engine->ToString(this, obj)->GetBuffer());
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
    if (!prop->CanGet())
        return false;
    
    Push(prop->GetGetter());
            
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

bool Context::DoPropertySet(int& numcalls, Property* prop)
{
    if (!prop->CanSet())
        return false;

    //  [ ....  ]
    //  [ value ]
    //  [ obj   ]
    //          < Top
    
    Push(prop->GetSetter());
                    
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

bool Context::OpUnpack(u2 expected)
{
    Value& t = Top();
    
    switch (t.tag)
    {
    case TAG_null:
    case TAG_boolean:
    case TAG_integer:
    case TAG_real:
    case TAG_string:
    {
        Pop();
        Value* start = GetStackPtr();
        StackAlloc(expected);
        for (Value* c = start; c < (start + expected); ++c)
        {
            *c = t;
        }
    }
    break;
    
    case TAG_userdata:
    case TAG_object:
    {
        Basic* obj = t.val.basic;
        Value meth(NULL_VALUE);
        
        if (GetOverrideFrom(engine, obj, OVR_unpack, meth))
        {
            Push(meth);
            return SetupCall(0, false, expected);
        }
        else if (t.IsDerivedFrom(Array::StaticGetClass()))
        {
            Pop();
            Array* v = (Array*)t.val.object;
            Value* start = GetStackPtr();
            StackAlloc(expected);
            
            if (expected <= v->GetLength())
            {
                Pika_memcpy(start, v->GetAt(0), expected * sizeof(Value));
            }
            else
            {
                Pika_memcpy(start, v->GetAt(0), expected * sizeof(Value));
                
                for (Value* c = (start + v->GetLength()); c < (start + expected); ++c)
                {
                    c->SetNull();
                }
            }
            return false;
        }
    }
    // Fall Through
    //      |
    //      V
    default:
    {
        ReportRuntimeError(Exception::ERROR_runtime,
                           "Attempt to unpack %u elements from object of type '%s'.",
                           expected,
                           engine->GetTypenameOf(t)->GetBuffer());
        return false;
    }
    }
    return false;
}// OpUnpack

bool Context::OpBind()
{
    Value& b = Top();
    Value& a = Top1();
    
    if (a.IsObject())
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
            Object* obj = a.val.object;
            OpOverride ovr = OVR_bind;
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
    }
    else if (b.IsObject())
    {
        Object* obj = b.val.object;
        OpOverride ovr = OVR_bind_r;
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
    ReportRuntimeError(Exception::ERROR_runtime,
                       "Unsupported operands for bind operator: '%s' and '%s'.",
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
    
    while (--n)
    { /* must use prefix operator-- */
        ASSERT(curr);
        curr = curr->parent;
    }
    ASSERT(curr);
    
#if defined (PIKA_CHECK_LEX_ENV)
    if ( !curr || !curr->lexEnv )
    {
        /* Shouldn't happen under properly compiled script. */
        RaiseException("Attempt to access invalid bound variable of depth %d and index of %d.",
                       depth,
                       idx);
    }
    else if (curr->lexEnv->length <= idx)
    {
        RaiseException("Attempt to access invalid bound variable of index %d at depth %d.",
                       depth,
                       idx);
    }
#endif
    return curr->lexEnv->values[idx];
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
    if (fun->name == 0 || fun->name == engine->emptyString)
    {
        f = Function::Create(engine, fun, package, closure);
    }
    else if (mself && (mself->IsDerivedFrom(Type::StaticGetClass()) ||
                       (mself->IsNull() && self.IsDerivedFrom(Type::StaticGetClass()))))
    {
        f = ClassMethod::Create(engine, closure, fun, package, mself->IsNull() ? self.val.type : mself->val.type);
    }
    else if (package->IsDerivedFrom(Type::StaticGetClass()))
    {
        f = InstanceMethod::Create(engine, closure, fun, package, (Type*)package);
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
    return Context::Create(engine, GetType());
}

void Context::Init(Context* ctx)
{
    if (ctx->GetArgCount() != 1)
        RaiseException("Context.init requires exactly 1 argument.");
    if (state != INVALID)
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
    
    //  Our Stack
    //  ---------
    //  [ ...      ]
    //  [ function ]
    //              < sp
    //
    //  The function we are using as an entry-point should be pushed onto the stack.
    
    //if (sp <= bsp + 1) {
    if (sp <= bsp)
    {
        RaiseException("Context not initialized. Please pass the function to execute to the context's constructor.");
        return;
    }
    
    if (IsInvalid())
    {
        prev = ctx;
        
        Value fn = PopTop();
        //Value selfobj = PopTop();
        
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
        
        //Push(selfobj); // push self
        PushNull();
        Push(fn);      // re-push function
        
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
        /*else
        {
            // TODO: we have already called the function. we need to check before setup call.
            prev->ReportRuntimeError(Exception::ERROR_runtime,
                                     "cannot create a context coroutine from a native function.");
        }*/
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
//    Value v = ctx->GetArg(0);
//    ctx->DoSuspend(v);
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
    
    for (;a != scopesTop; ++a)
    {
        a->DoMark(c);
        
        // TODO: Would it be faster to just mark the 'ENTIRE' stack from top to bottom.
        //       What I mean is there ever a gap between scope stack ranges?
        
        if (a->kind == SCOPE_call)
        {
            MarkValues(c, stack + a->stackBase, stack + a->stackTop);
        }
    }
    
    // Mark exception handlers.
    
    for (;e != handlers.End(); ++e)
    {
        e->DoMark(c);
    }
    
    // Mark current scope.
    
    if (env) env->Mark(c);
    if (closure) closure->Mark(c);
    if (package) package->Mark(c);
    
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

void Context::CheckParamCount(u2 amt)
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
        if (v.tag == TAG_real && NumberIsIntegral(v.val.real))
        {
            return RealToInteger(v.val.real);
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

Enumerator* Context::GetEnumArg(u2 arg)
{
    Value& v = GetArg(arg);
    
    if (v.tag != TAG_enumerator)
    {
        ArgumentTagError(arg, (ValueTag)v.tag, TAG_enumerator);
    }
    return v.val.enumerator;
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
        CheckParamCount(count);
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
        /* ================ Enumerator ================ */
        case 'E':
        case 'e':
        {
            if (currVar->tag != TAG_enumerator)
            {
                u2 argpos = (u2)(currVar - bsp);
                ArgumentTagError(argpos, (ValueTag)currVar->tag, TAG_enumerator);
            }
            
            Enumerator** enumpp = va_arg(va, Enumerator**);
            *enumpp = currVar->val.enumerator;
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
        CheckParamCount(count);
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
        
        /* Enumerator */        
        case 'E':
        case 'e':
        {
            if (currVar->tag != TAG_enumerator)
            {
                u2 argpos = (u2)(currVar - bsp);
                ArgumentTagError(argpos, (ValueTag)currVar->tag, TAG_enumerator);
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

Context::EErrRes Context::OpException(Exception& e)
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
            // TODO: Error type hierarchy.
            switch (e.kind)
            {
            case Exception::ERROR_syntax:     engine->SyntaxError_Type->CreateInstance(thrown);     break;
            case Exception::ERROR_runtime:    engine->RuntimeError_Type->CreateInstance(thrown);    break;
            case Exception::ERROR_overflow:   engine->OverflowError_Type->CreateInstance(thrown);   break;
            case Exception::ERROR_arithmetic: engine->ArithmeticError_Type->CreateInstance(thrown); break;
            case Exception::ERROR_index:      engine->IndexError_Type->CreateInstance(thrown);      break;
            case Exception::ERROR_type:       engine->TypeError_Type->CreateInstance(thrown);       break;
            case Exception::ERROR_system:     engine->SystemError_Type->CreateInstance(thrown);     break;
            case Exception::ERROR_assert:     engine->AssertError_Type->CreateInstance(thrown);     break;
            default:                          engine->Error_Type->CreateInstance(thrown);
            }
            
            const char* const msg = e.GetMessage();
            String* const errorStr = msg ? engine->AllocString(msg) : engine->emptyString;
            
            if (thrown.IsObject() && thrown.val.object)
            {
                Object* error_obj = thrown.val.object;
                error_obj->SetSlot(engine->message_String, errorStr);
                error_obj->SetSlot(engine->AllocString("name"), engine->AllocString(Exception::Static_Error_Formats[e.kind]));
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
                        --numRuns;
                        return ER_throw;
                    }
                    else if (scopes[a].env)
                    {
                        scopes[a].env->EndCall();
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
        
        state = INVALID;
        Deactivate();
        --numRuns;
        
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
                std::cerr << Exception::Static_Error_Formats[e.kind] << ": " << msg << std::endl;
            }
            else
            {
                std::cerr << "*** Unhandled Exception ***" << std::endl;
            }
        }
        return ER_exit;
    }
}

}// pika
