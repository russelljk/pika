/*
 *  PlatRE.cpp
 *  See Copyright Notice in PRegExp.h
 */
#include "Pika.h"
#include "PlatRE.h"
#include "re_config.h"

#if defined(HAVE_PCRE)

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                          PCRE                                                 //
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <pcre.h>

int const RE_NO_UTF8_CHECK  = PCRE_NO_UTF8_CHECK;
int const RE_UTF8           = PCRE_UTF8;
int const RE_MULTILINE      = PCRE_MULTILINE;
int const RE_INSENSITIVE    = PCRE_CASELESS;

#define OVECTOR_SIZE (128*3)

Pika_regex* Pika_regcomp(const char* pattern, int cflags, char* errmsg, size_t errlen, int* errorcode)
{
    int err = 0;
    const char* errdescr = 0;
    int erroff = 0;
    int options = PCRE_UTF8 | PCRE_NO_UTF8_CHECK | PCRE_MULTILINE | cflags;
    pcre* re = pcre_compile2(pattern, options, &err, &errdescr, &erroff, 0);
    
    if (!re && errmsg && errlen > 0)
        strncpy(errmsg, errdescr, pika::Min(errlen, strlen(errdescr)));    
    if (errorcode)
        *errorcode = err;
    return (Pika_regex*)re;
}

size_t Pika_regerror(int errcode, const Pika_regex* preg, char* errbuf, size_t errbuf_size) { return 0; }

void Pika_regfree(Pika_regex* re)
{
    pcre_free((pcre*)re);
}

int Pika_regexec(const Pika_regex* re, const char* subj, size_t const subjlen, Pika_regmatch* pmatch, size_t const nmatch, int eflags)
{
    //int* ovector = (int*)alloca(sizeof(int) * nmatch * 1.5);
    int ovector[OVECTOR_SIZE];
    int match = pcre_exec((pcre*)re, 0, subj, subjlen,
                          0,
                          PCRE_NO_UTF8_CHECK,
                          ovector,
                          OVECTOR_SIZE);
    for (int i = 0; i < pika::Min<int>(nmatch, match); ++i)
    {
        int so = ovector[i * 2];
        int eo = ovector[i * 2 + 1];
        pmatch[i].start = so >= 0 ? so : 0;
        pmatch[i].end   = eo >= 0 ? eo : 0;
    }
    return match;
}

#elif defined(HAVE_POSIX_RE)

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                          POSIX                                                //
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <regex.h>

int const RE_NO_UTF8_CHECK  = 0;
int const RE_UTF8           = 0;
int const RE_MULTILINE      = 0;
int const RE_INSENSITIVE    = 0;

Pika_regex* Pika_regnew()
{
    Pika_regex* re = (Pika_regex*)Pika_calloc(1, sizeof(regex_t));
    return re;
}

Pika_regex* Pika_regcomp(const char* pattern, int cflags, char* errmsg, size_t errlen, int* errorcode)
{
    Pika_regex* preg = Pika_regnew();
    int err = regcomp((regex_t*)preg, pattern, cflags);
    if (err != 0)
    {
        regerror(err, (regex_t*)preg, errmsg, errlen);
        if (errorcode)
            *errorcode = err;
        Pika_regfree(preg);
        return 0;
    }
    return preg;
}

size_t Pika_regerror(int errcode, const Pika_regex* preg, char* errbuf, size_t errbuf_size)
{
    return regerror(errcode, (regex_t*)preg, errbuf, errbuf_size);
}

int Pika_regexec(const Pika_regex* preg, const char* str, size_t const subjlen, Pika_regmatch* pmatch, size_t const nmatch, int eflags)
{
    regmatch_t* rm = (regmatch_t*)alloca(sizeof(regmatch_t) * nmatch);
    if (!rm)
    {
        return -1; // TODO: we need to create and convert to our own regex errors.
    }
    Pika_memzero(rm, sizeof(regmatch_t) * nmatch);
    int res = regexec((regex_t*)preg, str, nmatch, rm, eflags);
    int matchesmade = 0;
    if (res != 0)
        return 0;
    
    for (size_t i = 0; i < nmatch; ++i)
    {
        regoff_t so = rm[i].rm_so;
        regoff_t eo = rm[i].rm_eo;
        if (so == -1 || eo == -1)
        {
            matchesmade = i;
            break;
        }
        pmatch[i].start = so;
        pmatch[i].end   = eo;
    }
    return matchesmade;// was res
}

void Pika_regfree(Pika_regex* preg)
{
    regfree((regex_t*)preg);
    Pika_free(preg);
}

#else

int const RE_NO_UTF8_CHECK  = -1;
int const RE_UTF8           = -1;
int const RE_MULTILINE      = -1;

Pika_regex* Pika_regcomp(const char*, int, char*, size_t, int*) { return 0; }

size_t Pika_regerror(int, const Pika_regex*, char*, size_t) { return 0; }

void Pika_regfree(Pika_regex*) {}

int Pika_regexec(const Pika_regex*, const char*, size_t const, Pika_regmatch*, size_t const, int) { return 0; }

#endif

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
