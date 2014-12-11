/*
 *  PJson.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PJson.h"
#include "PJsonDecode.h"
#include "PJsonEncode.h"

#if defined(PIKA_WIN)
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH: break;
    }
    return TRUE;
}

#endif

using namespace pika;

int json_decode(Context* ctx, Value&)
{
    Engine* engine = ctx->GetEngine();
    String* json = ctx->GetStringArg(0);
    GCPAUSE(engine);
    JsonTokenizer tokenizer(json);
    JsonParser parser(engine, &tokenizer);
    Value res = parser.Parse();
    ctx->Push(res);
    return 1;
}

int json_encode(Context* ctx, Value&)
{
    Engine* engine = ctx->GetEngine();
    Value& arg = ctx->GetArg(0);
    GCPAUSE(engine);
    JsonEncoder encoder(engine);
    String* res = encoder.Encode(arg);
    if (res) {
        ctx->Push(res);
        return 1;
    }
    return 0;
}

PIKA_MODULE(json, eng, json)
{
    static RegisterFunction json_Functions[] = {
        { "decode",  json_decode,  1, DEF_STRICT,   0 },    
        { "encode",  json_encode,  1, DEF_STRICT,   0 },
    };
    
    json->EnterFunctions(json_Functions, countof(json_Functions));
    return json;
}
