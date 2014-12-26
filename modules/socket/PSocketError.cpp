/*
 *  PSocketError.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PSocketModule.h"

namespace pika {

void RaiseExceptionFromErrno(Exception::Kind errKind, const char* errMsg, int errorNum)
{
    if (!errorNum)
    {
        errorNum = errno;
    }
        
    ErrorStringHandler handler(errorNum);
    if (handler)
    {
        RaiseException(errKind, "%s with message \"%s\".", errMsg, handler.GetBuffer());
    }
    else
    {
        RaiseException(errKind, "%s.", errMsg);
    }
}

}// pika
