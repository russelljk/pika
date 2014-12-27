/*
 *  BZip2.cpp
 *  See Copyright Notice in Pika.h
 */
#include "BZip2.h"

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

PIKA_MODULE(bzip2, eng, bzip2)
{
    GCPAUSE(eng);
    String* BZStream_String = eng->AllocString("BZStream");
    Type*   BZStream_Type   = Type::Create(eng, BZStream_String, eng->Object_Type, 0, bzip2);
    BZStream_Type->SetAbstract(true);
    
    String* Compressor_String = eng->AllocString("Compressor");
    Type*   Compressor_Type   = Type::Create(eng, Compressor_String, BZStream_Type, Compressor::Constructor, bzip2);
    bzip2->SetSlot(Compressor_String, Compressor_Type);
    
    String* Decompressor_String = eng->AllocString("Decompressor");
    Type*   Decompressor_Type   = Type::Create(eng, Decompressor_String, BZStream_Type, Decompressor::Constructor, bzip2);
    bzip2->SetSlot(Decompressor_String, Decompressor_Type);
    
    SlotBinder<BZStream>(eng, BZStream_Type)
    .Method(&BZStream::Process, "process")
    ;
    
    SlotBinder<Compressor>(eng, Compressor_Type)
    .PropertyRW("level",
        &Compressor::GetLevel, "getLevel",
        &Compressor::SetLevel, "setLevel")
    ;
        
    String* BZError_String = eng->GetString("BZError");
    Type*   BZError_Type   = Type::Create(eng, BZError_String, eng->RuntimeError_Type, 0, bzip2);
    
    String* CompressError_String = eng->GetString("CompressError");
    Type*   CompressError_Type   = Type::Create(eng, CompressError_String, BZError_Type, 0, bzip2);
    
    String* DecompressError_String = eng->GetString("DecompressError");
    Type*   DecompressError_Type   = Type::Create(eng, DecompressError_String, BZError_Type, 0, bzip2);
    
    bzip2->SetSlot(        BZError_String,         BZError_Type);
    bzip2->SetSlot(  CompressError_String,   CompressError_Type);
    bzip2->SetSlot(DecompressError_String, DecompressError_Type);
    
    eng->SetTypeFor(        BZError::StaticGetClass(),         BZError_Type);
    eng->SetTypeFor(  CompressError::StaticGetClass(),   CompressError_Type);
    eng->SetTypeFor(DecompressError::StaticGetClass(), DecompressError_Type);
    
    //eng->AddBaseType(Compressor_String, Compressor_Type);
    //eng->AddBaseType(Decompressor_String, Decompressor_Type);
    return bzip2;
}
