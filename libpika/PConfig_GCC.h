/*
 *  PConfig_GCC.h
 *  See Copyright Notice in Pika.h
 */
#define PIKA_GNUC   // GNU Compiler Collection

// Determine the platform that we are using GCC on.

// Windows
#if defined(_WIN32)
#   define PIKA_WIN

// Mac OS X
#elif defined(__MACH__) && defined(__APPLE__)
#   define PIKA_MAC
#   define PIKA_POSIX

// Linux (or another *nix not including OS X)
#elif defined(unix) || defined(__unix__) || defined(__unix) || defined(linux) || defined(__linux__) || defined(__linux)
#   define PIKA_NIX
#   define PIKA_POSIX

// ???
#else
#   error unknown platform
#endif

/* Determine the byteorder. */
#if defined(__BIG_ENDIAN__)
#   define PIKA_BIG_ENDIAN
#endif

/* Include files that we need when using GCC. */

/* Typedefs for 1, 2, 4 and 8 byte signed and unsigned integers. */
#if defined(HAVE_STDINT_H)
#include <stdint.h>
typedef uint8_t             u1;
typedef uint16_t            u2;
typedef uint32_t            u4;
typedef uint64_t            u8;

typedef int8_t              s1;
typedef int16_t             s2;
typedef int32_t             s4;
typedef int64_t             s8;
#else
typedef unsigned char       u1;
typedef unsigned short      u2;
typedef unsigned int        u4;
typedef unsigned long long  u8;

typedef signed   char       s1;
typedef signed   short      s2;
typedef signed   int        s4;
typedef signed   long long  s8;
#endif

#define PIKA_FORCE_INLINE   __inline__

/* Are we using a 64bit arch. */
#if defined(__amd64) || defined(__x86_64__)
#   define PIKA_64
#   define PIKA_64BIT_REAL /* If we are using the x86-64 arch we might as well use the 64 integer and real types. */
#   define PIKA_64BIT_INT
#elif defined(PAE)
#   define PIKA_64
#elif defined(__LP64__) || defined(_LP64) // 64bit pointer
#   ifndef PIKA_64
#       define PIKA_64
#   endif
#else
#   define PIKA_32
#endif

#define PIKA_ALIGN          8

/* We need exceptions enabled. */
#ifndef __EXCEPTIONS
#   error Pika requires C++ exceptions enabled.
#endif

#define PIKA_STRUCT_ALIGN(n, name)  struct __attribute__((aligned(n))) name
#define PIKA_CLASS_ALIGN(n, name)   class  __attribute__((aligned(n))) name

#define Pika_strcasecmp     strcasecmp
#if defined(PIKA_MAC)
#   define Pika_isnan       std::isnan
#else
#   define Pika_isnan       isnan
#endif
#define Pika_vsnprintf      vsnprintf

#if defined(PIKA_MAC)
#   include <sys/param.h>
#   define PIKA_MAX_PATH        MAXPATHLEN
#else
#   ifndef PATH_MAX
#       define PATH_MAX         1024
#   endif
#   define PIKA_MAX_PATH        PATH_MAX
#endif

#define PIKA_INT_64BIT_FMT      "%lld"
#define PIKA_INT_32BIT_FMT      "%d"
#define SIZE_T_FMT              "%zu"
#define POINTER_FMT             "%p"
#define PIKA_ABS32              abs
#define PIKA_ABS64              llabs

#define PIKA_MODULE_EXPORT  extern "C"
