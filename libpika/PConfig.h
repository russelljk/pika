/*
 *  PConfig.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_CONFIG_HEADER
#define PIKA_CONFIG_HEADER

////////////////////////////////////////////// HEADERS /////////////////////////////////////////////

#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cctype>
#include <cerrno>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cassert>
#include <ctime>
#include <cstring>
#include <iostream>
#include <fstream>
#include "pika_config.h"

#if defined(HAVE_STRINGS_H)
#   include <strings.h>
#endif

#define PIKA_STDLIB
#define INLINE             inline
#define ASSERT             assert
#define ENABLE_TRACE

/////////////////////////////////////////// OPTIONAL DEFS //////////////////////////////////////////

/* Use mem-pools for Slot creation.
   Sometimes it can be faster but it is not thread safe. */
/* #define PIKA_USE_SLOT_POOL */

/* Use doubles instead of floats for the type Real. */
#define PIKA_64BIT_REAL

/* Disable the calling of hooks. Speeds up execution but debugging becomes impossible. */
/* #define PIKA_NO_HOOKS */

/* Incorrect number of arguments passed to a function will cause an exception */
/* #define PIKA_STRICT_ARGC */

/* returns null instead of raising an exception if a global variable is missing. */
/* #define PIKA_ALLOW_MISSING_GLOBALS */

/* returns null instead of raising an exception if a slot is missing. */
/* #define PIKA_ALLOW_MISSING_SLOTS */

/* Disallow using memzero in performance critical areas. */
/* #define PIKA_NOMEMZERO_FOR_BLOCKS */

/* Check that outer variables are valid. */
/* #define PIKA_CHECK_LEX_ENV */

/* String and identifiers are case insensitive. like BASIC. */
/* #define PIKA_STRING_CASE_INSENSITIVE */

// Shared library configuration ////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)//         Visual Studio
#   ifdef PIKA_DLL//                DLL
#       ifdef PIKA_EXPORTS
#           define PIKA_API     __declspec(dllexport)
#       else
#           define PIKA_API     __declspec(dllimport)
#       endif
#   else//                          LIB
#       define PIKA_API
#   endif
#elif defined(__BORLANDC__)//   Borland
#   ifdef PIKA_DLL//                DLL
#       ifdef PIKA_EXPORTS
#           define PIKA_API     __declspec(dllexport)
#       else
#           define PIKA_API     __declspec(dllimport)
#       endif
#   else
#       define PIKA_API//           LIB
#   endif
#else//                         GCC (or unknown compiler)
#   define PIKA_API
#endif

// Compiler specific configuration /////////////////////////////////////////////////////////////////

#if defined(__GNUC__) 
#   include     "PConfig_GCC.h"
#elif defined(_MSC_VER)
#   include     "PConfig_VisualStudio.h"
#elif defined(__BORLANDC__)
#   include     "PConfig_Borland.h"
#else
#   include     "PConfig_Platform.h"
#endif

// File Ext & Path config //////////////////////////////////////////////////////////////////////////

#define PIKA_EXT                      ".pika"
#define PIKA_COMPILED_EXT             ".pikac"

#define PIKA_EXT_ALT                  ".p"
#define PIKA_COMPILED_EXT_ALT         ".pc"

#if defined(PIKA_WIN)
#   define PIKA_PATH_SEP_CHAR         '\\'
#   define PIKA_PATH_SEP              "\\"
#   define PIKA_LIB_EXT               ".dll"
#   define PIKA_LIB_PREFIX            "pika"
#   define PIKA_CASE_INSENSITIVE      true
#elif defined(PIKA_NIX)
#   define PIKA_PATH_SEP_CHAR         '/'
#   define PIKA_PATH_SEP              "/"
#   define PIKA_LIB_EXT               ".so"
#   define PIKA_LIB_PREFIX            "libpika"
#   define PIKA_CASE_INSENSITIVE      false
#elif defined(PIKA_MAC)
# define PIKA_PATH_SEP_CHAR           '/'
#   define PIKA_PATH_SEP              "/"
#   define PIKA_LIB_EXT               ".dylib"
#   define PIKA_LIB_PREFIX            "libpika"
#   define PIKA_CASE_INSENSITIVE      true
#endif

// Constants ///////////////////////////////////////////////////////////////////////////////////////
/*

PIKA_MODULE_EXPORT          Declaration needed to export a function from a shared library.

PIKA_ALIGN [default 8]      Don't lower this, but you can increase it to another power of 2 (or more) if needed.
                            This only effects mem pools and userdata alignment NOT standard C allocation methods
                            like malloc and realloc.

PIKA_PATH_SEP_CHAR          System path seperator char.
PIKA_PATH_SEP               System path seperator string.
PIKA_LIB_PREFIX             Prefix of user loaded shared libs. 
PIKA_LIB_EXT                System extension used for modules (shared libraries).

u1                          1 byte unsigned integer
u2                          2 byte unsigned integer
u4                          4 byte unsigned integer
u8                          8 byte unsigned integer

s1                          1 byte signed integer
s2                          2 byte signed integer
s4                          4 byte signed integer
s8                          8 byte signed integer

PIKA_32                     Defined iff pointers are 32bit < (mutually)
PIKA_64                     Defined iff pointers are 64bit < (exclusive)

PIKA_WIN                    Windows                 < 
PIKA_NIX                    Unix, Linux or BSD      < (mutually exclusive)
PIKA_MAC                    Mac OS X                < 

PIKA_MAX_PATH               Maximum length a system path can be.

*/
// VM limits ///////////////////////////////////////////////////////////////////////////////////////
/* 

Most of these values can be adjusted as needed. Do not do so unless you know what you are doing.
Keep in mind the range specified [min .. max ] and the limits of the data type used in storing them.

*/
#define PIKA_MAX_LITERALS           0x7FFF      // Maximum number of literals that can be indexed.     1 .. max( u2 ) - 1
#define PIKA_INIT_SCOPE_STACK       128         // Initial size of a scope stack.                      1 .. ]
#define PIKA_MAX_SCOPE_STACK        32768       // Maximum size a scope stack can grow to.             PIKA_INIT_SCOPE_STACK .. max(size_t)
#define PIKA_MAX_EXCEPTION_STACK    255         // Maximum size an exception stack can grow to.        1 .. sizeof(size_t) ]
#define PIKA_INIT_OPERAND_STACK     64          // Initial size of an operand stack.                   1 .. PIKA_MAX_OPERAND_STACK
#define PIKA_MAX_OPERAND_STACK      1048575     // Maximum size an operand stack can grow to.          PIKA_INIT_OPERAND_STACK .. max(size_t)
#define PIKA_STACK_GROWTH_RATE      1.5         // Growth Rate of an operand stack.                    must be > 1.0.
#define PIKA_MAX_STACKLIMIT         0x7FFF      // Maximum operand stack limit for a single function.
#define PIKA_OPERAND_STACK_EXTRA    8           // Extra space added to a context's operand stack in-order to support type conversions and override operator calls.
#define PIKA_NATIVE_STACK_EXTRA     16          // Amount you can safely push without checking for an operand stack overflow. Should be at least PIKA_OPERAND_STACK_EXTRA.
#define PIKA_MAX_NATIVE_RECURSION   128         // Maximum number of recursive native calls allowed. And the number of interpreter calls allowed.
#define PIKA_MAX_RETC               128         // Maximum number of return values allowed

#ifdef PIKA_64BIT_INT
#   define          PINT_MAX       (LONG_LONG_MAX)
#   define          PINT_MIN       (LONG_LONG_MIN)
typedef             u8              puint_t;
typedef             s8              pint_t;
#   define          Pika_Abs        PIKA_ABS64
#   define          PINT_FMT        PIKA_INT_64BIT_FMT
#else
#   define          PINT_MAX       (INT_MAX)
#   define          PINT_MIN       (INT_MIN)
typedef             u4              puint_t;
typedef             s4              pint_t;
#   define          Pika_Abs        PIKA_ABS32
#   define          PINT_FMT        PIKA_INT_32BIT_FMT
#endif
#define             PIKA_REAL_FMT   "%.14g"

#if defined(PIKA_64BIT_REAL)
typedef double preal_t;
PIKA_FORCE_INLINE preal_t Pika_Absf(preal_t x) { return fabs(x); }
PIKA_FORCE_INLINE preal_t ArcCos(preal_t x)    { return acos(x); }
PIKA_FORCE_INLINE preal_t ArcSin(preal_t x)    { return asin(x); }
PIKA_FORCE_INLINE preal_t ArcTan(preal_t x)    { return atan(x); }
PIKA_FORCE_INLINE preal_t ArcTan2(preal_t x,
                                  preal_t y)   { return atan2(x, y); }
PIKA_FORCE_INLINE preal_t Ceil(preal_t x)   { return ceil(x); }
PIKA_FORCE_INLINE preal_t CopySign(preal_t x,
                                   preal_t y)  { return copysign(x, y); }
PIKA_FORCE_INLINE preal_t Cos(preal_t x)    { return cos(x); }
PIKA_FORCE_INLINE preal_t Cosh(preal_t x)   { return cosh(x); }
PIKA_FORCE_INLINE preal_t Exp(preal_t x)    { return exp(x); }
PIKA_FORCE_INLINE preal_t Floor(preal_t x)  { return floor(x); }
PIKA_FORCE_INLINE preal_t Mod(preal_t x,
                              preal_t y)    { return fmod(x, y); }
PIKA_FORCE_INLINE preal_t Log(preal_t x)    { return log(x); }
PIKA_FORCE_INLINE preal_t Log10(preal_t x)  { return log10(x); }
PIKA_FORCE_INLINE preal_t Pow(preal_t x,
                              preal_t y)    { return pow(x, y); }
PIKA_FORCE_INLINE preal_t Sin(preal_t x)    { return sin(x); }
PIKA_FORCE_INLINE preal_t Sinh(preal_t x)   { return sinh(x); }
PIKA_FORCE_INLINE preal_t Sqrt(preal_t x)   { return sqrt(x); }
PIKA_FORCE_INLINE preal_t Tan(preal_t x)    { return tan(x); }
PIKA_FORCE_INLINE preal_t Tanh(preal_t x)   { return tanh(x); }
#else
typedef float preal_t;
PIKA_FORCE_INLINE preal_t Pika_Absf(preal_t x) { return (preal_t)fabs(x); }
PIKA_FORCE_INLINE preal_t ArcCos(preal_t x)    { return (preal_t)acosf(x); }
PIKA_FORCE_INLINE preal_t ArcSin(preal_t x)    { return (preal_t)asinf(x); }
PIKA_FORCE_INLINE preal_t ArcTan(preal_t x)    { return (preal_t)atanf(x); }
PIKA_FORCE_INLINE preal_t ArcTan2(preal_t x,
                                  preal_t y)   { return (preal_t)atan2f(x, y);}
PIKA_FORCE_INLINE preal_t Ceil(preal_t x)   { return (preal_t)ceilf(x); }
PIKA_FORCE_INLINE preal_t CopySign(preal_t x,
                                   preal_t y)  { return (preal_t)copysignf(x, y);}
PIKA_FORCE_INLINE preal_t Cos(preal_t x)    { return (preal_t)cosf(x); }
PIKA_FORCE_INLINE preal_t Cosh(preal_t x)   { return (preal_t)coshf(x); }
PIKA_FORCE_INLINE preal_t Exp(preal_t x)    { return (preal_t)expf(x); }
PIKA_FORCE_INLINE preal_t Floor(preal_t x)  { return (preal_t)floorf(x); }
PIKA_FORCE_INLINE preal_t Mod(preal_t x, 
                              preal_t y)    { return (preal_t)fmodf(x, y);}
PIKA_FORCE_INLINE preal_t Log(preal_t x)    { return (preal_t)logf(x); }
PIKA_FORCE_INLINE preal_t Log10(preal_t x)  { return (preal_t)log10f(x); }
PIKA_FORCE_INLINE preal_t Pow(preal_t x, 
                              preal_t y)    { return (preal_t)powf(x, y); }
PIKA_FORCE_INLINE preal_t Sin(preal_t x)    { return (preal_t)sinf(x); }
PIKA_FORCE_INLINE preal_t Sinh(preal_t x)   { return (preal_t)sinhf(x); }
PIKA_FORCE_INLINE preal_t Sqrt(preal_t x)   { return (preal_t)sqrtf(x); }
PIKA_FORCE_INLINE preal_t Tan(preal_t x)    { return (preal_t)tanf(x); }
PIKA_FORCE_INLINE preal_t Tanh(preal_t x)   { return (preal_t)tanhf(x); }
#endif

PIKA_FORCE_INLINE pint_t Pika_RealToInteger(preal_t x) { return (pint_t)x; }
PIKA_FORCE_INLINE bool   Pika_RealIsInteger(preal_t x) { return modf(x, &x) == 0; }
PIKA_FORCE_INLINE bool   Pika_RealToBoolean(preal_t x) { return Pika_isnan((double)x) == 0 && x != (preal_t)0.0; }

INLINE bool IsAscii(int x)  { return isascii(x) != 0; }                 // Is an ascii character.
INLINE bool IsLetter(int x) { return IsAscii(x) && (isalpha(x) != 0); } // Is an upper or lower case letter
INLINE bool IsDigit(int x)  { return IsAscii(x) && (isdigit(x) != 0); } // Is a digit.
INLINE bool IsSpace(int x)  { return IsAscii(x) && (isspace(x) != 0); } // Is white space.

INLINE bool IsUpper(int x)  { return IsAscii(x) && (isupper(x) != 0); } // Is an upper case letter.
INLINE bool IsLower(int x)  { return IsAscii(x) && (islower(x) != 0); } // Is a lower case letter.

INLINE int  ToLower(int x)  { return(IsLetter(x)) ? tolower(x) : x; }   // Converts a letter to lower case.
INLINE int  ToUpper(int x)  { return(IsLetter(x)) ? toupper(x) : x; }   // Converts a letter to upper case.

INLINE bool IsLetterOrDigit(int x)   { return IsAscii(x) && (isalnum(x) != 0); } // Is a letter or a digit.
INLINE bool IsIdentifierExtra(int x) { return x == '_'   || x == '$'; }          // Is part of a valid identifier.
INLINE bool IsValidDigit(int x)      { return IsDigit(x) || x == '_'; }          // Is part of a valid number literal.

#define PIKA_BUFFER_MAX_LEN     (PINT_MAX)             // Max length a vector or other buffer may be.
#define PIKA_STRING_MAX_LEN     (PIKA_BUFFER_MAX_LEN) // Max length a string may be.
#define PIKA_MAX_SLOTS          (0xFFFF)               // Max number of slots an object can have. doesn't effect vectors or strings.

// Sanity check!

#if defined(PIKA_64) && defined(PIKA_32)
#   undef PIKA_32
#endif

#if !defined(PIKA_64) && !defined(PIKA_32)
#   error architecture size not set
#endif

typedef u4 code_t; // needs to be atleast 32bit

#define PIKA_MAX_NESTED_FUNCTIONS 255

#if defined(ENABLE_TRACE)
#define trace0(msg)                 fprintf(stderr, msg)
#define trace1(msg, A)              fprintf(stderr, msg, A)
#define trace2(msg, A, B)           fprintf(stderr, msg, A, B)
#define trace3(msg, A, B, C)        fprintf(stderr, msg, A, B, C)
#define trace4(msg, A, B, C, D)     fprintf(stderr, msg, A, B, C, D)
#else
#define trace0(msg)                 do {} while(0)
#define trace1(msg, A)              do {((void*)A);} while(0)
#define trace2(msg, A, B)           do {((void*)A);((void*)B);} while(0)
#define trace3(msg, A, B, C)        do {((void*)A);((void*)B);((void*)C);} while(0)
#define trace4(msg, A, B, C, D)     do {((void*)A);((void*)B);((void*)C);((void*)D);} while(0)
#endif

#if defined(PIKA_STRING_CASE_INSENSITIVE)
#define StrCmp          Pika_strcasecmp
#define StrCmpWithSize  strncasecmp
#define StrGetChar(X)   tolower(X)
#else
#define StrCmp          strcmp
#define StrCmpWithSize  strncmp
#define StrGetChar(X)   (X)
#endif

#define SHOULD_NEVER_HAPPEN() ASSERT(0)
#endif
