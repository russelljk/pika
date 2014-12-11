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
        
    char* errorMessage = Pika_GetError(errorNum);
    if (errorMessage)
    {
        RaiseException(errKind, "%s with message \"%s\".", errMsg, errorMessage);
        Pika_FreeString(errorMessage);
    }
    else
    {
        RaiseException(errKind, "%s.", errMsg);
    }
}

}// pika