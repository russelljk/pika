/*
 * PGenerator.cpp
 * See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PContext.h"
#include "PGenerator.h"

namespace pika {

// Enumerates each yielded value for the given Context.
class GeneratorEnum : public Enumerator
{
public:
    GeneratorEnum(Engine* eng, Context* ctx, Generator* gen)
        : Enumerator(eng), val(NULL_VALUE), context(ctx), generator(gen)
    {}
    
    virtual ~GeneratorEnum()
    {}
    
    virtual bool Rewind()
    {
        val.SetNull();
        Advance();
        return IsValid();
    }
    
    virtual bool IsValid()
    {
        return generator->ToBoolean();
    }
    
    virtual void GetCurrent(Value& curr)
    {
        curr = val;
    }
    
    virtual void Advance()
    {
        if (IsValid())
        {
            size_t const base = context->GetStackSize();
            generator->Resume(context, 1);
            context->Run();
            val = context->PopTop();
            size_t amt = context->GetStackSize();
            if (base < amt)
            {
                amt -= base;
                context->Pop(amt);
            }
        }
    }
private:
    Value val;
    Context* context;
    Generator* generator;
};

PIKA_IMPL(Generator)

Generator::Generator(Engine* eng, Type* typ, Function* fn) : 
    ThisSuper(eng, typ),
    state(GS_clean),
    function(fn)
{}

Generator::~Generator() {}

void Generator::Init(Context* ctx)
{
    Function* f = ctx->GetArgT<Function>(0);
    WriteBarrier(f);
    this->function = f;
}

void Generator::MarkRefs(Collector* c)
{
    ThisSuper::MarkRefs(c);        
    if (function)
        function->Mark(c);            
    MarkValues(c, stack.BeginPointer(),
                  stack.EndPointer());
    for (ScopeIter s = scopes.Begin(); s != scopes.End(); ++s)
    {
        s->DoMark(c);
    }
    
    for (HandlerIter h = handlers.Begin(); h != handlers.End(); ++h)
    {
        h->DoMark(c);
    }
}

bool Generator::ToBoolean()
{
    return state == GS_yielded;
}

bool Generator::IsYielded()
{
    return state == GS_yielded;
}

void Generator::Return()
{
    state = GS_finished;
}

void Generator::Yield(Context* ctx)
{
    if (state == GS_yielded || state == GS_finished)
        RaiseException("Attempt to yield a generator already %s.", (state==GS_yielded) ? "yielded" : "returned");
    
    // Instead of a performing a multitude of WriteBarriers we will 
    // just move this object to the gray list.
    
    engine->GetGC()->MoveToGray(this);
    
    // Have the context push this scope
    
    ctx->PushCallScope();
    
    // Get our ScopeInfo.
    
    ScopeIter callscope = ctx->GetScopeTop() - 1;
    
    // The scope id is needed for exception handlers.
    
    size_t const scopeid = ctx->scopes.IndexOf(callscope);
    
    // Copy stack
    
    size_t const stack_size = callscope->stackTop - callscope->stackBase - 1;
    stack.Resize(stack_size);
    Pika_memcpy(stack.GetAt(0), ctx->stack + callscope->stackBase, sizeof(Value) * stack_size);
    
    // Move lexical environment over to our stack.
    
    if (ctx->env)
        ctx->env->Set(stack.GetAt(0), stack.GetSize());
        
    // Copy Exception Handlers
    
    size_t num_handlers = FindBaseHandler(ctx, scopeid);
    if (num_handlers) {
        size_t start_handler = ctx->handlers.GetSize() - num_handlers;
        handlers.Resize(num_handlers);
        Pika_memcpy( handlers.GetAt(0),
                     ctx->handlers.GetAt(start_handler), 
                     sizeof(ExceptionBlock) * num_handlers );
        ctx->handlers.Pop(num_handlers);
    }
    
    // Copy all scopes up to the current then pop'em off the context's scope-stack.
    // XXX: In the future make sure that the original stack is not destroyed since
    //      the values have not been returned yet.
    
    size_t idx = FindLastCallScope(ctx, callscope);
    size_t amt = scopeid - idx;
    scopes.Resize(amt);
    Pika_memcpy(scopes.GetAt(0), ctx->scopes.GetAt(idx + 1), amt * sizeof(ScopeInfo));
    
    // Now transition into the caller's sope.
    
    ctx->scopesTop -= amt;
    ctx->PopCallScope();
    
    state = GS_yielded;
}

void Generator::Constructor(Engine* eng, Type* type, Value& res)
{
    Generator* g = Create(eng, type, eng->null_Function);
    res.Set(g);
}

Generator* Generator::Create(Engine* eng, Type* type, Function* function)
{
    Generator* gen = 0;
    GCNEW(eng, Generator, gen, (eng, type, function));
    return gen;
}

void Generator::Resume(Context* ctx, u4 retc)
{
    if (state != GS_yielded) {
        RaiseException("Attempt to call generator that is not yieled.");
    }
    // Need to push before copying the stack.
    
    ctx->PushCallScope();
    
    // Copy the scopes. Generally there is a single scope, however, 
    // other language constructs have their own scopes.
    
    size_t base = ctx->sp - ctx->stack;
    for (size_t i = 0; i < scopes.GetSize(); ++i)
    {
        (*ctx->scopesTop) = scopes[i];
        if (++(ctx->scopesTop) >= ctx->scopesEnd)
            ctx->GrowScopeStack();
    }
    
    ctx->PopCallScope();
    
    // Copy handlers.
    
    if (handlers.GetSize()) {
        size_t const scopeid = ctx->scopes.IndexOf(ctx->scopesTop);
        size_t j = ctx->handlers.GetSize();
        ctx->handlers.Resize(j + handlers.GetSize());
        for (size_t i = 0; i < handlers.GetSize(); ++i, ++j)
        {
            handlers[i].scope = scopeid;
            ctx->handlers[j] = handlers[i];
        }
    }
    
    // Copy stack.
    
    ctx->bsp = ctx->stack + base;
    ctx->sp = ctx->bsp + 1;
    
    ctx->StackAlloc( stack.GetSize() );
    
    Pika_memcpy( ctx->bsp,
                 stack.GetAt(0),
                 stack.GetSize() * sizeof(Value) );
    
    // Restore the lexical environment.
    
    Value* bsp = ctx->GetBasePtr();
    Value*  sp = ctx->GetStackPtr();
    
    if (ctx->env)
        ctx->env->Set(bsp, sp - bsp);
    ctx->retCount = retc;
    state = GS_resumed;
}

size_t Generator::FindBaseHandler(Context* ctx, size_t scopeid)
{
    size_t curr = ctx->handlers.GetSize();
    size_t amt = 0;
    
    while (curr > 0 && ctx->handlers[--curr].scope == scopeid)
    {
        ++amt;
    }
    
    return amt;
}

size_t Generator::FindLastCallScope(Context* ctx, ScopeIter iter)
{
    // Not every scope is a call scope.
    // We need to go back and find the caller.    
    iter--;
    while (iter->kind != SCOPE_call)
    {
        iter--;
    }
    return ctx->scopes.IndexOf(iter);
}

Enumerator* Generator::GetEnumerator(String*)
{
    Context* ctx = engine->GetActiveContextSafe();
    GeneratorEnum* ge = 0;
    GCNEW(engine, GeneratorEnum, ge, (engine, ctx, this));
    return ge;
}

namespace {

int Generator_state(Context* ctx, Value& self)
{
    Generator* gen = static_cast<Generator*>(self.val.object);
    ctx->Push((pint_t)gen->GetState());
    return 1;
}

}

void Generator::StaticInitType(Engine* eng)
{
    Package* Pkg_World = eng->GetWorld();
    String* Generator_String = eng->AllocString("Generator");
    eng->Generator_Type = Type::Create(eng, Generator_String, eng->Object_Type, Generator::Constructor, Pkg_World);
    Pkg_World->SetSlot(Generator_String, eng->Generator_Type);
    
    SlotBinder<Generator>(eng, eng->Generator_Type, Pkg_World)
    .Method(&Generator::ToBoolean,  "toBoolean")
    .Constant((pint_t)GS_clean,     "CLEAN")
    .Constant((pint_t)GS_yielded,   "YIELDED")
    .Constant((pint_t)GS_resumed,   "RESUMED")
    .Constant((pint_t)GS_finished,  "FINISHED")
    ;
    
    static RegisterProperty Generator_Properties[] =
    {
        { "state", Generator_state, "getState", 0, 0 }
    };
    
    eng->Generator_Type->EnterProperties(Generator_Properties, countof(Generator_Properties));
}

}// pika
