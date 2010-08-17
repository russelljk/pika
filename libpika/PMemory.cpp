/*
 *  PMemory.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PMemory.h"

#if defined(HAVE_MEMORY_H)
#   include <memory.h>
#endif

#if defined (HAVE_MALLOC_H)
#   include <malloc.h>
#endif

#include "Pika.h" // remove me if Pika_TransformString is moved.

void* Pika_malloc (size_t sz)                              { return malloc(sz); }
void* Pika_realloc(void* v, size_t sz)                     { return realloc(v, sz); }
void  Pika_free   (void* v)                                { if (v) free(v); }
void* Pika_memmove(void* dest, const void* src, size_t sz) { return memmove(dest, src, sz); }

#if defined(HAVE_MEMCPY)
void* Pika_memcpy(void* dest, const void* src, size_t sz) { return memcpy(dest, src, sz); }
#elif defined(HAVE_BCOPY)
void* Pika_memcpy(void* dest, const void* src, size_t sz) { bcopy(src, dest, sz); return dest; }
#else
#   error Pika_memcpy: memcpy or bcopy not provided
#endif

#if defined(HAVE_BZERO)
void Pika_memzero(void* dest, size_t sz) { bzero(dest, sz); }
#else
void Pika_memzero(void* dest, size_t sz) { memset(dest, 0, sz); }
#endif

#if defined(HAVE_STRCPY)
char* Pika_strcpy(char* dest, const char* src) { return strcpy(dest, src); }
#else
char* Pika_strcpy(char* dest, const char* src) { return (char*)Pika_memcpy (dest, src, 1 + strlen (src)); }
#endif /* HAVE_STRCPY */

void* Pika_calloc(size_t count, size_t sz) { return calloc(count, sz); }

char* Pika_strdup(const char* s, size_t* lenOut)
{
    if (!s) return 0;
    
    size_t len = strlen(s);
    char * ret = (char*)Pika_malloc((len + 1) * sizeof(char));
    Pika_strcpy(ret, s);
    ret[len] = '\0';
    
    if (lenOut) 
    {
        *lenOut = len;
    }    
    return ret;
}

char* Pika_TransformString(const char* s, size_t lenin, size_t* lenout)
{
    size_t const EXTRA = 32;
    if (!s)
    {
        return 0;
    }
    size_t len = strlen(s);
    Buffer<char> ret;
    ret.SetCapacity( (len + EXTRA) );
    
    const char* slim = s + lenin;
    
    while (s < slim)
    {
        // Handle Escape Sequences.
        if (*s == '\\')
        {
            *s++;
            if (isdigit(*s))
            {
                // Convert decimal number to a single character.                
                u4 x = 0;
                for (int i = 0; (i < 3) && (*s) && (isdigit(*s)); ++i)
                {              
                    x = x * 10 + (*s - '0');
                    ++s;
                }
                ret.Push( (char)x );
            }
            else if (*s == 'x' || *s == 'X')
            {
                // Convert hex number to a single character.
                ++s;
                u4 x = 0;
                
                for (int i = 0 ; i < 2 ; ++i)
                {
                    char ch = tolower(*s);
                    if (!ch) break;
                    
                    if (isdigit(ch))
                    {
                        x = x * 16 + (ch - '0');
                    }
                    else if (ch >= 'a' && ch <= 'f')
                    {
                        x = x * 16 + (ch - 'a' + 10);
                    }
                    else
                    {
                        break;
                    }
                    ++s;
                }
                ret.Push( (char)x );
            }
            else if (*s == '\r')
            {
                // Skip the CR|LF newline.                
                ++s;
                if (*s == '\n')
                {
                    ++s;
                }
            }
            else
            {
                int next = *s++;
                switch (next)
                {
                case 'a'  : ret.Push('\a'); break;
                case 'b'  : ret.Push('\b'); break;
                case 'f'  : ret.Push('\f'); break;
                case 'n'  : ret.Push('\n'); break;
                case 'r'  : ret.Push('\r'); break;
                case 's'  : ret.Push(' ');  break;
                case 't'  : ret.Push('\t'); break;
                case 'v'  : ret.Push('\v'); break;
                case '\'' : ret.Push('\''); break;
                case '\"' : ret.Push('\"'); break;
                case '\\' : ret.Push('\\'); break;                
                default: 
                {
                ret.Push('\\');
                ret.Push((char)next);
                }
                break;
                };
            }
        }
        else if (*s == '\r')
        {
            // Convert CR|LF into LF.
            ++s;
            if (*s == '\n')
            {
                ++s;
            }
            ret.Push('\n');
        }
        else
        {
            ret.Push(*s++);
        }
    }
    ret.Push('\0'); // <- Strings need a terminating null character, so they will play nice with the C string functions.
    
    size_t outsize = ret.GetSize();    
    char* outstr = (char*)Pika_malloc(outsize * sizeof(char));     
    Pika_memcpy(outstr, ret.GetAt(0), outsize * sizeof(char));
    
    if (lenout)
    {
        *lenout = outsize - 1; // <- The string length does not include the terminating null.
    }
    return outstr;
}
