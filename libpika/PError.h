/*
 *  PError.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_ERROR_HEADER
#define PIKA_ERROR_HEADER

#ifndef PIKA_CONFIG_HEADER
#   include "PConfig.h"
#endif

#ifndef PIKA_CLASSINFO_HEADER
#   include "PClassInfo.h"
#endif

namespace pika {

enum WarningLevel 
{
    WARN_mild,   // no side effects or performance penalties
    WARN_low,    // rare side effects or performance penalties
    WARN_medium, // occasional side effects or performance penalties
    WARN_severe, // guaranteed side effects or performance penalties
};

class Engine;
class Type;

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
        ERROR_userclass,
        MAX_ERROR_KIND,
    }
    const kind;
    
    Exception(Kind k, char* m=0, ClassInfo* ci=0) : kind(k), msg(m), error_class(ci) {}

    virtual ~Exception();
    
    virtual void        Report();
    virtual Type*       GetErrorType(Engine*);
    virtual const char* GetMessage() const { return msg; }
    virtual const char* GetErrorKind(Engine*);
    static const char*  Static_Error_Formats[MAX_ERROR_KIND];
protected:
    char* msg;
    ClassInfo* error_class;
};// Exception

/** Raises an exception through C++'s exception handling framework. 
  * This function never returns b/c a throw statement is executed.
  */
PIKA_API void RaiseException(Exception::Kind k, const char* fmt, ...);
PIKA_API void RaiseException(ClassInfo*, const char* fmt, ...);
PIKA_API void RaiseException(const char* fmt, ...);

/* 
    The following structs are used for registering Type's for custom Error classes. 
    All the built-in types are provided for convenience. 

    Creating a Custom Error Class: 
    ------------------------------
    Note these class definitions are not actual classes that will be instantiated.
    They are merely there to provide an easy way to create the Type from inside
    your module.
    
    First derive from one of the following structs. The class name doesn't matter.
    
    MyError.h
    
        struct MyError: RuntimeError {
            PIKA_DECL(MyError, RuntimeError)
        };

    MyError.cpp
    
        PIKA_IMPL(MyError)


    Then in your module's startup function
    
    MyModule.cpp

        PIKA_MODULE(mymodule, eng, mymodule)
        {
            GCPAUSE(eng);
            
            // Create the Error type so it can be used in scripts 
            // or by your own code.
            
            String* MyError_String = eng->GetString("MyError");
            Type* MyError_Type = Type::Create(
                eng,                    // Pointer to the Engine
                MyError_String,         // Name of the type as a String.
                eng->RuntimeError_Type, // The base or super type.
                0,                      // Instance creation function (leave blank to use the base classes)
                mymodule                // Module or package that will be the global scope
            );
            
            // So you can do mymodule.MyError inside a script.
            mymodule->SetSlot(MyError_String, MyError_Type);
            
            // Store the type in the types_table, for easy access.
            eng->SetTypeFor(MyError::StaticGetClass(), JsonError_Type);
            
            return mymodule;
        }

    At this point you can retrieve it using the following line of code.

        Type* MyError = engine->GetTypeFor(MyError::StaticGetClass())

    Or raise an exception directly...

        RaiseException(MyError::StaticGetClass(), "My printf style formated error string", formatArguments);

*/
struct PIKA_API ErrorClass {
    static ClassInfo* StaticCreateClass();
    PIKA_REG(ErrorClass)
};

struct RuntimeError: ErrorClass {
    PIKA_DECL(RuntimeError, ErrorClass)
};

struct TypeError: ErrorClass {
    PIKA_DECL(TypeError, ErrorClass)
};

struct ArithmeticError: ErrorClass {
    PIKA_DECL(ArithmeticError, ErrorClass)
};

struct OverflowError: ArithmeticError {
    PIKA_DECL(OverflowError, ArithmeticError)
};

struct UnderflowError: ArithmeticError {
    PIKA_DECL(UnderflowError, ArithmeticError)
};

struct DivideByZeroError: ArithmeticError {
    PIKA_DECL(DivideByZeroError, ArithmeticError)
};

struct IndexError: ErrorClass {
    PIKA_DECL(IndexError, ErrorClass)
};

struct SystemError: ErrorClass {
    PIKA_DECL(SystemError, ErrorClass)
};

struct AssertError: ErrorClass {
    PIKA_DECL(AssertError, ErrorClass)
};

}// pika

#endif
