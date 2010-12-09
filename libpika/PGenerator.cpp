/*
 * PGenerator.cpp
 * See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PContext.h"
#include "PGenerator.h"

namespace pika {

PIKA_IMPL(Generator)

Generator::Generator(Engine* eng, Type* typ, Function* fn) : 
    ThisSuper(eng, typ),
    scopetop(0),
    state(GS_clean),
    function(fn)
{}

Generator::Generator(Generator* rhs) :
    ThisSuper(rhs),
    scopetop(rhs->scopetop),
    state(rhs->state),
    function(rhs->function),
    handlers(rhs->handlers),
    scopes(rhs->scopes),
    stack(rhs->stack)
{
    GCPAUSE_NORUN(engine);
    for (ScopeIter siter = scopes.Begin(); siter != scopes.End(); ++siter)
    {
        if (siter->kind == SCOPE_call)
        {
            if (siter->generator == rhs)
            {
                siter->generator = this;
            }
            if (siter->closure == function && siter->env)
            {
                LexicalEnv* oldenv = siter->env;
                siter->env = LexicalEnv::Create(engine, oldenv);
            }
        }
    }
}

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

PIKA_DOC(Generator_toBoolean, "Returns whether or not the generator is yielded and can be resumed by calling [next].")

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

Object* Generator::Clone()
{
    Generator* gen = 0;
    PIKA_NEW(Generator, gen, (this));
    engine->AddToGCNoRun(gen);
    return gen;
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
    scopetop = scopeid;
    // Copy stack
    
    size_t const stack_size = callscope->stackTop - callscope->stackBase - 1;
    stack.Resize(stack_size);
    Pika_memcpy(stack.GetAt(0), ctx->stack + callscope->stackBase, sizeof(Value) * stack_size);
    
    // Move lexical environment over to our stack.
    
    if (ctx->env)
        ctx->env->Set(stack.GetAt(0), stack.GetSize());
    
    // Copy all scopes up to the current then pop'em off the context's scope-stack.
    // XXX: In the future make sure that the original stack is not destroyed since
    //      the values have not been returned yet.
    
    size_t idx = FindLastCallScope(ctx, callscope);
    size_t amt = scopeid - idx;
    scopes.Resize(amt);
    Pika_memcpy(scopes.GetAt(0), ctx->scopes.GetAt(idx + 1), amt * sizeof(ScopeInfo));
    
    // Copy Exception Handlers
    
    size_t num_handlers = FindBaseHandler(ctx, idx+1);
    if (num_handlers) {
        size_t start_handler = ctx->handlers.GetSize() - num_handlers;
        handlers.Resize(num_handlers);
        Pika_memcpy( handlers.GetAt(0),
                     ctx->handlers.GetAt(start_handler), 
                     sizeof(ExceptionBlock) * num_handlers );
        ctx->handlers.Pop(num_handlers);
    }
    
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
    PIKA_NEW(Generator, gen, (eng, type, function));
    eng->AddToGCNoRun(gen);
    return gen;
}

void Generator::Resume(Context* ctx, u4 retc, bool tailcall)
{
    if (state != GS_yielded) {
        RaiseException("Attempt to call generator that is not yieled.");
    }
    
    // First we need to deal with the current call.
    
    if (tailcall)
    {
        // If we are resumed via a tail call.
        // We need to rewind the stack before we can restore ours.
        ctx->sp = ctx->bsp;
    }
    else
    {
        // Else its a normal function call.
        // Save the current scope.
        ctx->PushCallScope();
    }
    
    // Save the size of the stack. This is where our bsp will start.
    
    size_t const base = ctx->sp - ctx->stack;
        
    // Copy the scopes. 
    // Generally there is a single scope, however, the yield could have happened
    // inside a class, package or using statement.
    
    for (size_t i = 0; i < scopes.GetSize(); ++i)
    {
        (*ctx->scopesTop) = scopes[i];
        if (++(ctx->scopesTop) >= ctx->scopesEnd)
            ctx->GrowScopeStack();
    }
    
    // Now restore our scope.
    
    ctx->PopCallScope();
    
    // Copy exceptions handlers.
    
    if (handlers.GetSize())
    {
    
        size_t const scopeid = ctx->scopes.IndexOf(ctx->scopesTop);
        size_t j = ctx->handlers.GetSize();
        ctx->handlers.Resize(j + handlers.GetSize());
        for (size_t i = 0; i < handlers.GetSize(); ++i, ++j)
        {
            ASSERT(scopetop >= handlers[i].scope);
            
            // Find the scopeid relative to the scopesTop index at yield time.
            // This is important because additional handlers might have been
            // added between the time we yielded and now. 
            // offset will be <= 0
            ptrdiff_t offset = handlers[i].scope - scopetop;
            
            // Set the scope id and add (subtract actually) the offset.
            handlers[i].scope = scopeid + offset;
            ctx->handlers[j] = handlers[i];
        }
    }
    
    // Copy stack.
    
    ctx->bsp = ctx->stack + base;
    ctx->sp = ctx->bsp + 1;
    
    ctx->StackAlloc( stack.GetSize() );
    
    // Copy all arguments, locals and temporary values on the stack.
    Pika_memcpy( ctx->bsp,
                 stack.GetAt(0),
                 stack.GetSize() * sizeof(Value) );
    
    
    // Restore the lexical environment.
    
    if (ctx->env)
    {
        LexicalEnv* env = ctx->env;
        if (env->IsAllocated())
        {
            RaiseException("Generator's lexical environment should not be heap allocated.");
        }
        Value* bsp = ctx->GetBasePtr();        
        env->Set(bsp, env->Length());
    }
    
    ctx->retCount = retc;
    state = GS_resumed;
}

size_t Generator::FindBaseHandler(Context* ctx, size_t scopeid)
{
    size_t curr = ctx->handlers.GetSize();
    size_t amt = 0;
    
    while (curr > 0 && ctx->handlers[--curr].scope >= scopeid)
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

namespace {

PIKA_DOC(Generator_getState, "Returns the current [state] of the generator.")

PIKA_DOC(Generator_state, "The current state of the generator. <br />\
It will be one of the following values:\n\
\
'''Generator.CLEAN''' - The generator has not be initialized or run.<br />\
'''Generator.YIELDED''' - The generator has yielded.<br />\
'''Generator.RESUMED''' - The generator is running.<br />\
'''Generator.FINISHED''' - The generator has run and finished either by a return statement or and exception.<br />\
")

int Generator_state(Context* ctx, Value& self)
{
    Generator* gen = static_cast<Generator*>(self.val.object);
    ctx->Push((pint_t)gen->GetState());
    return 1;
}

PIKA_DOC(Generator_getFunction, "Returns the [function] for this generator.")
PIKA_DOC(Generator_function, "The [Function function] this generator is using.")

int Generator_function(Context* ctx, Value& self)
{
    Generator* gen = static_cast<Generator*>(self.val.object);
    Function* func = gen->GetFunction();
    if (func)
        ctx->Push(func);
    else
        ctx->PushNull();
    return 1;
}

PIKA_DOC(Generator_next, "Resumes the generator. This method will return any \
arguments yielded or returned by its [function]. If [toBoolean] returns false \
then an exception will be raised when this method is called. A generator can \
only be resumed with it's [state] is equal to '''Generator.YIELDED'''. \n\
If values returned are from a '''yield''' statement [toBoolean] will be '''true''' and the [state] will be '''Generator.YIELDED'''. \
If values returned are from a '''return''' statement [toBoolean] will be '''false''' and the [state] will be '''Generator.FINISHED'''.\
")

int Generator_next(Context* ctx, Value& self)
{
    Generator* gen = static_cast<Generator*>(self.val.object);
    ctx->PushNull();
    ctx->Push(gen);
    u4 retc = ctx->GetRetCount();
    if (ctx->SetupCall(0, retc))
    {
        ctx->Run();
    }
    return retc;
}

PIKA_DOC(Generator_class, "The Generator class is an [Iterator] that enumerates \
the values yielded by its [function]. Because it is an Iterator, Generators can \
be used in '''for''' loops or you can use the methods [toBoolean] and [next] to control the iteration.\n\
You can create a new generator by calling a function that contains a yield statement.\
[[[\
function count(n)\n\
  for i = 0 to n\n\
    yield i\n\
  end\n\
end\n\
\n\
g = count(10)\n\
print(g) #=> Generator:instance\n\
print(g.next(), g.next(), g.next()) #=> 0 1 2\n\
]]]\
")
}

void Generator::StaticInitType(Engine* eng)
{
    Package* Pkg_World = eng->GetWorld();
    String* Generator_String = eng->AllocString("Generator");
    eng->Generator_Type = Type::Create(eng, Generator_String, eng->Iterator_Type, Generator::Constructor, Pkg_World);
    Pkg_World->SetSlot(Generator_String, eng->Generator_Type);
    eng->Generator_Type->SetDoc(PIKA_GET_DOC(Generator_class));
    SlotBinder<Generator>(eng, eng->Generator_Type)
    .Method(&Generator::ToBoolean,  "toBoolean", PIKA_GET_DOC(Generator_toBoolean))
    .Register(Generator_next, "next", 0, false, true, PIKA_GET_DOC(Generator_next))
    .Constant((pint_t)GS_clean,     "CLEAN")
    .Constant((pint_t)GS_yielded,   "YIELDED")
    .Constant((pint_t)GS_resumed,   "RESUMED")
    .Constant((pint_t)GS_finished,  "FINISHED")
    ;
    
    static RegisterProperty Generator_Properties[] =
    {
        { "state",    Generator_state,    "getState",    0, 0, false, PIKA_GET_DOC(Generator_getState),    0, PIKA_GET_DOC(Generator_state)    },
        { "function", Generator_function, "getFunction", 0, 0, false, PIKA_GET_DOC(Generator_getFunction), 0, PIKA_GET_DOC(Generator_function) }
    };
    
    eng->Generator_Type->EnterProperties(Generator_Properties, countof(Generator_Properties));
}

}// pika
