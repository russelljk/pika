/*
 *  PZipFile.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PZipFile.h"

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

extern void Init_ZipReader(Engine*, Package*);
extern void Init_ZipWriter(Engine*, Package*);

PIKA_MODULE(zipfile, eng, zipfile)
{
    GCPAUSE(eng);
    Init_ZipReader(eng, zipfile);
    Init_ZipWriter(eng, zipfile);
    return zipfile;
}
