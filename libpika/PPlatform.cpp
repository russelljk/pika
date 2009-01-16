/*
 *  PPlatform.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PPlatform.h"

int Pika_snprintf(char* buff, size_t count, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int res = Pika_vsnprintf(buff, count, fmt, args);
    va_end(args);
    return res;
}

size_t Pika_strhash(const char* str)
{
    size_t hash = 5381;
    int c;

    while ( (c = *str++) )
    {
        hash = ((hash << 5) + hash) + StrGetChar(c);
    }
    return hash;
}

size_t Pika_strhash(const char* str, size_t len)
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
extern char* Pika_index(const char* str, int idx)
{
    return index(str, idx);
}
#else
extern char* Pika_index(const char* str, int idx)
{
    return strchr(str, idx);
}
#endif

#if defined(HAVE_RINDEX)
extern char* Pika_rindex(const char* str, int idx)
{
    return rindex(str, idx);
}
#else
extern char* Pika_rindex(const char* str, int idx)
{
    return strrchr(str, idx);
}
#endif

/*
#include <dl.h> 

shl_t shl_load(const char *path, int flags, long address); 
int shl_findsym( 
     shl_t *handle, 
     const char *sym, 
     short type, 
     void *value 
); 
int shl_definesym( 
     const char *sym, 
     short type, 
     long value, 
     int flags 
); 
int shl_getsymbols( 
     shl_t handle, 
     short type, 
     int flags, 
     void *(*memory) (), 
     struct shl_symbol **symbols, 
); 
int shl_unload(shl_t handle); 
int shl_get(int index, struct shl_descriptor **desc); 
int shl_gethandle(shl_t handle, struct shl_descriptor **desc); 
int shl_get_r(int index, struct shl_descriptor *desc); 
int shl_gethandle_r(shl_t handle, struct shl_descriptor *desc);
*/

