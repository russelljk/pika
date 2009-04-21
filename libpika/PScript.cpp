/*
 *  PScript.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"

#define PIKA_RUNPROFILE

#if defined(PIKA_RUNPROFILE)
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
        firstRun(false),
        running(false)
{}

Script::~Script()
{}

void Script::Initialize(LiteralPool* lp, Context* ctx, Function* entry)
{
    ASSERT(lp);
    ASSERT(ctx);
    ASSERT(entry);
    
    literals   = lp;
    context    = ctx;
    entryPoint = entry;
    
    WriteBarrier(literals);
    WriteBarrier(context);
    WriteBarrier(entryPoint);
}

void Script::MarkRefs(Collector* c)
{
    ThisSuper::MarkRefs(c);
    
    if (entryPoint) entryPoint->Mark(c);
    if (literals) literals->Mark(c);
    if (context) context->Mark(c);
    if (arguments) arguments->Mark(c);
}

bool Script::Run(Array* args)
{
    if (running || firstRun) return false;
    if (!context || !entryPoint) return false;
    if (context->IsInvalid()) return false;
    {
        GCPAUSE(engine); // pause gc
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
    }// resume gc
    
    context->PushNull();
    context->Push(entryPoint);
    context->SetupCall(0);
#if defined(PIKA_RUNPROFILE)
    Timer t;
    t.Start();
#endif
    running = true;
    
    context->Run();
    
    running = false;
#if defined(PIKA_RUNPROFILE)
    t.End();
    puts("\n******************");
    printf("\t%g", t.Val());
    puts("\n******************");
#endif
    engine->PutImport(name, Value(this));
    
    firstRun = true;
    
    engine->GetGC()->IncrementalRun();
    
    return context->GetState() == Context::DEAD || context->GetState() == Context::SUSPENDED;
}

Script* Script::Create(Engine* eng, String* name, Package* pkg)
{
    Script* s = 0;
    PIKA_NEW(Script, s, (eng, eng->Script_Type, name, pkg));
    eng->AddToGC(s);
    return s;
}

}// pika

static void Script_newFn(Engine* eng, Type* type, Value& res)
{
    Script* s = Script::Create(eng, eng->emptyString, eng->GetWorld());
    res.Set(s);
}

void InitScriptAPI(Engine* eng)
{
    eng->Script_Type = Type::Create(eng,
                                    eng->AllocString("Script"),
                                    eng->Package_Type,
                                    Script_newFn, eng->GetWorld());
}
