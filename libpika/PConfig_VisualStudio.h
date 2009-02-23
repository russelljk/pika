/*
 *  PConfig_VisualStudio.h
 *  See Copyright Notice in Pika.h
 */

#define PIKA_MSC    // Configuration for MS Visual C++
#define PIKA_WIN    // VC++ is only available on Windows.

typedef unsigned __int8                 u1;
typedef unsigned __int16                u2;
typedef unsigned __int32                u4;
typedef unsigned __int64                u8;

typedef signed   __int8                 s1;
typedef signed   __int16                s2;
typedef signed   __int32                s4;
typedef signed   __int64                s8;

#if defined(_DEBUG)
#   define PIKA_FORCE_INLINE            inline
#else
#if (_MSC_VER >= 1200)
#   define PIKA_FORCE_INLINE            __forceinline
#else
#   define PIKA_FORCE_INLINE            __inline
#endif
#endif

#if defined _WIN64
#   define PIKA_64
#	define ssize_t                      s8
#else
#   define PIKA_32
#	define ssize_t                      s4
#endif

#define PIKA_ALIGN                      8

#define PIKA_STRUCT_ALIGN(n, name)      __declspec(align(n)) struct PIKA_API name
#define PIKA_CLASS_ALIGN(n, name)       __declspec(align(n)) class  PIKA_API name

#define setenv _putenv_s
extern errno_t unsetenv(const char* name);

#define Pika_strcasecmp                 _stricmp
#define Pika_vsnprintf                  _vsnprintf

#define PIKA_MAX_PATH                   _MAX_PATH

#define copysignf                       _copysign
#define copysign                        _copysign
#define Pika_isnan                      _isnan

#define PIKA_INT_64BIT_FMT             "%I64"
#define PIKA_INT_32BIT_FMT             "%d"
#define SIZE_T_FMT                      "%Iu"
#define POINTER_FMT                     "%p"

#define PIKA_ABS64                      __abs64
#define PIKA_ABS32                      abs

#define PIKA_MODULE_EXPORT extern "C" __declspec(dllexport)
