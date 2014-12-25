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
    "ArithmeticError",
    "OverflowError",
    "UnderflowError",
    "DivideByZeroError",
    "IndexError",
    "TypeError",
    "SystemError",
    "AssertError",
    "ScriptError",
    "CustomError"
};
  
Exception::~Exception() { Pika_delete(msg); }

void Exception::Report() { if (msg) { std::cerr << msg << std::endl; }}

const char* Exception::GetErrorKind(Engine* eng)
{
    if (error_class)
    {
        Type* error_type = eng->GetTypeFor(error_class);
        if (error_type)
        {
            return error_type->GetName()->GetBuffer();
        }
    }
    return Static_Error_Formats[kind];
}

Type* Exception::GetErrorType(Engine* eng)
{
    if (error_class)
    {
        return eng->GetTypeFor(error_class);
    }
    return 0;
}

char* vMakeMessage(const char* msg, va_list args)
{
    static const size_t ERR_BUF_SZ = 512;
    
    Exception::Kind k = Exception::ERROR_runtime;
    char buffer[ERR_BUF_SZ + 1];
    int len = Pika_vsnprintf(buffer, ERR_BUF_SZ, msg, args);
    
    if (len > 0)
    {
        buffer[ERR_BUF_SZ] = 0;
        char* errormsg = (char*)Pika_malloc((len + 1) * sizeof(char));
        Pika_memcpy(errormsg, buffer, (len)*sizeof(char));
        errormsg[len] = 0;
        return errormsg;
    }
    return 0;
}

void RaiseException(Exception::Kind k, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char* msg = vMakeMessage(fmt, args);
    va_end(args);
    
    throw Exception(k, msg, 0);
}

void RaiseException(const char* fmt, ...)
{
    Exception::Kind k = Exception::ERROR_runtime;
    va_list args;
    va_start(args, fmt);
    char* msg = vMakeMessage(fmt, args);
    va_end(args);
    
    throw Exception(k, msg, 0);
}

void RaiseException(ClassInfo* ci, const char* fmt, ...)
{
    Exception::Kind k = Exception::ERROR_userclass;
    va_list args;
    va_start(args, fmt);
    char* msg = vMakeMessage(fmt, args);
    va_end(args);
    
    throw Exception(k, msg, ci);
}

ClassInfo* ErrorClass::StaticCreateClass()
{
    if (!ErrorClassClassInfo)    
        ErrorClassClassInfo = ClassInfo::Create("Error", 0);    
    return ErrorClassClassInfo;
}

PIKA_IMPL(ErrorClass)
PIKA_IMPL(RuntimeError)
PIKA_IMPL(TypeError)
PIKA_IMPL(ArithmeticError)
PIKA_IMPL(OverflowError)
PIKA_IMPL(UnderflowError)
PIKA_IMPL(DivideByZeroError)
PIKA_IMPL(IndexError)
PIKA_IMPL(SystemError)
PIKA_IMPL(AssertError)

}// pika
