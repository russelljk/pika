#include "PBigNum.h"

using namespace pika;
extern void Initialize_BigInteger(Package*, Engine*);

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



/*
    For rhs comparison operators we can just use the inverse
    
    opLt_r => MAKE_COMP_REVERSE(opLt_r, >=)
*/
PIKA_MODULE(bignum, eng, bignum)
{
    Initialize_BigInteger(bignum, eng);
    return bignum;
}
