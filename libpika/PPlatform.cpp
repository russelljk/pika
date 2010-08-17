/*
 *  PPlatform.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PPlatform.h"

int Pika_StringCompare(const char* a, size_t lena, const char* b, size_t lenb)
{
    const char* stra = a;
    const char* strb = b;
    int res  = 0;
    size_t len  = 0;
    
    while ((res = StrCmp(stra, strb)) == 0)
    {
        len += strlen(stra);
        
        if (len == lena)
            return 0;
            
        stra = a + len + 1;
        strb = b + len + 1;
        len++;
    }
    return res;
}

int Pika_snprintf(char* buff, size_t count, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int res = Pika_vsnprintf(buff, count, fmt, args);
    va_end(args);
    return res;
}

size_t Pika_StringHash(const char* str)
{
    size_t hash = 5381;
    int c;

    while ( (c = *str++) )
    {
        hash = ((hash << 5) + hash) + StrGetChar(c);
    }
    return hash;
}

size_t Pika_StringHash(const char* str, size_t len)
{
    size_t hash = 5381;
    int c;

    const char* str_end = str + len;

    while (str < str_end)
    {
        c = *str++;
        hash = ((hash << 5) + hash) + StrGetChar(c);
    }
    return hash;
}

#if defined(HAVE_STRTOK_R)
char* Pika_strtok(char* str, const char* sep, char** last)
{
    return strtok_r(str, sep, last);
}
#elif defined(HAVE_STRTOK_S)
char* Pika_strtok(char* str, const char* sep, char** last)
{
    return strtok_s(str, sep, last);
}
#elif defined(HAVE_STRTOK)
char* Pika_strtok(char* str, const char* sep, char**)
{
    return strtok(str, sep);
}
#else
#   error no strtok function provided
#endif


#if defined(HAVE_INDEX)
const char* Pika_index(const char* str, int idx)
{
    return index(str, idx);
}
#else
const char* Pika_index(const char* str, int idx)
{
    return strchr(str, idx);
}
#endif

#if defined(HAVE_RINDEX)
const char* Pika_rindex(const char* str, int idx)
{
    return rindex(str, idx);
}
#else
const char* Pika_rindex(const char* str, int idx)
{
    return strrchr(str, idx);
}
#endif
