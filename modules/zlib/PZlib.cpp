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
    String* Deflater_String = eng->AllocString("Deflater");
    Type*   Deflater_Type   = Type::Create(eng, Deflater_String, eng->Object_Type, Deflater::Constructor, zlib);
    zlib->SetSlot(Deflater_String, Deflater_Type);
    
    String* Inflater_String = eng->AllocString("Inflater");
    Type*   Inflater_Type   = Type::Create(eng, Inflater_String, eng->Object_Type, Inflater::Constructor, zlib);
    zlib->SetSlot(Inflater_String, Inflater_Type);
    
    //eng->AddBaseType(Deflater_String, Deflater_Type);
    //eng->AddBaseType(Inflater_String, Inflater_Type);
    return zlib;
}
