/*
 *  PScript.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PParser.h"
#include "PPlatform.h"

#if defined(PIKA_DEBUG_OUTPUT)
#   include "PProfiler.h"
#   include <iostream>
#endif

namespace pika {

PIKA_IMPL(Script)

Script::Script(Engine* eng, Type* scriptType, String* name, Package* pkg)
        : ThisSuper(eng, scriptType, name, pkg),
        literals(0),
        context(0),
        entryPoint(0),
        arguments(0),
        import_value(0),
        firstRun(false),
        running(false),
        file_name(0)

{}

Script::Script(const Script* rhs) :
        ThisSuper(rhs),
        literals(rhs->literals),
        context(rhs->context),
        entryPoint(rhs->entryPoint),
        arguments(rhs->arguments),
        import_value(rhs->import_value),
        firstRun(rhs->firstRun),
        running(rhs->running),
        file_name(rhs->file_name)
{
}
        
Script::~Script()
{}

Object* Script::Clone()
{
    Script* s = 0;
    GCNEW(engine, Script, s, (this));
    return s;
}

void Script::Initialize(LiteralPool* lp, Context* ctx, Function* entry, String* path)
{
    ASSERT(lp);
    ASSERT(ctx);
    ASSERT(entry);    
    
    if (path) {
        char* real_name = Pika_GetFullPath(path->GetBuffer());
        if (real_name) {
            this->file_name = engine->AllocString(real_name);
            
        }
    }
    
    if (!file_name) {
        this->file_name = path ? path : engine->emptyString;
    }
    
    if (firstRun)
        firstRun = false;
    literals   = lp;
    context    = ctx;
    entryPoint = entry;
    
    WriteBarrier(file_name);
    WriteBarrier(literals);
    WriteBarrier(context);
    WriteBarrier(entryPoint);
}

Script* Script::CreateWithBuffer(Engine* eng, String* buff, String* name, Package* loc)
{
    Program* tree = 0;    
    Script* script = 0; 
    if (!loc) loc = eng->GetWorld();
    if (!name) name = eng->emptyString;
    
    {   GCPAUSE_NORUN(eng);
        std::auto_ptr<CompileState> cs    ( new CompileState(eng) );
        std::auto_ptr<Parser>       parser( new Parser(cs.get(), 
                                                       buff->GetBuffer(),
                                                       buff->GetLength()) );
        
        tree = parser->DoParse();
        if (!tree)
            RaiseException(Exception::ERROR_syntax, "Attempt to generate type \"%s\" parse tree failed.\n", name->GetBuffer());
        tree->CalculateResources(0);
        
        if (cs->HasErrors())
            RaiseException(Exception::ERROR_syntax, "Attempt to compile script failed.\n");
        
        tree->GenerateCode();
        
        if (cs->HasErrors())
            RaiseException(Exception::ERROR_syntax, "Attempt to generate code failed.\n");       
        
        script = Script::Create(eng, name, loc);
        Function* entryPoint = Function::Create(eng, 
                                                tree->def, // Type's body
                                                script);   // Set the Type the package
        
        Context* ctx = Context::Create(eng, eng->Context_Type);
        
        eng->GetGC()->ForceToGray(script);
        script->Initialize(cs->literals, ctx, entryPoint, 0);
    } // GC Pause    
    return script;
}

void Script::MarkRefs(Collector* c)
{
    ThisSuper::MarkRefs(c);
    
    if (entryPoint) entryPoint->Mark(c);
    if (literals) literals->Mark(c);
    if (context) context->Mark(c);
    if (arguments) arguments->Mark(c);
    if (import_value) import_value->Mark(c);
    if (file_name) file_name->Mark(c);
}

bool Script::Run(Array* args)
{
    if (running || firstRun) return false;
    if (!context || !entryPoint) return false;
    
    if (entryPoint->def->isGenerator)
    {
        RaiseException("Script's __main function cannot be a generator.");
    }
    
    {   GCPAUSE(engine); // pause gc
        const char* args_str = "__arguments";
        if (args)
        {
            arguments = args; // WriteBarrier unnessary bc/ of SetSlot
            SetSlot(args_str, arguments, Slot::ATTR_protected);
        }
        else
        {
            Value nval(NULL_VALUE);
            SetSlot(args_str, nval, Slot::ATTR_protected);
        }        
        SetSlot("__script",  this,       Slot::ATTR_internal | Slot::ATTR_forcewrite);
        SetSlot("__context", context,    Slot::ATTR_internal | Slot::ATTR_forcewrite);
        SetSlot("__main",    entryPoint, Slot::ATTR_internal | Slot::ATTR_forcewrite);
        SetSlot("__FILE",    file_name ? file_name : engine->emptyString, Slot::ATTR_internal | Slot::ATTR_forcewrite);
    }// resume gc
    
    context->PushNull();
    context->Push(entryPoint);
#   if defined(PIKA_DEBUG_OUTPUT)
        Timer t;
#   endif            
    if (context->SetupCall(0)) {
#   if defined(PIKA_DEBUG_OUTPUT)
        t.Start();
#   endif
        running = true;    
        context->Run();
    }
    
    running = false;
#if defined(PIKA_DEBUG_OUTPUT)
    t.End();
    puts("\n******************");
    printf("\t%g", t.Val());
    puts("\n******************");
#endif
    
    engine->PutImport(name, Value(this->GetImportResult()));    
    firstRun = true;    
    context->ClearAcc();
    engine->GetGC()->IncrementalRun();
    
    return context->GetState() == Context::DEAD || context->GetState() == Context::SUSPENDED;
}

static int Script_createWith(Context* ctx, Value& self)
{
    u2 argc = ctx->GetArgCount();    
    String* buff = 0;
    String* name = 0;
    Package* pkg= 0;
    
    switch (argc)
    {
        case 3: pkg = ctx->GetArgT<Package>(2);
        case 2: name = ctx->GetStringArg(1);
        case 1: buff = ctx->GetStringArg(0);          
            break;
        default:
            ctx->WrongArgCount();
            break;
    }
    Script* s = Script::CreateWithBuffer(ctx->GetEngine(), buff, name, pkg);
    if (s)
    {
        ctx->Push(s);
        return 1;
    }
    return 0;
}

Script* Script::Create(Engine* eng, String* name, Package* pkg)
{
    Script* s = 0;
    PIKA_NEW(Script, s, (eng, eng->Script_Type, name, pkg));
    eng->AddToGC(s);
    return s;
}

void Script::SetImportResult(Package* impres)
{
    if (impres)
    {
        WriteBarrier(impres);
        import_value = impres;
    }
}

void Script::Constructor(Engine* eng, Type* type, Value& res)
{
    Script* s = Script::Create(eng, eng->emptyString, eng->GetWorld());
    res.Set(s);
}

namespace {

PIKA_DOC(Script_run, "/([args])\
\n\
Runs the script with the optional arguments |args|. The return value will be a \
'''true''' or '''false''' based on the success of the operation.")

int Script_run(Context* ctx, Value& self)
{
    Script* s = (Script*)self.val.object;
    Array* args = 0;
    if (ctx->GetArgCount() == 1) {
        if (!ctx->GetArg(0).IsNull())
            args = ctx->GetArgT<Array>(0);
    } else if (ctx->GetArgCount() != 0) {
        ctx->WrongArgCount();
    }
    bool res = s->Run(args);
    if (res)
        ctx->PushTrue();
    else
        ctx->PushFalse();
    return 1;
}

PIKA_DOC(Script_fromBuffer, "/(buff)\
\n\
Creates a script form the string buffer |buff| given. The string should \
contain the text of the script being created. After creation you should \
execute the script with [run Script.run].\
")

PIKA_DOC(Script_export, "/(pkg)\
\n\
Sets the package, |pkg|, as the exported value of this script. This is the \
value returned from the function [import] which would normally be this script instance.\
[[[\
class A\n\
end\n\
\n\
__package.export(A)\
]]]\
")

PIKA_DOC(Script_class, "Scripts are the result of an [import imported] Pika \
file. Since Script is derived from Package they serve as the global scope for \
the script. Imported scripts are located in the [imports] package along with \
other scripts however thier parent package is [world].")

}// namespace

void Script::StaticInitType(Engine* eng)
{
    eng->Script_Type = Type::Create(eng,
                                    eng->AllocString("Script"),
                                    eng->Package_Type,
                                    Script::Constructor, eng->GetWorld());
    SlotBinder<Script>(eng, eng->Script_Type)
    .Method(&Script::SetImportResult, "export", PIKA_GET_DOC(Script_export))
    ;
    
    static RegisterFunction ScriptMethods[] =
    {
        { "run", Script_run, 0, DEF_VAR_ARGS, PIKA_GET_DOC(Script_run) },
    };
    
    static RegisterFunction Script_ClassMethods[] =
    {
        { "fromBuffer", Script_createWith, 3, DEF_VAR_ARGS, PIKA_GET_DOC(Script_fromBuffer) },
    };
    
    eng->Script_Type->EnterMethods(ScriptMethods, countof(ScriptMethods));
    eng->Script_Type->EnterClassMethods(Script_ClassMethods, countof(Script_ClassMethods));
    eng->Script_Type->SetDoc(PIKA_GET_DOC(Script_class));
    eng->GetWorld()->SetSlot("Script", eng->Script_Type);
}

}// pika
