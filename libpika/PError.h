/*
 *  PError.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_ERROR_HEADER
#define PIKA_ERROR_HEADER

#ifndef PIKA_CONFIG_HEADER
#   include "PConfig.h"
#endif

namespace pika {

enum WarningLevel 
{
    WARN_mild,   // no side effects or performance penalties
    WARN_low,    // rare side effects or performance penalties
    WARN_medium, // occasional side effects or performance penalties
    WARN_severe, // guaranteed side effects or performance penalties
};

class PIKA_API Exception 
{
public:
    enum Kind 
    {
        ERROR_syntax,
        ERROR_runtime,
        ERROR_arithmetic,
        ERROR_overflow,
        ERROR_underflow,
        ERROR_dividebyzero,
        ERROR_index,
        ERROR_type,
        ERROR_system,
        ERROR_assert,
        ERROR_script,        
        MAX_ERROR_KIND,
    }
    const kind;

    Exception(Kind k) : kind(k), msg(0) {}

    Exception(Kind k, char* m) : kind(k), msg(m) {}

    virtual ~Exception();

    virtual void Report();
    
    virtual const char* GetMessage() const { return msg; }
    virtual const char* GetErrorKind();
    static const char* Static_Error_Formats[MAX_ERROR_KIND];
protected:
    char* msg;
};// Exception

/** Raises an exception through C++'s exception handling framework. 
  * This function never returns b/c a throw statement is executed.
  */
PIKA_API void RaiseException(Exception::Kind k, const char* msg, ...);
PIKA_API void RaiseException(const char* msg, ...);


}// pika

#endif
