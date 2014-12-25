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

namespace pika {
    PIKA_IMPL(JsonError)
    PIKA_IMPL(JsonEncodeError)
    PIKA_IMPL(JsonDecodeError)
    PIKA_IMPL(JsonSyntaxError)
}// pika

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
    GCPAUSE(eng);
    
    static RegisterFunction json_Functions[] = {
        { "decode",  json_decode,  1, DEF_STRICT,   0 },    
        { "encode",  json_encode,  1, DEF_STRICT,   0 },
    };
    String* JsonError_String = eng->GetString("JsonError");
    Type* JsonError_Type = Type::Create(eng, JsonError_String, eng->RuntimeError_Type, 0, json);
    
    String* JsonEncodeError_String = eng->GetString("JsonEncodeError");
    Type* JsonEncodeError_Type = Type::Create(eng, JsonEncodeError_String, JsonError_Type, 0, json);
    
    String* JsonDecodeError_String = eng->GetString("JsonDecodeError");
    Type* JsonDecodeError_Type = Type::Create(eng, JsonDecodeError_String, JsonError_Type, 0, json);
    
    String* JsonSyntaxError_String = eng->GetString("JsonSyntaxError");
    Type* JsonSyntaxError_Type = Type::Create(eng, JsonSyntaxError_String, JsonDecodeError_Type, 0, json);
    
    json->SetSlot(JsonError_String,         JsonError_Type);
    json->SetSlot(JsonEncodeError_String,   JsonEncodeError_Type);
    json->SetSlot(JsonDecodeError_String,   JsonDecodeError_Type);
    json->SetSlot(JsonSyntaxError_String,   JsonSyntaxError_Type);
        
    eng->SetTypeFor(JsonError::StaticGetClass(), JsonError_Type);
    eng->SetTypeFor(JsonEncodeError::StaticGetClass(), JsonEncodeError_Type);
    eng->SetTypeFor(JsonDecodeError::StaticGetClass(), JsonDecodeError_Type);
    eng->SetTypeFor(JsonSyntaxError::StaticGetClass(), JsonSyntaxError_Type);
        
    json->EnterFunctions(json_Functions, countof(json_Functions));
    return json;
}
