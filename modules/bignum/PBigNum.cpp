/*
 *  PBigNum.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PBigNum.h"

using namespace pika;
extern void Initialize_BigInteger(Package*, Engine*);
extern void Initialize_BigReal(Package*, Engine*);


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
    Initialize_BigReal(bignum, eng);
    /*
    TODO: Add the following package methods:
    cos
    sin
    tan
    acos
    asin
    atan
    cosh
    sinh
    tanh
    acosh
    asinh
    atanh
    hypot
    abs  => mpz_abs | mpfr_abs
    min (vararg)
    max (vararg)
    
    PI -> mpfr_const_pi
    
    log
    log2
    log10
    
    exp
    exp2
    exp10
    
    sqrt => mpz_sqrt | mpfr_sqrt | mpfr_sqrt_ui
    
    TODO: Error Checking
    */
    return bignum;
}
