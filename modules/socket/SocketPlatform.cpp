/*
 *  SockeyPlatform.cpp
 *  See Copyright Notice in PSocketModule.h
 */
#include "SocketPlatform.h"

char* Pika_ErrorMessage(int err)
{
    size_t ERROR_SIZE = 1024;
    char* errorMessage = (char*)Pika_malloc(ERROR_SIZE);
    
    while (strerror_r(err, errorMessage, ERROR_SIZE) == -1 && errno == ERANGE)
    {
        ERROR_SIZE *= 2;
        errorMessage = (char*)Pika_realloc(errorMessage, ERROR_SIZE);
        if (!errorMessage)
            return 0;
    }
    return errorMessage;
}

void Pika_FreeSocketString(char* message)
{
    if (message)
    {
        Pika_free(message);
    }
}
