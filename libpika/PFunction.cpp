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
#include "PScript.h"

namespace pika {

// LexicalEnv //////////////////////////////////////////////////////////////////////////////////////////

LexicalEnv::LexicalEnv(bool close)
        : values(0),
        length(0),
        allocated(false),
        mustClose(close)
{}

LexicalEnv::LexicalEnv(LexicalEnv* rhs)
        : values(rhs->values),
        length(rhs->length),
        allocated(false),
        mustClose(rhs->mustClose)
{
    if (rhs->allocated)
    {
        Allocate();
    }
}

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

LexicalEnv* LexicalEnv::Create(Engine* eng, LexicalEnv* lex)
{
    LexicalEnv* env=0;
    PIKA_NEW(LexicalEnv, env, (lex));
    eng->AddToGC(env);
    return env;
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

Function::Function(Engine* eng, Type* p, Def* funcdef, Package* loc, Function* parent_func, String* doc)
        : ThisSuper(eng, p),
        defaults(0),
        lexEnv(0),
        def(funcdef),
        parent(parent_func),
        location(loc),
        __doc(doc)
{
    ASSERT(engine);
    ASSERT(def);
    ASSERT(location);
}

Function::Function(const Function* rhs) : ThisSuper(rhs),
        defaults(rhs->defaults),
        lexEnv(rhs->lexEnv),
        def(rhs->def),
        parent(rhs->parent),
        location(rhs->location),
        __doc(rhs->__doc)
{
}

Function* Function::Create(Engine* eng, Def* def, Package* loc, Function* parent)
{
    Function* cl = 0;
    PIKA_NEW(Function, cl, (eng, eng->Function_Type, def, loc ? loc : eng->GetWorld(), parent, 0));
    eng->AddToGC(cl);
    return cl;
}

Function* Function::Create(Engine* eng, RegisterFunction* rf, Package* loc)
{
    String* funcName = eng->AllocString(rf->name);
    Def* def = Def::CreateWith(eng, funcName, rf->code,
                               rf->argc, rf->flags, 0, rf->__doc);
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
    if (__doc) __doc->Mark(c);
}

int Function::DetermineLineNumber(code_t* xpc)
{
    if (!def)
        return -1; // return a bogus line number.
    
    int lineno = def->line; // The starting line of the function.    
    code_t* pc = (xpc - 1);    
    size_t lastline = def->lineInfo.GetSize();
    
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
    Function* fn = 0;
    GCNEW(engine, Function, fn, (this));
    return fn;
}

String* Function::GetName() const
{ 
    return def ? def->name: engine->emptyString;
}

String* Function::ToString()
{
    return String::ConcatSep(GetType()->GetName(), def->name, ':');
}

Function* Function::BindWith(Value& withobj)
{
    BoundFunction* bc = BoundFunction::Create(engine, 0, this, withobj);
    return bc;
}

bool Function::IsLocatedIn(Package* pkg) const
{
    if (!pkg)
        return false;
        
    Package* curr = location;
    
    while (curr && curr != pkg)
        curr = curr->GetSuper();
        
    return curr == pkg;
}

String* Function::GetFileName() const
{
    if (!location)
        return engine->emptyString;
    
    GCPAUSE_NORUN(engine);
    Package* curr = location;
    
    while (curr) {
        if (curr->IsDerivedFrom(Script::StaticGetClass())) {
            Script* s = static_cast<Script*>(curr);
            return s->GetFileName();            
        }
        curr = curr->GetSuper();
    }
    return engine->emptyString;
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
                               Type*     tclass,
                               String*   doc)
: Function(eng, t, func_def, loc, c, doc),
classtype(tclass)
{
    ASSERT(classtype);
}

InstanceMethod::InstanceMethod(Engine* eng, Type* tthis, Function* f, Type* tclass)
: Function(eng,
           tthis,
           f->GetDef(),
           f->location,
           f->parent,
           f->__doc),
classtype(tclass)
{
    if (f)
    {
        lexEnv = f->lexEnv;
        defaults = f->defaults;
    }
}

InstanceMethod::InstanceMethod(const InstanceMethod* rhs) : 
    ThisSuper(rhs), 
    classtype(rhs->classtype)
{
}

InstanceMethod* InstanceMethod::Create(Engine* eng, Type* type, Function* f, Type* t)
{
    InstanceMethod* im = 0;
    GCNEW(eng, InstanceMethod, im, (eng, type ? type : eng->InstanceMethod_Type, f, t));
    return im;
}

InstanceMethod::~InstanceMethod() {}

InstanceMethod* InstanceMethod::Create(Engine* eng, Type* type, Function* c, Def* func_def,
                                       Package* loc, Type* tclass)
{
    InstanceMethod* im = 0;
    GCNEW(eng, InstanceMethod, im, (eng, type ? type : eng->InstanceMethod_Type, c, func_def, loc, tclass));
    return im;
}

Object* InstanceMethod::Clone()
{
    InstanceMethod* fn = 0;
    GCNEW(engine, InstanceMethod, fn, (this));
    return fn;
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

ClassMethod::ClassMethod(const ClassMethod* rhs) : ThisSuper(rhs),
    classtype(rhs->classtype)
{
}

ClassMethod::ClassMethod(Engine* eng, Type* tthis, Function* f, Type* tclass)
: Function(eng,
           tthis,
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

Object* ClassMethod::Clone()
{
    ClassMethod* fn = 0;
    GCNEW(engine, ClassMethod, fn, (this));
    return fn;
}

ClassMethod* ClassMethod::Create(Engine* eng, Type* type, Function* f, Type* t)
{
    ClassMethod* cm = 0;
    GCNEW(eng, ClassMethod, cm, (eng, type ? type : eng->ClassMethod_Type, f, t));
    return cm;
}

ClassMethod* ClassMethod::Create(Engine* eng, Type* type, Function* c, Def* func_def, Package* loc, Type* tclass)
{
    ClassMethod* cm = 0;
    GCNEW(eng, ClassMethod, cm, (eng, type ? type : eng->ClassMethod_Type, c, func_def, loc, tclass));
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
        : ThisSuper(eng, t, c->def, c->location, c->parent),
        closure(c),
        self(bound)
{
    lexEnv = c->lexEnv;
}

BoundFunction::BoundFunction(const BoundFunction* rhs)
        : ThisSuper(rhs),
        closure(rhs->closure),
        self(rhs->self)
{
}

BoundFunction::~BoundFunction() {}

BoundFunction* BoundFunction::Create(Engine* eng, Type* type, Function* c, Value& bound)
{
    BoundFunction* b = 0;
    PIKA_NEW(BoundFunction, b, (eng, type ? type : eng->BoundFunction_Type, c, bound));
    eng->AddToGC(b);
    return b;
}

void BoundFunction::Init(Context* ctx)
{
    closure = ctx->GetArgT<Function>(0);
    if (!closure)
        RaiseException(Exception::ERROR_type, "BoundFunction.init excepts a valid Function and self object.");
    self = ctx->GetArg(1);
    WriteBarrier(closure);
    lexEnv = closure->lexEnv;
    if (self.IsCollectible())
        WriteBarrier(self);
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

Object* BoundFunction::Clone()
{
    BoundFunction* fn = 0;
    GCNEW(engine, BoundFunction, fn, (this));
    return fn;
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
    tree->CalculateResources(0);
    
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

namespace {

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

int Function_getLocal(Context* ctx, Value& self)
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
    RaiseException(Exception::ERROR_index, "Attempt to access local variable: "PINT_FMT".", idx);
    return 0;
}

int Function_setLocal(Context* ctx, Value& self)
{
    GETSELF(Function, fn, "Function");
    pint_t idx = ctx->GetIntArg(0);
    Value& val = ctx->GetArg(1);
    
    if (fn->lexEnv)
    {
        pint_t count = static_cast<pint_t>(fn->lexEnv->Length());
        if (idx >= 0 && idx < count)
        {            
            fn->lexEnv->At(idx) = val;
            return 0;
        }
    }    
    RaiseException(Exception::ERROR_index, "Attempt to access local variable: "PINT_FMT".", idx);
    return 0;
}


int Function_getLocalCount(Context* ctx, Value& self)
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

int Function_printBytecode(Context* ctx, Value&)
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

int Function_printLiterals(Context* ctx, Value& self)
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

/* A native, do nothing function. */
int null_Function(Context*, Value&) { return 0; }

Function* Create_null_Function(Engine* eng)
{
    GCPAUSE_NORUN(eng);
    Package* pkg = eng->GetWorld();
    Def* def = Def::CreateWith(eng, eng->emptyString, null_Function, 0, DEF_VAR_ARGS, 0, 0);
    return Function::Create(eng, def, pkg, 0);
}

int Function_name(Context* ctx, Value& self)
{
    // The Function type needs to dispatch getName based on type.
    // Since Function.name will call Function::GetName instead of Type::GetName
    // we do this.
    if (self.IsDerivedFrom(Function::StaticGetClass()))
    {
        Function* fn = (Function*)self.val.object;
        ctx->Push(fn->GetName());
        return 1;        
    }
    else if (self.IsDerivedFrom(Type::StaticGetClass()))
    {
        Type* t = (Type*)self.val.object;
        ctx->Push(t->GetName());   
        return 1;
    }    
    RaiseException(Exception::ERROR_type, "Attempt to call Function.getName with incorrect type.");
    return 0;    
}

int Function_parent(Context* ctx, Value& self)
{
    if (self.IsDerivedFrom(Function::StaticGetClass()))
    {
        Function* fn = (Function*)self.val.object;
        ctx->Push(fn->GetParent());
        return 1;        
    }
    else if (self.IsDerivedFrom(Type::StaticGetClass()))
    {
        Type* t = (Type*)self.val.object;
        ctx->Push(t->GetSuper());   
        return 1;
    }    
    RaiseException(Exception::ERROR_type, "Attempt to call Function.getParent with incorrect type.");
    return 0;    
}

int Function_location(Context* ctx, Value& self)
{
    if (self.IsDerivedFrom(Function::StaticGetClass()))
    {
        Function* fn = (Function*)self.val.object;
        ctx->Push(fn->GetLocation());
        return 1;        
    }
    else if (self.IsDerivedFrom(Type::StaticGetClass()))
    {
        Type* t = (Type*)self.val.object;
        ctx->Push(t->GetLocation());   
        return 1;
    }    
    RaiseException(Exception::ERROR_type, "Attempt to call Function.getLocation with incorrect type.");
    return 0;    
}

}//namespace

void InstanceMethod::Constructor(Engine* eng, Type* obj_type, Value& res)
{
    InstanceMethod* im = InstanceMethod::Create(eng, obj_type, eng->null_Function, eng->Value_Type);
    res.Set(im);
}

void BoundFunction::Constructor(Engine* eng, Type* obj_type, Value& res)
{
    Value self(NULL_VALUE);
    BoundFunction* cm = BoundFunction::Create(eng, obj_type, eng->null_Function, self);
    res.Set(cm);
}

void ClassMethod::Constructor(Engine* eng, Type* obj_type, Value& res)
{
    ClassMethod* bf = ClassMethod::Create(eng, obj_type, eng->null_Function, eng->Value_Type);
    res.Set(bf);
}

void Function::Constructor(Engine* eng, Type* obj_type, Value& res)
{
    GCPAUSE_NORUN(eng);
    Package* pkg = eng->GetWorld();
    Def* def = Def::CreateWith(eng, eng->emptyString, null_Function, 0, DEF_VAR_ARGS, 0, 0);
    Context* ctx = eng->GetActiveContext();
    if (ctx)
    {
        Package* currpkg = ctx->GetPackage();
        if (currpkg)
        {
            pkg = currpkg;
        }
    }
    Object* obj = Function::Create(eng, def, pkg, 0);
    obj->SetType(obj_type);
    res.Set(obj);
}

String* Function::GetDoc()
{ 
    if (!__doc) {
        if (def && def->__native_doc__) {
            __doc = engine->GetString(def->__native_doc__);
            WriteBarrier(__doc);
        } else {
            return engine->emptyString;
        }
    }
    return __doc; 
}

void Function::SetDoc(String* doc)
{
    if (doc)
        WriteBarrier(doc);
    __doc = doc;
}

void Function::SetDoc(const char* cstr)
{
    if (!cstr) {
        __doc = 0;
    } else {
        SetDoc(engine->GetString(cstr));
    }
}

int Function_apply(Context* ctx, Value& self)
{
    u2 argc = ctx->GetArgCount();
    u2 retc = ctx->GetRetCount();
    ctx->CheckStackSpace(argc + 2 + retc);
        
    const Value* btm  = ctx->GetArgs();
    const Value* top  = btm + argc;
    
    for (const Value* v = btm; v < top; ++v)
        ctx->Push(*v);
    
    ctx->PushNull();
    ctx->Push(self);
    
    if (ctx->SetupCall(argc, retc)) {
        ctx->Run();
    }
    return retc;
}

void Function::StaticInitType(Engine* eng)
{
    SlotBinder<Function>(eng, eng->Function_Type)
    .Method(&Function::BindWith, "bindto")
    .RegisterMethod(Function_apply,         "apply")
    .RegisterMethod(Function_getBytecode,   "getBytecode")
    .RegisterMethod(Function_getText,       "getText")
    .RegisterMethod(Function_getLocal,      "getLocal")
    .RegisterMethod(Function_setLocal,      "setLocal")    
    .RegisterMethod(Function_getLocalCount, "getLocalCount")
    .RegisterMethod(Function_printLiterals, "printLiterals")
    .RegisterClassMethod(Function_printBytecode,    "printBytecode")
    .Constant((pint_t)PIKA_MAX_RETC,             "MAX_RET_COUNT")
    .Constant((pint_t)PIKA_MAX_NESTED_FUNCTIONS, "MAX_FUNCTION_DEPTH")
    ;
    
    static RegisterProperty Function_Properties[] = {
        { "name",     Function_name,     "getName",     0, 0, true, 0, 0 },
        { "parent",   Function_parent,   "getParent",   0, 0, true, 0, 0 },
        { "location", Function_location, "getLocation", 0, 0, true, 0, 0 },
    };
        
    eng->Function_Type->EnterProperties(Function_Properties, countof(Function_Properties));
        
    eng->null_Function = Create_null_Function(eng);
    
    SlotBinder<InstanceMethod>(eng, eng->InstanceMethod_Type)
    .PropertyR("classObject", &InstanceMethod::GetClassType, "getClassObject")
    ;
    
    SlotBinder<ClassMethod>(eng, eng->ClassMethod_Type)
    .PropertyR("classObject", &ClassMethod::GetClassType, "getClassObject")
    ;
    
    SlotBinder<BoundFunction>(eng, eng->BoundFunction_Type)
    .PropertyR("boundFunction", &BoundFunction::GetBoundFunction, "getBoundFunction")
    .PropertyR("boundSelf",     &BoundFunction::GetBoundSelf,     "getBoundSelf")
    ;
    
    LocalsObject::StaticInitType(eng);
}

}// pika
