#include "PSocketModule.h"

namespace pika {

void RaiseExceptionFromErrno(Exception::Kind errKind, const char* errMsg, int errorNum)
{
    if (!errorNum)
    {
        errorNum = errno;
    }
    
    char* errorMessage = Pika_ErrorMessage(errorNum);
    if (errorMessage)
    {
        RaiseException(errKind, "%s with message \"%s\".", errMsg, errorMessage);
        Pika_FreeErrorMessage(errorMessage);
    }
    else
    {
        RaiseException(errKind, "%s.", errMsg);
    }
}

}// pika
