/*
 *  PMemory.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_MEMORY_HEADER
#define PIKA_MEMORY_HEADER

#include "PConfig.h"
#include <new> // placement new
#include <memory>

PIKA_API void* Pika_malloc(size_t sz);
PIKA_API void* Pika_realloc(void* v, size_t sz);
PIKA_API void  Pika_free(void* v); // passing a null pointer is OK
PIKA_API char* Pika_strcpy(char* dest, const char* src);
PIKA_API char* Pika_strdup(const char* s,  size_t* len /*out*/ = 0);
PIKA_API char* Pika_TransformString(const char* s, size_t lenin, size_t* len /*out*/);

PIKA_API void* Pika_memmove(void* dest, const void* src, size_t sz);
PIKA_API void* Pika_memcpy(void* dest, const void* src, size_t sz);

PIKA_API void  Pika_memzero(void* dest, size_t sz);
PIKA_API void* Pika_calloc(size_t count, size_t sz);

template<typename T> INLINE T*   Pika_construct(void* pos) { T* t = new (pos) T; return t; }
template<typename T> INLINE void Pika_destruct(T* t)       { if (t) t->~T(); }
template<typename T> INLINE T*   Pika_new()                { void* v = Pika_malloc(sizeof(T)); return Pika_construct<T>(v); }
template<typename T> INLINE void Pika_delete(T* t)         { Pika_destruct<T>(t); Pika_free((void*)t); }

#define PIKA_NEW(T, p, args)                    \
do                                              \
{                                               \
    void* ptrvoid##p_ = Pika_malloc(sizeof(T)); \
    p = new (ptrvoid##p_) T args;               \
}                                               \
while (false)

#define GCNEW(eng, T, p, args)                  \
do                                              \
{                                               \
    void* ptrvoid##p_ = Pika_malloc(sizeof(T)); \
    p = new (ptrvoid##p_) T args;               \
    eng->AddToGC(p);                            \
}                                               \
while (false)

#endif
