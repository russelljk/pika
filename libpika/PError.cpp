/*
 *  PError.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"

namespace pika {

const char* Exception::Static_Error_Formats[MAX_ERROR_KIND] =
{
    "SyntaxError",
    "RuntimeError",
    "OverflowError",
    "ArithmeticError",
    "IndexError",
    "TypeError",
    "SystemError",
    "AssertError",
    "ScriptError",
};

Exception::~Exception() { Pika_delete(msg); }

void Exception::Report() { if (msg) { std::cerr << msg << std::endl; }}

void RaiseException(Exception::Kind k, const char *msg, ...)
{
    static const size_t ERR_BUF_SZ = 512;
    char buffer[ERR_BUF_SZ + 1];
    va_list args;

    va_start(args, msg);
    int len = Pika_vsnprintf(buffer, ERR_BUF_SZ, msg, args);
    va_end(args);

    if (len > 0)
    {
        buffer[ERR_BUF_SZ] = 0;
        char* errormsg = (char*)Pika_malloc((len + 1) * sizeof(char));
        Pika_memcpy(errormsg, buffer, (len)*sizeof(char));
        errormsg[len] = 0;
        throw Exception(k, errormsg);
    }
    else
    {
        throw Exception(k);
    }
}

void RaiseException(const char *msg, ...)
{
    static const size_t ERR_BUF_SZ = 512;
    
    Exception::Kind k = Exception::ERROR_runtime;
    char buffer[ERR_BUF_SZ + 1];
    va_list args;

    va_start(args, msg);
    int len = Pika_vsnprintf(buffer, ERR_BUF_SZ, msg, args);
    va_end(args);

    if (len > 0)
    {
        buffer[ERR_BUF_SZ] = 0;
        char* errormsg = (char*)Pika_malloc((len + 1) * sizeof(char));
        Pika_memcpy(errormsg, buffer, (len)*sizeof(char));
        errormsg[len] = 0;
        throw Exception(k, errormsg);
    }
    else
    {
        throw Exception(k);
    }
}

}// pika
