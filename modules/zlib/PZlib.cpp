/*
 *  PZlib.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PZlib.h"

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

PIKA_MODULE(zlib, eng, zlib)
{
    GCPAUSE(eng);
    /*
        Class Hierarchy:
        
        (Object) +--->  ZStream  +---> Compressor
                 |               |
                 |               +---> Decompressor
                 |
                 +--->  (Error) +---> (RuntimeError) --->  ZlibError  +---> CompressError
                                                                      |
                                                                      +---> DecompressError
    */
    String* ZStream_String = eng->AllocString("ZStream");
    Type*   ZStream_Type   = Type::Create(eng, ZStream_String, eng->Object_Type, 0, zlib);
    ZStream_Type->SetAbstract(true);
    
    String* Compressor_String = eng->AllocString("Compressor");
    Type*   Compressor_Type   = Type::Create(eng, Compressor_String, ZStream_Type, Compressor::Constructor, zlib);
    zlib->SetSlot(Compressor_String, Compressor_Type);
    
    String* Decompressor_String = eng->AllocString("Decompressor");
    Type*   Decompressor_Type   = Type::Create(eng, Decompressor_String, ZStream_Type, Decompressor::Constructor, zlib);
    zlib->SetSlot(Decompressor_String, Decompressor_Type);
    
    SlotBinder<ZStream>(eng, ZStream_Type)
    .Method(&ZStream::Process, "process")
    ;
    
    SlotBinder<Compressor>(eng, Compressor_Type)
    .PropertyRW("level",
        &Compressor::GetLevel, "getLevel",
        &Compressor::SetLevel, "setLevel")
    ;
    
    static NamedConstant Zlib_Constants[] = {
        { "Z_NO_COMPRESSION",   Z_NO_COMPRESSION },
        { "Z_BEST_SPEED",       Z_BEST_SPEED },
        { "Z_BEST_COMPRESSION", Z_DEFAULT_COMPRESSION },
    };
    
    Basic::EnterConstants(zlib, Zlib_Constants, countof(Zlib_Constants));
    
    String* ZlibError_String = eng->GetString("ZlibError");
    Type*   ZlibError_Type   = Type::Create(eng, ZlibError_String, eng->RuntimeError_Type, 0, zlib);
    
    String* CompressError_String = eng->GetString("CompressError");
    Type*   CompressError_Type   = Type::Create(eng, CompressError_String, ZlibError_Type, 0, zlib);
    
    String* DecompressError_String = eng->GetString("DecompressError");
    Type*   DecompressError_Type   = Type::Create(eng, DecompressError_String, ZlibError_Type, 0, zlib);
    
    zlib->SetSlot(       ZlibError_String,         ZlibError_Type);
    zlib->SetSlot(CompressError_String,  CompressError_Type);
    zlib->SetSlot(DecompressError_String,  DecompressError_Type);
    
    eng->SetTypeFor(       ZlibError::StaticGetClass(),        ZlibError_Type);
    eng->SetTypeFor(CompressError::StaticGetClass(), CompressError_Type);
    eng->SetTypeFor(DecompressError::StaticGetClass(), DecompressError_Type);
    
    //eng->AddBaseType(Compressor_String, Compressor_Type);
    //eng->AddBaseType(Decompressor_String, Decompressor_Type);
    return zlib;
}
