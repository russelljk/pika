/*
 *  PBigReal.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PBigNum.h"

#define BIGREAL_TYPENAME "BigReal"

namespace pika {    
    BigReal::BigReal(Engine* eng, Type* typeObj) : Object(eng, typeObj)
    {
        mpfr_init(this->number);
    }
    
    BigReal::~BigReal()
    {
        mpfr_clear(this->number);
    }

    BigReal* BigReal::Create(Engine* eng, Type* type)
    {
        GCPAUSE_NORUN(eng);
        BigReal* bigreal = 0;
        GCNEW(eng, BigReal, bigreal, (eng, type));
        return bigreal;
    }
    
    void BigReal::Init(Context* ctx)
    {
        u2 argc = ctx->GetArgCount();
        if (argc == 1)
        {
            Value& arg = ctx->GetArg(0);
            if (arg.IsReal())
            {
                preal_t n = arg.val.real;
                this->FromReal(n);   
            }
            else if (arg.IsInteger())
            {
                pint_t n = arg.val.integer;
                mpfr_set_si(this->number, n, DEFAULT_RND);
            }
            else if (arg.IsString())
            {
                String* s = arg.val.str;
                if (mpfr_set_str(this->number, s->GetBuffer(), 10, DEFAULT_RND) == -1)
                {
                    mpfr_init(this->number); // Our number might have changed.
                    RaiseException(Exception::ERROR_type, "Attempt to initialize BigReal with String \"%s\".", s->GetBuffer());
                }
            }
            else if (arg.IsDerivedFrom(BigReal::StaticGetClass()))
            {
                BigReal* br =  static_cast<BigReal*>(arg.val.object);
                mpfr_set(this->number, br->number, DEFAULT_RND);
            }
            else if (arg.IsDerivedFrom(BigInteger::StaticGetClass()))
            {
                BigInteger* bigint =  static_cast<BigInteger*>(arg.val.object);
                mpfr_set_z(this->number, bigint->number, DEFAULT_RND);
            }
            else
            {
                ctx->GetRealArg(0);
            }
        }
        else if (argc != 0)
        {
            ctx->WrongArgCount();
        }
    }
    
    String* BigReal::ToString()
    {
        return this->ToString(10);
    }
    
    String* BigReal::ToString(pint_t radix)
    {
        trace0("toString\n");
        if (radix < 2 or radix > 36)
        {
            RaiseException(Exception::ERROR_type, "Attempt to call BigReal.toString with an invalid radix "PINT_FMT". Radix must be >= 2 and <= 36.", radix);
        }
        
        int sd = 0; // significant digits.
        String* res = 0;
        
        if (true)
        {
            pint_t prec = mpfr_get_prec(this->number);
            Buffer<char> buff(prec);
            mpfr_snprintf(buff.GetAt(0), buff.GetSize(), "%.Rf", this->number);
            buff.Push('\0');
            res = engine->GetString(buff.GetAt(0)); 
        }
        else
        {
            res = engine->emptyString;
        }
                    
        return res;
    }
    
    preal_t BigReal::ToReal() const
    {
        return (preal_t)mpfr_get_d(this->number, DEFAULT_RND);
    }
    
    pint_t BigReal::ToInteger() const
    {
        if (!mpfr_fits_slong_p(this->number, DEFAULT_RND))
        {
            RaiseException(Exception::ERROR_overflow, "BigReal is too large to conver to an Integer.");
        }
        pint_t res = mpfr_get_si(this->number, DEFAULT_RND);
        return res;
    }
    
    void BigReal::FromReal(preal_t n)
    {
        mpfr_set_d(this->number, n, DEFAULT_RND);
    }
    
    void BigReal::Constructor(Engine* eng, Type* obj_type, Value& res)
    {
        BigReal* bn = BigReal::Create(eng, obj_type);
        res.Set(bn);
    }
    
    int BigReal::Comp(Value const& right) const
    {
        int res = 0;
        if (right.IsDerivedFrom(BigReal::StaticGetClass()))
        {
            BigReal const* rhs = static_cast<BigReal const*>(right.val.object);
            res = mpfr_cmp(this->number, rhs->number);
        }
        else if (right.IsReal())
        {
            preal_t rhs = right.val.real;
            res = mpfr_cmp_d(this->number, rhs);
        }
        else if (right.IsInteger())
        {
            pint_t rhs = right.val.integer;
            res = mpfr_cmp_si(this->number, rhs);
        }
        else if (right.IsDerivedFrom(BigInteger::StaticGetClass()))
        {
            BigInteger const* rhs = static_cast<BigInteger const*>(right.val.object);
            res = mpfr_cmp_z(this->number, rhs->number);
        }
        else
        {
            GCPAUSE_NORUN(this->engine);
            String* typestr = engine->GetTypenameOf(right);
            RaiseException(Exception::ERROR_type, "Attempt to call BigReal.opComp with right hand operand of type %s.",
                typestr->GetBuffer());
        }
        return res;
    }
    
    bool BigReal::Equals(Value const& right) const
    {
        int res = 0;
        if (right.IsDerivedFrom(BigReal::StaticGetClass()))
        {
            BigReal const* rhs = static_cast<BigReal const*>(right.val.object);
            res = mpfr_cmp(this->number, rhs->number);
            return res == 0;
        }
        else if (right.IsReal())
        {
            preal_t rhs = right.val.real;
            res = mpfr_cmp_d(this->number, rhs);
            return res == 0;
        }
        else if (right.IsInteger())
        {
            pint_t rhs = right.val.integer;
            res = mpfr_cmp_si(this->number, rhs);
            return res == 0;
        }
        else if (right.IsDerivedFrom(BigInteger::StaticGetClass()))
        {
            BigInteger const* rhs = static_cast<BigInteger const*>(right.val.object);
            res = mpfr_cmp_z(this->number, rhs->number);
            return res == 0;
        }
        return false;
    }
    
    bool BigReal::NotEquals(Value const& right) const
    {
        return !this->Equals(right);
    }
    
    BigReal* BigReal::DoBinaryOp(Value const& right, 
                        const char* ovrName,
                        mpfr_fn_t mpfr_fn,
                        mpfr_d_fn_t mpfr_d_fn,
                        mpfr_si_fn_t mpfr_si_fn,
                        mpfr_z_fn_t mpfr_z_fn) const
    {
        BigReal* res = BigReal::Create(this->engine, this->GetType());
        if (right.IsDerivedFrom(BigReal::StaticGetClass()))
        {
            BigReal const* rhs = static_cast<BigReal const*>(right.val.object);
            mpfr_fn(res->number, this->number, rhs->number, DEFAULT_RND);
        }
        else if (right.IsReal())
        {
            preal_t rhs = right.val.real;
            mpfr_d_fn(res->number, this->number, rhs, DEFAULT_RND);
        }
        else if (right.IsInteger())
        {
            pint_t rhs = right.val.integer;
            mpfr_si_fn(res->number, this->number, rhs, DEFAULT_RND);
        }
        else if (right.IsDerivedFrom(BigInteger::StaticGetClass()))
        {
            BigInteger const* rhs = static_cast<BigInteger const*>(right.val.object);
            mpfr_z_fn(res->number, this->number, rhs->number, DEFAULT_RND);
        }
        else
        {
            GCPAUSE_NORUN(this->engine);
            String* typestr = engine->GetTypenameOf(right);
            RaiseException(Exception::ERROR_type, 
                "Attempt to call BigReal.%s with right hand operand of type %s.",
                ovrName,
                typestr->GetBuffer());
        }
        return res;
    }
    
    BigReal* BigReal::Add(Value const& right) const
    {
        return this->DoBinaryOp(right, "opAdd", mpfr_add, mpfr_add_d, mpfr_add_si, mpfr_add_z);
    }
    
    BigReal* BigReal::Sub(Value const& right) const
    {
        return this->DoBinaryOp(right, "opSub", mpfr_sub, mpfr_sub_d, mpfr_sub_si, mpfr_sub_z);
    }
    
    BigReal* BigReal::Sub_r(Value const& right) const
    {
        BigReal* res = BigReal::Create(this->engine, this->GetType());
        if (right.IsReal())
        {
            preal_t rhs = right.val.real;
            mpfr_d_sub(res->number, rhs, this->number, DEFAULT_RND);
        }
        else if (right.IsInteger())
        {
            pint_t rhs = right.val.integer;
            mpfr_si_sub(res->number, rhs, this->number, DEFAULT_RND);
        }
        else if (right.IsDerivedFrom(BigInteger::StaticGetClass()))
        {
            BigInteger const* rhs = static_cast<BigInteger const*>(right.val.object);
            mpfr_z_sub(res->number, rhs->number, this->number, DEFAULT_RND);
        }
        else
        {
            GCPAUSE_NORUN(this->engine);
            String* typestr = engine->GetTypenameOf(right);
            RaiseException(Exception::ERROR_type, "Attempt to call BigReal.opSub_r with right hand operand of type %s.",
                typestr->GetBuffer());
        }
        return res;
    }
        
    BigReal* BigReal::Mul(Value const& right) const
    {
        return this->DoBinaryOp(right, "opMul", mpfr_mul, mpfr_mul_d, mpfr_mul_si, mpfr_mul_z);
    }
    
    BigReal* BigReal::Div(Value const& right) const
    {
        return this->DoBinaryOp(right, "opDiv", mpfr_div, mpfr_div_d, mpfr_div_si, mpfr_div_z);
    }
    
    BigReal* BigReal::Mod(Value const& right) const
    {
        BigReal* res = BigReal::Create(this->engine, this->GetType());
        if (right.IsDerivedFrom(BigReal::StaticGetClass()))
        {
            BigReal const* rhs = static_cast<BigReal const*>(right.val.object);
            mpfr_fmod(res->number, this->number, rhs->number, DEFAULT_RND);
        }
        else
        {
            GCPAUSE_NORUN(this->engine);
            String* typestr = engine->GetTypenameOf(right);
            RaiseException(Exception::ERROR_type, "Attempt to call BigReal.opMod with right hand operand of type %s.",
                typestr->GetBuffer());
        }
        return res;
    }
    
    BigReal* BigReal::Pow(Value const& right) const
    {
        BigReal* res = BigReal::Create(this->engine, this->GetType());
        if (right.IsDerivedFrom(BigReal::StaticGetClass()))
        {
            BigReal const* rhs = static_cast<BigReal const*>(right.val.object);
            mpfr_pow(res->number, this->number, rhs->number, DEFAULT_RND);
        }
        else if (right.IsInteger())
        {
            pint_t rhs = right.val.integer;
            mpfr_pow_si(res->number, this->number, rhs, DEFAULT_RND);
        }
        else if (right.IsDerivedFrom(BigInteger::StaticGetClass()))
        {
            BigInteger const* rhs = static_cast<BigInteger const*>(right.val.object);
            mpfr_pow_z(res->number, this->number, rhs->number, DEFAULT_RND);
        }
        else
        {
            GCPAUSE_NORUN(this->engine);
            String* typestr = engine->GetTypenameOf(right);
            RaiseException(Exception::ERROR_type, "Attempt to call BigReal.opSub_r with right hand operand of type %s.",
                typestr->GetBuffer());
        }
        return res;
    }
    
    BigReal* BigReal::Neg() const
    {
        BigReal* res = BigReal::Create(this->engine, this->GetType());
        mpfr_neg(res->number, this->number, DEFAULT_RND);
        return res;
    }
    
    bool BigReal::IsIntegral() const
    {
        return mpfr_integer_p(this->number) != 0;
    }
    
    bool BigReal::IsNan() const
    {
        return mpfr_nan_p(this->number) != 0;
    }
    
    bool BigReal::IsInfinite() const
    {
        return mpfr_inf_p(this->number) != 0;
    }
    
    bool BigReal::IsZero() const
    {
        return mpfr_zero_p(this->number) != 0;
    }
    
    bool BigReal::IsRegular() const
    {
        return mpfr_regular_p(this->number) != 0;
    }
    
    pint_t BigReal::Sign() const
    {
        return mpfr_sgn(this->number);
    }
    
    PIKA_IMPL(BigReal)
}

using namespace pika;

int BigReal_toString(Context* ctx, Value& self)
{
    BigReal* bigreal = static_cast<BigReal*>(self.val.object);
    String* res = 0;
    u2 argc = ctx->GetArgCount();
    
    if (argc == 1)
    {
        pint_t radix = ctx->GetIntArg(0);
        res = bigreal->ToString(radix);
    }
    else if (argc == 0)
    {
        res = bigreal->ToString();   
    }
    else
    {
        ctx->WrongArgCount();
    }
    ctx->Push(res);
    return 1;
}

int BigReal_toInteger(Context* ctx, Value& self)
{
    BigReal* bn = static_cast<BigReal*>(self.val.object);
    pint_t res = bn->ToInteger();
    ctx->Push(res);
    return 1;
}

int BigReal_toReal(Context* ctx, Value& self)
{
    BigReal* bn = static_cast<BigReal*>(self.val.object);
    preal_t res = bn->ToReal();
    ctx->Push(res);
    return 1;
}

int BigReal_opComp(Context* ctx, Value& self)
{
    BigReal* bn = static_cast<BigReal*>(self.val.object);
    Value& arg = ctx->GetArg(0);
    pint_t res = (pint_t)bn->Comp(arg);
    ctx->Push(res);
    return 1;
}

int BigReal_opEq(Context* ctx, Value& self)
{
    BigReal* bn = static_cast<BigReal*>(self.val.object);
    Value& arg = ctx->GetArg(0);
    bool res = bn->Equals(arg);
    ctx->PushBool(res);
    return 1;
}

int BigReal_opNe(Context* ctx, Value& self)
{
    BigReal* bn = static_cast<BigReal*>(self.val.object);
    Value& arg = ctx->GetArg(0);
    bool res = bn->NotEquals(arg);
    ctx->PushBool(res);
    return 1;
}

#define MAKE_BINARY_OP(FN, METHOD) \
    int BigReal_##FN(Context* ctx, Value& self)\
    {\
        BigReal* lhs = static_cast<BigReal*>(self.val.object);\
        Value& arg = ctx->GetArg(0);\
        BigReal* res = lhs->METHOD(arg);\
        ctx->Push(res);\
        return 1;\
    }

MAKE_BINARY_OP(opAdd,   Add)
MAKE_BINARY_OP(opMul,   Mul)
MAKE_BINARY_OP(opDiv,   Div)
MAKE_BINARY_OP(opSub,   Sub)
MAKE_BINARY_OP(opSub_r, Sub_r)
MAKE_BINARY_OP(opMod,   Mod)
MAKE_BINARY_OP(opPow,   Pow)

#define MAKE_UNARY_OP(FN, METHOD) \
    int BigReal_##FN(Context* ctx, Value& self)\
    {\
        BigReal* lhs = static_cast<BigReal*>(self.val.object);\
        BigReal* res = lhs->METHOD();\
        ctx->Push(res);\
        return 1;\
    }

MAKE_UNARY_OP(opNeg, Neg)
    
#define MAKE_UNARY_BOOL_METHOD(NAME, METHOD) \
    int BigReal_##NAME(Context* ctx, Value& self)\
    {\
        BigReal* bi = static_cast<BigReal*>(self.val.object);\
        bool res = bi->METHOD();\
        ctx->PushBool(res);\
        return 1;\
    }

MAKE_UNARY_BOOL_METHOD(isIntegral,  IsIntegral)
MAKE_UNARY_BOOL_METHOD(isNan,       IsNan)
MAKE_UNARY_BOOL_METHOD(isInfinite,  IsInfinite)
MAKE_UNARY_BOOL_METHOD(isZero,      IsZero)
MAKE_UNARY_BOOL_METHOD(isRegular,   IsRegular)


int BigReal_sign(Context* ctx, Value& self)
{
    BigReal* bi = static_cast<BigReal*>(self.val.object);
    pint_t res = bi->Sign();
    ctx->Push(res);
    return 1;
}
        
void Initialize_BigReal(Package* bignum, Engine* eng)
{    
    String* BigReal_String = eng->GetString(BIGREAL_TYPENAME);
    
    Type* BigReal_Type = Type::Create(eng, BigReal_String, eng->Object_Type, BigReal::Constructor, bignum);
    
    static RegisterFunction BigReal_Methods[] = {
        { "toString",   BigReal_toString,   0, DEF_VAR_ARGS, 0 },
        { "opAdd",      BigReal_opAdd,      1, DEF_STRICT, 0 },
        { "opAdd_r",    BigReal_opAdd,      1, DEF_STRICT, 0 },
        { "opMul",      BigReal_opMul,      1, DEF_STRICT, 0 },
        { "opMul_r",    BigReal_opMul,      1, DEF_STRICT, 0 },
        { "opDiv",      BigReal_opDiv,      1, DEF_STRICT, 0 },
        { "opMod",      BigReal_opMod,      1, DEF_STRICT, 0 },
        { "opSub",      BigReal_opSub,      1, DEF_STRICT, 0 },
        { "opSub_r",    BigReal_opSub_r,    1, DEF_STRICT, 0 },
        { "opPow",      BigReal_opPow,      1, DEF_STRICT, 0 },
        { "opNeg",      BigReal_opNeg,      0, DEF_STRICT, 0 },
        { "opEq",       BigReal_opEq,       1, DEF_STRICT, 0 },
        { "opNe",       BigReal_opNe,       1, DEF_STRICT, 0 },
        { "opComp",     BigReal_opComp,     1, DEF_STRICT, 0 },
        { "toInteger",  BigReal_toInteger,  0, DEF_STRICT, 0 },  
        { "toReal",     BigReal_toReal,     0, DEF_STRICT, 0 },
        { "nan?",       BigReal_isNan,      0, DEF_STRICT, 0 },
        { "infinite?",  BigReal_isInfinite, 0, DEF_STRICT, 0 },
        { "zero?",      BigReal_isZero,     0, DEF_STRICT, 0 },
        { "integral?",  BigReal_isIntegral, 0, DEF_STRICT, 0 },
        { "regular?",   BigReal_isRegular,  0, DEF_STRICT, 0 },
    };
    
    static RegisterProperty BigReal_Properties[] = {
       { "sign", BigReal_sign, "getSign", 0, 0, false, 0, 0, 0 }, 
    };
    
    BigReal_Type->EnterProperties(BigReal_Properties, countof(BigReal_Properties));
    BigReal_Type->EnterMethods(BigReal_Methods, countof(BigReal_Methods));
    
    bignum->SetSlot(BigReal_String, BigReal_Type);
    eng->AddBaseType(BigReal_String, BigReal_Type);
}
