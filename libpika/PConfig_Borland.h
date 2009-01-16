/*
 *  PConfig_Borland.h
 *  See Copyright Notice in Pika.h
 */
// TODO: This file needs to be updated.

#define PIKA_BORLANDC   // Borland Turbo Explorer   
#define PIKA_WIN        // Only on windows?

#include <stdint.h>

typedef unsigned __int8                 u1;
typedef unsigned __int16                u2;
typedef unsigned __int32                u4;
typedef unsigned __int64                u8;

typedef signed   __int8                 s1;
typedef signed   __int16                s2;
typedef signed   __int32                s4;
typedef signed   __int64                s8;

#define PIKA_FORCE_INLINE               inline
#define PIKA_32
#define PIKA_ALIGN                      8

// TODO: How do we align on Borland compilers?
#define PIKA_STRUCT_ALIGN(n, name)      struct name
#define PIKA_CLASS_ALIGN(n, name)       class  name

#define Pika_strcasecmp                 stricmp
#define Pika_isnan                      isnan
#define Pika_vsnprintf                  _vsnprintf
#define PIKA_MAX_PATH                   _MAX_PATH

/*
TODO: Format specifiers
#define PIKA_INT_64BIT_FMT
#define PIKA_INT_32BIT_FMT      "%d"
#define SIZE_T_FMT
#define POINTER_FMT             "%p"
#define PIKA_ABS32              abs
#define PIKA_ABS64              
*/
// floating point alternatives are not provided by Borland.

INLINE float acosf(float x)     { return((float)acos((double)x)); }
INLINE float asinf(float x)     { return((float)asin((double)x)); }
INLINE float atanf(float x)     { return((float)atan((double)x)); }
INLINE float atan2f(float x,
                    float y)    { return((float)atan2((double)x, (double)y)); }
INLINE float ceilf(float x)     { return((float)ceil((double)x)); }
INLINE float cosf(float x)      { return((float)cos((double)x));  }
INLINE float coshf(float x)     { return((float)cosh((double)x)); }
INLINE float expf(float x)      { return((float)exp((double)x));  }
INLINE float floorf(float x)    { return((float)floor((double)x));}
INLINE float fmodf(float x,
                   float y)     { return((float)fmod((double)x, (double)y)); }
INLINE float logf(float x)      { return((float)log((double)x)); }
INLINE float log10f(float x)    { return((float)log10((double)x));}
INLINE float powf(float x,
                  float y)      { return((float)pow((double)x, (double)y)); }
INLINE float sinf(float x)      { return((float)sin((double)x));  }
INLINE float sinhf(float x)     { return((float)sinh((double)x)); }
INLINE float sqrtf(float x)     { return((float)sqrt((double)x)); }
INLINE float tanf(float x)      { return((float)tan((double)x));  }
INLINE float tanhf(float x)     { return((float)tanh((double)x)); }

#define PIKA_MODULE_EXPORT extern "C" __declspec(dllexport)
