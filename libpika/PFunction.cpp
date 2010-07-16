/*
 * PFunction.cpp
 * See Copyright Notice in Pika.h
 */
#include "PConfig.h"
#include "PValue.h"
#include "PCollector.h"
#include "PDef.h"
#include "PBasic.h"
#include "PEngine.h"
#include "PObject.h"
#include "PFunction.h"
#include "PEngine.h"
#include "PContext.h"
#include "PLocalsObject.h"
#include "PNativeBind.h"
#include "PParser.h"
#include "PAst.h"

namespace pika {

// LexicalEnv //////////////////////////////////////////////////////////////////////////////////////////

LexicalEnv::LexicalEnv(bool close)
        :
        values(0),
        length(0),
        allocated(false),
        mustClose(close) {}
        
LexicalEnv::~LexicalEnv() { Deallocate(); }

void LexicalEnv::MarkRefs(Collector* c)
{
    MarkValues(c, values, values + length);
}

void LexicalEnv::Allocate()
{
    if (!allocated && length)
    {
        const size_t numBytes = sizeof(Value) * length;
        const Value* old = values;
        values = (Value*)Pika_malloc(numBytes);
        Pika_memcpy(values, old, numBytes);
        allocated = true;
    }
}

void LexicalEnv::Deallocate()
{
    if (allocated)
    {
        Pika_free(values);
        allocated = false;
    }
    length = 0;
    values = 0;
}

void LexicalEnv::EndCall()
{
    if (mustClose)
    {
        ASSERT(!IsAllocated());
        Allocate();
    }
    else
    {
        Set(0, 0);
    }
}

LexicalEnv* LexicalEnv::Create(Engine* eng, bool close)
{
    LexicalEnv* ov;
    PIKA_NEW(LexicalEnv, ov, (close));
    eng->AddToGC(ov);
    return ov;
}

Defaults::Defaults(Value* v, size_t l) : length(l) 
{
    size_t amt = sizeof(Value)*length;
    values = (Value*)Pika_malloc(amt);
    Pika_memcpy(values, v, amt);
}

void Defaults::MarkRefs(Collector* c)
{
    MarkValues(c, values, values + length);
}

Defaults::~Defaults() { if (values) Pika_free(values); }
    
Defaults* Defaults::Create(Engine* eng, Value* v, size_t l)
{
    Defaults* defs;
    GCNEW(eng, Defaults, defs, (v, l));
    return defs;
}

// Function ////////////////////////////////////////////////////////////////////////////////////////

PIKA_IMPL(Function)

Function::Function(Engine* eng, Type* p, Def* funcdef, Package* loc, Function* parent_func)
        : ThisSuper(eng, p),
        defaults(0),
        lexEnv(0),
        def(funcdef),
        parent(parent_func),
        location(loc)
{
    ASSERT(engine);
    ASSERT(def);
    ASSERT(location);
}

Function* Function::Create(Engine* eng, Def* def, Package* loc, Function* parent)
{
    Function* cl = 0;
    PIKA_NEW(Function, cl, (eng, eng->Function_Type, def, loc ? loc : eng->GetWorld(), parent));
    eng->AddToGC(cl);
    return cl;
}

Function* Function::Create(Engine* eng, RegisterFunction* rf, Package* loc)
{
    String* funcName = eng->AllocString(rf->name);
    Def* def = Def::CreateWith(eng, funcName, rf->code,
                               rf->argc, rf->varargs,
                               rf->strict, 0);    
    return Create(eng, def, loc, 0);
}

Function::~Function() {}

void Function::BeginCall(Context* ctx)
{
    ASSERT(def);
}

void Function::MarkRefs(Collector* c)
{
    ThisSuper::MarkRefs(c);
    
    if (parent)   parent->Mark(c);
    if (def)      def->Mark(c);
    if (lexEnv)   lexEnv->Mark(c);
    if (location) location->Mark(c);
    if (defaults) defaults->Mark(c);
}

int Function::DetermineLineNumber(code_t* xpc)
{
    if (!def)
        return -1; // return a bogus line number.
    
    int     lineno   = def->line; // The starting line of the function.    
    code_t* pc       = (xpc - 1);    
    size_t  lastline = def->lineInfo.GetSize();
    
    // No line information so return the lineno.
    if (!lastline)
        return lineno;
    
    // Loop through each line in the function.
    lastline -= 1;
    for (size_t i = 0; i <= lastline; ++i)
    {
        // Get the bytecode position of both this and the next line (if available).
        code_t* currpos = def->lineInfo[i].pos;
        code_t* nextpos = (i == lastline) ? currpos : def->lineInfo[i + 1].pos;
        
        if (pc == currpos || 
           (pc > currpos && ((pc < nextpos) || (i == lastline))))
        {
            lineno = def->lineInfo[i].line;
            break;        
        }
    }
    return lineno;
}

Object* Function::Clone()
{
    Function* c = Function::Create(engine, def, location, parent);
    c->lexEnv  = lexEnv;
    return c;
}

String* Function::GetName()
{ 
    return def ? def->name: engine->emptyString;
}

Value Function::Apply(Value& aself, Array* args)
{
    Context* ctx = engine->GetActiveContextSafe();
    u2 argc = args ? static_cast<u2>(args->GetLength()) : 0;
    
    for (u2 a = 0; a < argc; ++a)
        ctx->Push(args->At(a));
        
    ctx->Push(aself);
    ctx->Push(this);
    
    if (ctx->SetupCall(argc))
    {
        ctx->Run();
    }
    Value& ret = ctx->PopTop();
    return ret;
}

String* Function::ToString()
{
    return String::ConcatSpace(GetType()->GetName(), def->name);
}

Function* Function::BindWith(Value& withobj)
{
    BoundFunction* bc = BoundFunction::Create(engine, this, withobj);
    return bc;
}

bool Function::IsLocatedIn(Package* pkg)
{
    if (!pkg)
        return false;
        
    Package* curr = location;
    
    while (curr && curr != pkg)
        curr = curr->GetSuper();
        
    return curr == pkg;
}

String* Function::GetDotPath()
{
    GCPAUSE_NORUN(engine);
    String* res = String::Concat(location->GetDotName(), engine->dot_String);
    if (def->name->GetLength() == 0)
    {
        res = String::Concat(res, engine->AllocString("(anonymous)"));
    }
    else
    {
        res = String::Concat(res, def->name);
    }
    return res;
}

// InstanceMethod //////////////////////////////////////////////////////////////////////////////////

PIKA_IMPL(InstanceMethod)

InstanceMethod::InstanceMethod(Engine*   eng,
                               Type*     t,
                               Function* c,
                               Def*      func_def,
                               Package*  loc,
                               Type*     tclass)
: Function(eng, t, func_def, loc, c),
classtype(tclass)
{
    ASSERT(classtype);
}

InstanceMethod::InstanceMethod(Engine* eng, Function* f, Type* tclass)
: Function(eng,
           eng->InstanceMethod_Type,
           f->GetDef(),
           f->location,
           f->parent),
classtype(tclass)
{
    if (f)
    {
        lexEnv = f->lexEnv;
        defaults = f->defaults;
    }
}

InstanceMethod* InstanceMethod::Create(Engine* eng, Function* f, Type* t)
{
    InstanceMethod* im = 0;
    GCNEW(eng, InstanceMethod, im, (eng, f, t));
    return im;
}

InstanceMethod::~InstanceMethod() {}

InstanceMethod* InstanceMethod::Create(Engine* eng, Function* c, Def* func_def,
                                       Package* loc, Type* tclass)
{
    InstanceMethod* im = 0;
    GCNEW(eng, InstanceMethod, im, (eng, eng->InstanceMethod_Type, c, func_def, loc, tclass));
    return im;
}

void InstanceMethod::BeginCall(Context* ctx)
{
    if (classtype)
    {
        Value& self = ctx->GetSelf();
        if (!classtype->IsInstance(self))
        {
            String* typestr = engine->GetTypenameOf(self);
            if (def->name->GetLength())
            {
                ctx->ReportRuntimeError(Exception::ERROR_type,
                                        "Attempt to call InstanceMethod \"%s\".\n\ttype \"%s\" is not a subclass of type \"%s\".",
                                        def->name->GetBuffer(),
                                        typestr->GetBuffer(),
                                        classtype->GetDotName()->GetBuffer());
            }
            else
            {
                ctx->ReportRuntimeError(Exception::ERROR_type,
                                        "Attempt to call anonymous InstanceMethod.\n\ttype \"%s\" is not a subclass of type \"%s\".",
                                        typestr->GetBuffer(),
                                        classtype->GetDotName()->GetBuffer());
            }
        }
    }
}

void InstanceMethod::MarkRefs(Collector *c)
{
    Function::MarkRefs(c);
    if (classtype)
        classtype->Mark(c);
}

String* InstanceMethod::GetDotPath()
{
    if (!classtype)
        return Function::GetDotPath();
        
    GCPAUSE_NORUN(engine);
    String* res = this->GetLocation()->GetDotName();//
    //res = String::Concat(res, classtype->GetName());
    res = String::Concat(res, engine->dot_String);
    if (def->name->GetLength() == 0)
    {
        res = String::Concat(res, engine->AllocString("(anonymous)"));
    }
    else
    {
        res = String::Concat(res, def->name);
    }
    return res;
}

// ClassMethod /////////////////////////////////////////////////////////////////////////////////////

PIKA_IMPL(ClassMethod)

ClassMethod::ClassMethod(Engine*   eng,
                         Type*     t,
                         Function* c,
                         Def*      func_def,
                         Package*  loc,
                         Type*     tclass)
: Function(eng, t, func_def, loc, c),
classtype(tclass)
{
    ASSERT(classtype);
}

ClassMethod::ClassMethod(Engine* eng, Function* f, Type* tclass)
: Function(eng,
           eng->ClassMethod_Type,
           f->GetDef(),
           f->location,
           f->parent),
            classtype(tclass)
{
    if (f)
    {
        lexEnv = f->lexEnv;
        defaults = f->defaults;
    }
}

ClassMethod::~ClassMethod() {}

ClassMethod* ClassMethod::Create(Engine* eng, Function* f, Type* t)
{
    ClassMethod* cm = 0;
    GCNEW(eng, ClassMethod, cm, (eng, f, t));
    return cm;
}

ClassMethod* ClassMethod::Create(Engine* eng, Function* c, Def* func_def, Package* loc, Type* tclass)
{
    ClassMethod* cm = 0;
    GCNEW(eng, ClassMethod, cm, (eng, eng->ClassMethod_Type, c, func_def, loc, tclass));
    return cm;
}

void ClassMethod::BeginCall(Context* ctx)
{
    if (classtype)
    {
        Value self(classtype);
        ctx->SetSelf(self);
    }
}

void ClassMethod::MarkRefs(Collector *c)
{
    Function::MarkRefs(c);
    if (classtype)
        classtype->Mark(c);
}

String* ClassMethod::GetDotPath()
{
    if (!classtype)
        return Function::GetDotPath();
        
    GCPAUSE_NORUN(engine);
    String* res = String::Concat(this->GetLocation()->GetDotName(), engine->dot_String);
    res = String::Concat(res, classtype->GetName());
    res = String::Concat(res, engine->dot_String);
    if (def->name->GetLength() == 0)
    {
        res = String::Concat(res, engine->AllocString("(anonymous)"));
    }
    else
    {
        res = String::Concat(res, def->name);
    }
    return res;
}

// BoundFunction ///////////////////////////////////////////////////////////////////////////////////

PIKA_IMPL(BoundFunction)

BoundFunction::BoundFunction(Engine* eng, Type* t, Function* c, Value& bound)
        : Function(eng, t, c->def, c->location, c->parent),
        closure(c),
        self(bound)
{
    lexEnv = c->lexEnv;
}

BoundFunction::~BoundFunction() {}

BoundFunction* BoundFunction::Create(Engine* eng, Function* c, Value& bound)
{
    BoundFunction* b = 0;
    PIKA_NEW(BoundFunction, b, (eng, eng->BoundFunction_Type, c, bound));
    eng->AddToGC(b);
    return b;
}

void BoundFunction::BeginCall(Context* ctx)
{
    ctx->SetSelf(self);
    closure->BeginCall(ctx);
}

void BoundFunction::MarkRefs(Collector *c)
{
    Function::MarkRefs(c);
    if (closure) closure->Mark(c);
    MarkValue(c, self);
}

void Function::Init(Context* ctx)
{
    String* source_text = ctx->GetStringArg(0);
    InitWithBody(source_text);
}

void Function::InitWithBody(String* body)
{
    GCPAUSE_NORUN(engine);
    std::auto_ptr<CompileState> cs    ( new CompileState(engine) );
    std::auto_ptr<Parser>       parser( new Parser(cs.get(), body->GetBuffer(), body->GetLength()) );
    
    Program* tree = parser->DoFunctionParse();
    ASSERT(tree);
    tree->CalculateResources(0, *cs);
    
    if (cs->HasErrors())
        RaiseException(Exception::ERROR_syntax, "Attempt to compile script failed.\n");
        
    tree->GenerateCode();
    
    if (cs->HasErrors())
        RaiseException(Exception::ERROR_syntax, "Attempt to generate code failed.\n");
        
    def = tree->def;
    location = engine->GetWorld();
    WriteBarrier(def);
    WriteBarrier(location);
}

}// pika

int Function_getText(Context* ctx, Value& self)
{
    GETSELF(Function, cl, "Function");
    
    if (cl->GetDef() && cl->GetDef()->source)
    {
        ctx->Push(cl->GetDef()->source);
        return 1;
    }
    ctx->PushNull();
    return 1;
}

int Function_getBytecode(Context* ctx, Value& self)
{
    GETSELF(Function, cl, "Function");
    
    Def* def = cl->GetDef();
    
    if (def && def->bytecode)
    {
        Bytecode* bc = def->bytecode;
        if (bc->code)
        {
            char*   codeptr = reinterpret_cast<char*>(bc->code);
            size_t  codelen = sizeof(code_t) * bc->length;
            String* codestr = ctx->GetEngine()->AllocString(codeptr, codelen);
            
            ctx->Push(codestr);
            return 1;
        }
    }
    ctx->PushNull();
    return 1;
}

static int Function_getLocal(Context* ctx, Value& self)
{
    GETSELF(Function, fn, "Function");
    pint_t idx = ctx->GetIntArg(0);
    
    if (fn->lexEnv)
    {
        pint_t count = static_cast<pint_t>(fn->lexEnv->Length());
        if (idx >= 0 && idx < count)
        {
            ctx->Push(fn->lexEnv->At(idx));
            return 1;
        }
    }
    
    RaiseException("Attempt to access local variable: %d", (u2)idx);
    return 0;
}

static int Function_getLocalCount(Context* ctx, Value& self)
{
    GETSELF(Function, fn, "Function");
    pint_t count = 0;
    if (fn->lexEnv)
    {
        count = static_cast<pint_t>(fn->lexEnv->Length());
    }
    
    ctx->Push(count);
    return 1;
}

static int Function_gen(Context* ctx, Value& self)
{
    GETSELF(Function, f, "Function");
    Context* c = Context::Create(f->GetEngine(), f->GetEngine()->Context_Type);
    ctx->Push(c);
    
    u4 argc = ctx->GetArgCount();
    Value* args = ctx->GetArgs();
    
    c->CheckStackSpace( argc + 2 );
    for (u4 a = 0; a < argc; ++a)
    {
        c->Push(args[a]);
    }
    //c->PushNull();
    c->Push(f);
    c->Setup(ctx);
    
    return 1;
}

// TODO: it currently takes our argc which causes a problem
static int Function_genAs(Context* ctx, Value& self)
{
    GETSELF(Function, f, "Function");
    Context* c = Context::Create(f->GetEngine(), f->GetEngine()->Context_Type);
    ctx->Push(c);
    
    u4 argc = ctx->GetArgCount();
    Value* args = ctx->GetArgs();
    if (!argc)
    {
        c->PushNull();
    }
    else
    {
        c->CheckStackSpace( argc + 2 );
        for (u4 a = 1; a < argc; ++a)
        {
            c->Push(args[a]);
        }
        c->Push(args[0]);
    }
    c->Push(f);
    c->Setup(ctx);
    
    return 1;
}

static int Function_printBytecode(Context* ctx, Value&)
{
    String* str = ctx->GetStringArg(0);
    size_t len = str->GetLength();
    if (len % (sizeof(code_t)))
        RaiseException("illegal bytecode size");
    code_t* bc = (code_t*)str->GetBuffer();
    code_t* end = bc + (len / (sizeof(code_t)));
    for (code_t* curr = bc; curr < end; curr++)
    {
        printf("%.4u  ", (u4)(curr - bc));
        Pika_PrintInstruction(*curr);
    }
    return 0;
}

static int Function_printLiterals(Context* ctx, Value& self)
{
    GETSELF(Function, f, "Function");
    Def* def = f->GetDef();
    GCPAUSE(f->GetEngine());
    if (def)
    {
        LiteralPool* literals = def->literals;
        for (size_t i = 0; i < literals->GetSize(); ++i)
        {
            const Value& val = literals->Get(i);
            String* s = f->GetEngine()->ToString(ctx, val);
            puts(s->GetBuffer());
        }
    }
    return 0;
}

// LocalObject API /////////////////////////////////////////////////////////////////////////////////

static int LocalsObject_getParent(Context* ctx, Value& self)
{
    LocalsObject* obj = (LocalsObject*)self.val.object;    
    Object* parent = obj->GetParent();
    
    if (parent)
    {
        ctx->Push(parent);
    }
    else
    {
        ctx->PushNull();
    }
    return 1;
}


int null_Function(Context*, Value&) { return 0; }

static int Function_call(Context* ctx, Value& self)
{
    Array* args = 0;
    Value selfObj = NULL_VALUE;
    u4 argc = ctx->GetArgCount();
    switch (argc)
    {
    case 2:
        selfObj = ctx->GetArg(0);
        args = ctx->GetArgT<Array>(1); 
        break;
    case 1:
        args = ctx->GetArgT<Array>(0);
        break;
    case 0:
        break;
    default:
        ctx->WrongArgCount();
    }
    Function* fn = self.val.function;
    ctx->Push(fn->Apply(selfObj, args));
    return 1;
}

void InitFunctionAPI(Engine* eng)
{
    SlotBinder<Function>(eng, eng->Function_Type)
    .Method(&Function::Apply,               "apply")
    .RegisterMethod(Function_getBytecode,   "getBytecode")
    .RegisterMethod(Function_getText,       "getText")
    .RegisterMethod(Function_getLocal,      "getLocal")
    .RegisterMethod(Function_getLocalCount, "getLocalCount")
    .RegisterMethod(Function_call,          "call")
    .RegisterMethod(Function_gen,           "gen")
    .RegisterMethod(Function_genAs,         "genAs")
    .RegisterMethod(Function_printLiterals, "printLiterals")
    .RegisterClassMethod(Function_printBytecode,    "printBytecode")
    .PropertyR("name",      &Function::GetName,     "getName")
    .PropertyR("location",  &Function::GetLocation, "getLocation")
    .PropertyR("parent",    &Function::GetParent,   "getParent")
    .Constant((pint_t)PIKA_MAX_RETC, "MAX_RET_COUNT")
    .Constant((pint_t)PIKA_MAX_NESTED_FUNCTIONS, "MAX_FUNCTION_DEPTH")
    ;
    
    SlotBinder<InstanceMethod>(eng, eng->InstanceMethod_Type)
    .PropertyR("instanceType", &InstanceMethod::GetClassType, "GetInstanceType")
    ;
    
    SlotBinder<ClassMethod>(eng, eng->ClassMethod_Type)
    .PropertyR("instanceType", &ClassMethod::GetClassType, "GetInstanceType")
    ;
    
    SlotBinder<BoundFunction>(eng, eng->BoundFunction_Type)
    .PropertyR("boundFunction",  &BoundFunction::GetBoundFunction,  "getBoundFunction")
    .PropertyR("boundSelf",      &BoundFunction::GetBoundSelf,      "getBoundSelf")
    ;
    
    struct RegisterProperty localsObject_Properties[] =
    {
        { "parent", LocalsObject_getParent, "getParent", 0, 0 }
    };
    eng->LocalsObject_Type->EnterProperties(localsObject_Properties, countof(localsObject_Properties));
    eng->LocalsObject_Type->SetFinal(true);
    eng->LocalsObject_Type->SetAbstract(true);
}
