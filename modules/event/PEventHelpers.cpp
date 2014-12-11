/*
 *  PEventHelpers.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PEventModule.h"

namespace pika {
void Pika_MakeTimeval(preal_t secs, timeval& tv)
{
    tv.tv_sec = (long)secs;
    tv.tv_usec = (long)((secs - tv.tv_sec) * 1000000.0);   
}

pint_t GetFilenoFrom(Context* ctx, Value vfile)
{
    Engine* engine = ctx->GetEngine();
    if (vfile.IsInteger())
    {
        return vfile.val.integer;
    }

    Value filenoMethod(NULL_VALUE);

    if (!engine->GetFieldFromValue(vfile, engine->GetString("fileno"), filenoMethod))
    {
        String* filetype = engine->GetTypenameOf(vfile);
        RaiseException(Exception::ERROR_type, 
            "Event.init argument 2 must be either an file descriptor or an Object implementing the method 'fileno'.");
    }
    
    ctx->CheckStackSpace(2);
    ctx->Push(vfile);
    ctx->Push(filenoMethod);

    if (ctx->SetupCall(0))
    {
        ctx->Run();
    }

    Value retValue = ctx->PopTop();

    if (!retValue.IsInteger())
    {
        String* filetype = engine->GetTypenameOf(vfile);
        RaiseException(Exception::ERROR_type, 
            "Excepted return type of Integer from %s.fileno.",
            filetype->GetBuffer());
    }

    return retValue.val.integer;
}
}// pika