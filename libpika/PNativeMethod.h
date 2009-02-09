/*
 *  PNativeMethod.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_NATIVEMETHODDEF_HEADER
#define PIKA_NATIVEMETHODDEF_HEADER

#include "PString.h"
#include "PArray.h"
#include "PFunction.h"
#include "PPackage.h"
#include "PType.h"

namespace pika
{

//  NativeVarType /////////////////////////////////////////////////////////////////////////////////////////

/** Native type of an argument or return value. Currently not used for anything other than metadata. */

enum NativeVarType 
{
    BTNull,
    BTInt8,  BTInt16,  BTInt32,  BTInt64,
    BTUInt8, BTUInt16, BTUInt32, BTUInt64,
    BTSizeT, BTSSizeT,
    BTFloat, BTDouble,
    BTBool,
    BTCeeString,
    BTScriptString,
    BTUserdata,
    BTObject,
    BTVal
};

// VarType /////////////////////////////////////////////////////////////////////////////////////////

/** Binds a native function's parameter by 
  * converting the script object into a native C++ type.
  */
template<typename AType>
struct VarType
{
};

template<>
struct VarType<Object*>
{
    enum 
    {
        eType = BTObject, // Native type
        eSig  = 'o',      // Identifying character used by ParseArgs and ParseArgsInPlace
    };

    INLINE VarType() { }
    INLINE VarType(Context* ctx, u2 arg) { val = ctx->GetObjectArg(arg); }
    INLINE VarType(Value* args, u2 arg) { val = args[arg].val.object; }

    INLINE operator Object*() { return val; }

    Object* val;
};

template<>
struct VarType<Value&>
{
    enum 
    {
        eType = BTVal,
        eSig  = 'x',
    };

    INLINE VarType() { }
    INLINE VarType(Context* ctx, u2 arg) { val = ctx->GetArg(arg); }
    INLINE VarType(Value* args, u2 arg) { val = args[arg]; }

    INLINE operator Value&() { return val; }

    Value val;
};

template<>
struct VarType<const Value&>
{
    enum 
    {
        eType = BTVal,
        eSig  = 'x',
    };

    INLINE VarType() { }
    INLINE VarType(Context* ctx, u2 arg) { val = ctx->GetArg(arg); }
    INLINE VarType(Value* args, u2 arg) { val = args[arg]; }

    INLINE operator const Value&() { return val; }

    Value val;
};

template<>
struct VarType<Value>
{
    enum 
    {
        eType = BTVal,
        eSig  = 'x',
    };

    INLINE VarType() {}
    INLINE VarType(Context* ctx, u2 arg) { val = ctx->GetArg(arg); }
    INLINE VarType(Value* args, u2 arg) { val = args[arg]; }

    INLINE operator Value() { return val; }

    Value val;
};

template<>
struct VarType<Array*>
{
    enum 
    {
        eType = BTObject,
        eSig = 'o',
    };

    INLINE VarType() {}
    INLINE VarType(Context* ctx, u2 arg) { val = ctx->GetObjectArg(arg); }
    INLINE VarType(Value* args, u2 arg) { val = args[arg].val.object; }

    INLINE operator Array*()
    {
        if (!val->IsDerivedFrom(Array::StaticGetClass()))
            RaiseException("Expecting Array argument\n.");
        return (Array*)val;
    }

    Object* val;
};

template<>
struct VarType<Array&>
{
    enum 
    {
        eType = BTObject,
        eSig = 'o',
    };

    INLINE VarType() { }
    INLINE VarType(Context* ctx, u2 arg) { val = ctx->GetObjectArg(arg); }
    INLINE VarType(Value* args, u2 arg) { val = args[arg].val.object; }

    INLINE operator Array&()
    {
        if (!val->IsDerivedFrom(Array::StaticGetClass()))
            RaiseException("Expecting Array argument\n.");
        return *(Array*)val;
    }

    Object* val;
};

template<>
struct VarType<float>
{
    enum 
    {
        eType = BTFloat,
        eSig  = 'R',
    };

    INLINE VarType() { }
    INLINE VarType(Context* ctx, u2 arg) { val = (float)ctx->ArgToReal(arg); }
    INLINE VarType(Value* args, u2 arg)  { val = (float)args[arg].val.real;  }
    INLINE operator float() { return val; }

    float val;
};

template<>
struct VarType<double>
{
    enum 
    {
        eType = BTDouble,
        eSig  = 'R',
    };

    INLINE VarType() { }
    INLINE VarType(Context* ctx, u2 arg) { val = (double)ctx->ArgToReal(arg); }
    INLINE VarType(Value* args, u2 arg)  { val = (double)args[arg].val.real;  }
    INLINE operator double() { return val; }

    double val;
};

template<>
struct VarType<bool>
{
    enum 
    {
        eType = BTBool,
        eSig  = 'B',
    };

    INLINE VarType() { }
    INLINE VarType(Context* ctx, u2 arg) { val = ctx->ArgToBool(arg); }
    INLINE VarType(Value* args, u2 arg) { val = args[arg].val.index != 0; }
    INLINE operator bool() { return val; }

    bool val;
};

template<>
struct VarType<const char*>
{
    enum 
    {
        eType = BTCeeString,
        eSig  = 'S',
    };

    INLINE VarType() { }
    INLINE VarType(Context* ctx, u2 arg) { val = ctx->GetStringArg(arg); }
    INLINE VarType(Value* args, u2 arg) { val = args[arg].val.str; }
    INLINE operator const char*() { return val->GetBuffer(); }

    String* val;
};

template<>
struct VarType<String*>
{
    enum 
    {
        eType = BTScriptString,
        eSig  = 'S',
    };

    INLINE VarType() { }
    INLINE VarType(Context* ctx, u2 arg) { val = ctx->GetStringArg(arg); }
    INLINE VarType(Value* args, u2 arg) { val = args[arg].val.str; }
    INLINE operator String*() { return val; }

    String* val;
};

// RetType /////////////////////////////////////////////////////////////////////////////////////////

/** Binds a native function's return value by 
  * converting the native C++ type into a Script object.
  */
template<typename AType>
struct RetType
{
};

template<>
struct RetType<Value>
{
    INLINE RetType(Context* ctx, Value v)
    {
        ctx->Push(v);
    }
};

template<>
struct RetType<Value&>
{
    INLINE RetType(Context* ctx, Value& v)
    {
        ctx->Push(v);
    }
};

template<>
struct RetType<float>
{
    INLINE RetType(Context* ctx, float f)
    {
        ctx->Push((preal_t)f);
    }
};

template<>
struct RetType<double>
{
    INLINE RetType(Context* ctx, double f)
    {
        ctx->Push((preal_t)f);
    }
};

template<>
struct RetType<bool>
{
    INLINE RetType(Context* ctx, bool b)
    {
        ctx->PushBool(b);
    }
};

template<>
struct RetType<const char*>
{
    INLINE RetType(Context* ctx, const char* s)
    {
        // TODO: Might be a good idea to pause to GC.
        String* str = s ? ctx->GetEngine()->AllocString(s) : ctx->GetEngine()->emptyString;
        ctx->Push(str);
    }
};

template<>
struct RetType<char*>
{
    INLINE RetType(Context* ctx, char* s)
    {
        // TODO: Might be a good idea to pause to GC.
        String* str = s ? ctx->GetEngine()->AllocString(s) : ctx->GetEngine()->emptyString;
        ctx->Push(str);
    }
};

template<>
struct RetType<String*>
{
    INLINE RetType(Context* ctx, String* str)
    {
        if (str)
        {
            ctx->Push(str);
        }
        else
        {
            ctx->PushNull();
        }
    }
};

template<>
struct RetType<Object*>
{
    INLINE RetType(Context* ctx, Object* o)
    {
        if (o)
        {
            ctx->Push(o);
        }
        else
        {
            ctx->PushNull();
        }        
    }
};

template<>
struct RetType<Array*>
{
    INLINE RetType(Context* ctx, Array* v)
    {
        if (v)
        {
            ctx->Push(v);
        }
        else
        {
            ctx->PushNull();
        }        
    }
};

// NativeMethodBase ////////////////////////////////////////////////////////////////////////////////

struct PIKA_API NativeMethodBase
{
    virtual ~NativeMethodBase() = 0;
    virtual void Invoke(void* obj, Context*) = 0;
    virtual int  GetRetCount() const { return 1; }
    virtual int  GetArgCount() const = 0;
    void*        operator new(size_t n);
    void         operator delete(void *v);
};

#include "PNativeMethodDecls.h"
#include "PNativeConstMethodDecls.h"
#include "PNativeStaticMethodDecls.h"

// MakeStaticMethodVA //////////////////////////////////////////////////////////////////////////////

template <typename TRet>
NativeMethodBase* MakeStaticMethodVA(TRet(*func)(Context*))
{
    return new StaticMethodVA<TRet>(func);
}

// MakeFunctionVA //////////////////////////////////////////////////////////////////////////////////

template <class AClass, typename TRet>
NativeMethodBase* MakeFunctionVA(TRet(AClass::*func)(Context*))
{
    return new MethodVA<AClass, TRet>(func);
}

// MakeMethod //////////////////////////////////////////////////////////////////////////////////////

template <class AClass, typename TRet> 
NativeMethodBase* 
MakeMethod(TRet(AClass::*func)())
{
    return new Method0<AClass, TRet>(func);
}

template <class AClass, typename TRet, typename TParam0> 
NativeMethodBase* 
MakeMethod(TRet(AClass::*func)(TParam0))
{
    return new Method1<AClass, TRet, TParam0>(func);
}

template <class AClass, typename TRet, typename TParam0, typename TParam1> 
NativeMethodBase* 
MakeMethod(TRet(AClass::*func)(TParam0, TParam1))
{
    return new Method2<AClass, TRet, TParam0, TParam1>(func);
}

template <class AClass, typename TRet, typename TParam0, typename TParam1, typename TParam2> 
NativeMethodBase*
MakeMethod(TRet(AClass::*func)(TParam0, TParam1, TParam2))
{
    return new Method3<AClass, TRet, TParam0, TParam1, TParam2>(func);
}

template <class AClass, typename TRet, typename TParam0, typename TParam1, typename TParam2, 
typename TParam3> 
NativeMethodBase* 
MakeMethod(TRet(AClass::*func)(TParam0, TParam1, TParam2, TParam3))
{
    return new Method4<AClass, TRet, TParam0, TParam1, TParam2, TParam3>(func);
}

template <class AClass, typename TRet, typename TParam0, typename TParam1, typename TParam2, 
typename TParam3, typename TParam4>
NativeMethodBase* 
MakeMethod(TRet(AClass::*func)(TParam0, TParam1, TParam2,
                            TParam3, TParam4))
{
    return new Method5<AClass, TRet, TParam0, TParam1, TParam2, TParam3, TParam4>(func);
}

template <class AClass, typename TRet, typename TParam0, typename TParam1, 
typename TParam2, typename TParam3, typename TParam4, typename TParam5> 
NativeMethodBase* 
MakeMethod(TRet(AClass::*func)(TParam0, TParam1, TParam2, TParam3,
                            TParam4, TParam5))
{
    return new Method6<AClass, TRet, TParam0, TParam1, TParam2, TParam3, TParam4, TParam5>(func);
}

template <class AClass, typename TRet, typename TParam0, typename TParam1, typename TParam2,
typename TParam3, typename TParam4, typename TParam5, typename TParam6> 
NativeMethodBase* 
MakeMethod(TRet(AClass::*func)(TParam0, TParam1, TParam2, TParam3,
                            TParam4, TParam5, TParam6))
{
    return new Method7 < AClass, TRet, TParam0, TParam1, TParam2, TParam3,
           TParam4, TParam5, TParam6 > (func);
}

template <class AClass, typename TRet, typename TParam0, typename TParam1, typename TParam2,
typename TParam3, typename TParam4, typename TParam5, typename TParam6, typename TParam7> 
NativeMethodBase* 
MakeMethod(TRet(AClass::*func)(TParam0, TParam1, TParam2, TParam3,
                            TParam4, TParam5, TParam6,
                            TParam7))
{
    return new Method8 < AClass, TRet, TParam0, TParam1, TParam2, TParam3,
           TParam4, TParam5, TParam6,
           TParam7 > (func);
}

template <class AClass, typename TRet, typename TParam0, typename TParam1, typename TParam2,
typename TParam3, typename TParam4, typename TParam5,
typename TParam6, typename TParam7, typename TParam8> 
NativeMethodBase* 
MakeMethod(TRet(AClass::*func)(TParam0, TParam1, TParam2, TParam3,
                            TParam4, TParam5, TParam6,
                            TParam7, TParam8))
{
    return new Method9 < AClass, TRet, TParam0, TParam1, TParam2, TParam3,
           TParam4, TParam5, TParam6,
           TParam7, TParam8 > (func);
}

template <class AClass, typename TRet, typename TParam0, typename TParam1, typename TParam2,
typename TParam3, typename TParam4, typename TParam5,
typename TParam6, typename TParam7, typename TParam8,
typename TParam9>
NativeMethodBase* 
MakeMethod(TRet(AClass::*func)(TParam0, TParam1, TParam2, TParam3,
                            TParam4, TParam5, TParam6,
                            TParam7, TParam8, TParam9))
{
    return new Method10 < AClass, TRet, TParam0, TParam1, TParam2, TParam3,
           TParam4, TParam5, TParam6,
           TParam7, TParam8, TParam9 > (func);
}

// MakeFunctionVA (const) //////////////////////////////////////////////////////////////////////////

template <class AClass, typename TRet>
NativeMethodBase* MakeFunctionVA(TRet(AClass::*func)(Context*) const)
{
    return new ConstMethodVA<AClass, TRet>(func);
}

// MakeMethod (const) //////////////////////////////////////////////////////////////////////////

template <class AClass, typename TRet>
NativeMethodBase* MakeMethod(TRet(AClass::*func)() const)
{
    return new ConstMethod0<AClass, TRet>(func);
}

template <class AClass, typename TRet, typename TParam0>
NativeMethodBase* MakeMethod(TRet(AClass::*func)(TParam0) const)
{
    return new ConstMethod1<AClass, TRet, TParam0>(func);
}

template <class AClass, typename TRet, typename TParam0, typename TParam1>
NativeMethodBase* MakeMethod(TRet(AClass::*func)(TParam0, TParam1) const)
{
    return new ConstMethod2<AClass, TRet, TParam0, TParam1>(func);
}

template <class AClass, typename TRet, typename TParam0, typename TParam1, typename TParam2>
NativeMethodBase* MakeMethod(TRet(AClass::*func)(TParam0, TParam1, TParam2) const)
{
    return new ConstMethod3<AClass, TRet, TParam0, TParam1, TParam2>(func);
}

template <class AClass, typename TRet, typename TParam0, typename TParam1, typename TParam2, typename TParam3>
NativeMethodBase* MakeMethod(TRet(AClass::*func)(TParam0, TParam1, TParam2, TParam3) const)
{
    return new ConstMethod4<AClass, TRet, TParam0, TParam1, TParam2, TParam3>(func);
}

template <class AClass, typename TRet, typename TParam0, typename TParam1, typename TParam2,
typename TParam3, typename TParam4>
NativeMethodBase* MakeMethod(TRet(AClass::*func)(TParam0, TParam1, TParam2, TParam3, TParam4) const)
{
    return new ConstMethod5<AClass, TRet, TParam0, TParam1, TParam2, TParam3, TParam4>(func);
}

template <class AClass, typename TRet, typename TParam0, typename TParam1, typename TParam2,
typename TParam3, typename TParam4, typename TParam5>
NativeMethodBase* MakeMethod(TRet(AClass::*func)(TParam0, TParam1, TParam2, TParam3, TParam4, TParam5) const)
{
    return new ConstMethod6<AClass, TRet, TParam0, TParam1, TParam2, TParam3, TParam4, TParam5>(func);
}

template <class AClass, typename TRet, typename TParam0, typename TParam1, typename TParam2,
typename TParam3, typename TParam4, typename TParam5, typename TParam6>
NativeMethodBase* MakeMethod(TRet(AClass::*func)(TParam0, TParam1, TParam2, TParam3,
                            TParam4, TParam5, TParam6) const)
{
    return new ConstMethod7 < AClass, TRet, TParam0, TParam1, TParam2, TParam3,
           TParam4, TParam5, TParam6 > (func);
}

template <class AClass, typename TRet, typename TParam0, typename TParam1, typename TParam2,
typename TParam3, typename TParam4, typename TParam5,
typename TParam6, typename TParam7>
NativeMethodBase* MakeMethod(TRet(AClass::*func)(TParam0, TParam1, TParam2, TParam3,
                            TParam4, TParam5, TParam6,
                            TParam7) const)
{
    return new ConstMethod8 < AClass, TRet, TParam0, TParam1, TParam2, TParam3,
           TParam4, TParam5, TParam6,
           TParam7 > (func);
}

template <class AClass, typename TRet, typename TParam0, typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5, typename TParam6, typename TParam7, typename TParam8>
NativeMethodBase* MakeMethod(TRet(AClass::*func)(TParam0, TParam1, TParam2, TParam3, TParam4, TParam5, TParam6, TParam7, TParam8) const)
{
    return new ConstMethod9 < AClass, TRet, TParam0, TParam1, TParam2, TParam3, TParam4, TParam5, TParam6, TParam7, TParam8 > (func);
}

template <class AClass, typename TRet, typename TParam0, typename TParam1, typename TParam2,
typename TParam3, typename TParam4, typename TParam5,
typename TParam6, typename TParam7, typename TParam8,
typename TParam9>
NativeMethodBase* MakeMethod(TRet(AClass::*func)(TParam0, TParam1, TParam2, TParam3,
                            TParam4, TParam5, TParam6,
                            TParam7, TParam8, TParam9) const)
{
    return new ConstMethod10 < AClass, TRet, TParam0, TParam1, TParam2, TParam3,
           TParam4, TParam5, TParam6,
           TParam7, TParam8, TParam9 > (func);
}

// MakeStaticMethod //////////////////////////////////////////////////////////////////////////

template <typename TRet>
NativeMethodBase* MakeStaticMethod(TRet(*func)())
{
    return new StaticMethod0<TRet>(func);
}

template <typename TRet, typename TParam0>
NativeMethodBase* MakeStaticMethod(TRet(*func)(TParam0))
{
    return new StaticMethod1<TRet, TParam0>(func);
}

template <typename TRet, typename TParam0, typename TParam1>
NativeMethodBase* MakeStaticMethod(TRet(*func)(TParam0, TParam1))
{
    return new StaticMethod2<TRet, TParam0, TParam1>(func);
}

template <typename TRet, typename TParam0, typename TParam1, typename TParam2>
NativeMethodBase* MakeStaticMethod(TRet(*func)(TParam0, TParam1, TParam2))
{
    return new StaticMethod3<TRet, TParam0, TParam1, TParam2>(func);
}

template <typename TRet, typename TParam0, typename TParam1, typename TParam2, typename TParam3>
NativeMethodBase* MakeStaticMethod(TRet(*func)(TParam0, TParam1, TParam2, TParam3))
{
    return new StaticMethod4<TRet, TParam0, TParam1, TParam2, TParam3>(func);
}

template <typename TRet, typename TParam0, typename TParam1, typename TParam2, typename TParam3, typename TParam4>
NativeMethodBase* MakeStaticMethod(TRet(*func)(TParam0, TParam1, TParam2, TParam3, TParam4))
{
    return new StaticMethod5<TRet, TParam0, TParam1, TParam2, TParam3, TParam4>(func);
}

template <typename TRet, typename TParam0, typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5>
NativeMethodBase* MakeStaticMethod(TRet(*func)(TParam0, TParam1, TParam2, TParam3, TParam4, TParam5))
{
    return new StaticMethod6<TRet, TParam0, TParam1, TParam2, TParam3, TParam4, TParam5>(func);
}

// BIND_INT_TYPE ///////////////////////////////////////////////////////////////////////////////////

#define BIND_INT_TYPE(TYPE, ENUM)                                                                  \
    template<> struct VarType<TYPE>                                                                \
    {                                                                                              \
        enum                                                                                       \
        {                                                                                          \
            eType = ENUM,                                                                          \
            eSig = 'I',                                                                            \
        };                                                                                         \
                                                                                                   \
        INLINE VarType() { }                                                                       \
        INLINE VarType(Context* ctx, u2 arg) { val = (TYPE)ctx->ArgToInt(arg); }                   \
        INLINE VarType(Value* args,  u2 arg) { val = (TYPE)args[arg].val.integer; }                \
                                                                                                   \
        INLINE operator TYPE() { return val; }                                                     \
                                                                                                   \
        TYPE val;                                                                                  \
    };                                                                                             \
    template<> struct RetType<TYPE>                                                                \
    {                                                                                              \
        INLINE RetType(Context* ctx, TYPE i)                                                       \
        {                                                                                          \
            ctx->Push((pint_t)i);                                                                  \
        }                                                                                          \
    }

// DECLARE_BINDING /////////////////////////////////////////////////////////////////////////////////

#define DECLARE_BINDING(TCLASS)                                                                 \
template<>                                                                                      \
struct VarType<TCLASS*>                                                                         \
{                                                                                               \
    enum                                                                                        \
    {                                                                                           \
        eType = BTObject,                                                                       \
        eSig = 'o',                                                                             \
    };                                                                                          \
    INLINE VarType() { }                                                                        \
    INLINE VarType(Context* ctx, u2 arg) { val = ctx->GetObjectArg(arg); }                      \
    INLINE VarType(Value* args, u2 arg) { val = args[arg].val.object; }                         \
    INLINE operator TCLASS*()                                                                   \
    {                                                                                           \
        if (!val->IsDerivedFrom(TCLASS::StaticGetClass()))                                      \
            RaiseException("Expecting %s argument\n.", TCLASS::StaticGetClass()->GetName());    \
        return (TCLASS*)val;                                                                    \
    }                                                                                           \
    Object* val;                                                                                \
};                                                                                              \
template<>                                                                                      \
struct VarType<const TCLASS*>                                                                   \
{                                                                                               \
    enum                                                                                        \
    {                                                                                           \
        eType = BTObject,                                                                       \
        eSig = 'o',                                                                             \
    };                                                                                          \
    INLINE VarType() { }                                                                        \
    INLINE VarType(Context* ctx, u2 arg) { val = ctx->GetObjectArg(arg); }                      \
    INLINE VarType(Value* args, u2 arg) { val = args[arg].val.object; }                         \
    INLINE operator const TCLASS*()                                                             \
    {                                                                                           \
        if (!val->IsDerivedFrom(TCLASS::StaticGetClass()))                                      \
            RaiseException("Expecting %s argument\n.", TCLASS::StaticGetClass()->GetName());    \
        return (TCLASS*)val;                                                                    \
    }                                                                                           \
    Object* val;                                                                                \
};\
                                                                                                \
template<>                                                                                      \
struct VarType<TCLASS&>                                                                         \
{                                                                                               \
    enum                                                                                        \
    {                                                                                           \
        eType = BTObject,                                                                       \
        eSig = 'o',                                                                             \
    };                                                                                          \
    INLINE VarType() { }                                                                        \
    INLINE VarType(Context* ctx, u2 arg) { val = ctx->GetObjectArg(arg); }                      \
    INLINE VarType(Value* args, u2 arg) { val = args[arg].val.object; }                         \
    INLINE operator TCLASS&()                                                                   \
    {                                                                                           \
        if (!val->IsDerivedFrom(TCLASS::StaticGetClass()))                                      \
            RaiseException("Expecting %s argument\n.", TCLASS::StaticGetClass()->GetName());    \
        return *(TCLASS*)val;                                                                   \
    }                                                                                           \
    Object* val;                                                                                \
};                                                                                              \
                                                                                                \
template<>                                                                                      \
struct RetType<TCLASS*>                                                                         \
{                                                                                               \
    INLINE RetType(Context* ctx, TCLASS* v)                                                     \
    {                                                                                           \
        if (v) ctx->Push(v);                                                                    \
        else ctx->PushNull();                                                                   \
    }                                                                                           \
}                                                                                               \

// VarType< Nullable<T> >
// --------------------------------------------------------------------------------------
// By default passing null as a bound parameter that expects type T will cause an
// exception. Wrapping the parameter with Nullable will tell the script that its ok
// to pass a null pointer.
//
// example:
//
// class Foo {
// public:
//     bool LoadFile(Nullable<String*> filename) {
//         if (!filename) return false;
//         ...
//     }
// };
//

template<typename T>
struct VarType< Nullable<T> >
{
    enum 
    {
        eType =  VarType<T>::eType,
        eSig  = -VarType<T>::eSig,
    };

    INLINE VarType() { }

    INLINE VarType(Context* ctx, u2 arg)
    {
        Value& v = ctx->GetArg(arg);
        if (v.IsNull())
        {
            val = (T)0;
        }
        else
        {
            VarType<T> vt(ctx, arg);
            val = vt;
        }
    }

    INLINE VarType(Value* args, u2 arg)
    {
        if (args[arg].IsNull())
        {
            val = (T)0;
        }
        else
        {
            VarType<T> vt(args, arg);
            val = vt;
        }
    }

    INLINE operator Nullable<T>() { return Nullable<T>(val); }

    T val;
};
////////////////////////////////////////////////////////////////////////////////
template<>
struct VarType<Function*>
{
    enum
    {
        eType = BTObject,
        eSig = 'o',
    };
    INLINE VarType() { }
    
    INLINE VarType(Context* ctx, u2 arg) { val = ctx->GetObjectArg(arg); }
    
    INLINE VarType(Value* args, u2 arg) { val = args[arg].val.object; }
    
    INLINE operator Function*()
    {
        if (!val->IsDerivedFrom(Function::StaticGetClass()))
        {
            Value res;
            if (GetOverrideFrom(val->GetEngine(), val, OVR_call, res))
            {
                return (Function*)res.val.object;
            }
            else
            {
                RaiseException(Exception::ERROR_type, "Expecting %s argument\n.", Function::StaticGetClass()->GetName());
            }
        }
        return (Function*)val;
    }
    Object* val;
};

template<>
struct RetType<Function*>
{
    INLINE RetType(Context* ctx, Function* v)
    {
        if (v) ctx->Push(v);
        else ctx->PushNull();
    }
};

//////////////////////////////////////////////////////////////////////////////////////////
//DECLARE_BINDING(Function);

DECLARE_BINDING(Package);
DECLARE_BINDING(Type);

BIND_INT_TYPE(s1, BTInt8 );
BIND_INT_TYPE(s2, BTInt16);
BIND_INT_TYPE(s4, BTInt32);
BIND_INT_TYPE(s8, BTInt64);

BIND_INT_TYPE(u1, BTUInt8 );
BIND_INT_TYPE(u2, BTUInt16);
BIND_INT_TYPE(u4, BTUInt32);
BIND_INT_TYPE(u8, BTUInt64);

// size_t and ssize_t are treated as distinct types.
#ifdef PIKA_MAC
BIND_INT_TYPE(size_t,  BTSizeT );
BIND_INT_TYPE(ssize_t, BTSSizeT);
#endif

}// pika

#endif
