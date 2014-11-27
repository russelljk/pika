#include "PBigNum.h"

#if defined(PIKA_WIN)

#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH: break;
    }
    return TRUE;
}

#endif

#define BIGINTEGER_TYPENAME "BigInteger"

void (*gmp_free_func)(void*, size_t);

namespace pika {    
    BigInteger::BigInteger(Engine* eng, Type* typeObj) : Object(eng, typeObj)
    {
        mpz_init(this->number);
    }
    
    BigInteger::~BigInteger()
    {
        mpz_clear(this->number);
    }

    BigInteger* BigInteger::Create(Engine* eng, Type* type)
    {
        GCPAUSE_NORUN(eng);
        BigInteger* bigint = 0;
        GCNEW(eng, BigInteger, bigint, (eng, type));
        return bigint;
    }
    
    void BigInteger::Init(Context* ctx)
    {
        u2 argc = ctx->GetArgCount();
        if (argc == 1)
        {
            Value& arg = ctx->GetArg(0);
            if (arg.IsInteger())
            {
                pint_t n = arg.val.integer;
                this->FromInt(n);                
            }
            else if (arg.IsReal())
            {
                preal_t n = arg.val.real;
                mpz_init_set_d(this->number, n);  
            }
            else if (arg.IsDerivedFrom(BigInteger::StaticGetClass()))
            {
                BigInteger* bi =  static_cast<BigInteger*>(arg.val.object);
                mpz_init_set(this->number, bi->number);
            }
#ifdef  HAVE_MPFR
            else if (arg.IsDerivedFrom(BigReal::StaticGetClass()))
            {
                BigReal* br =  static_cast<BigReal*>(arg.val.object);
                mpfr_get_z(this->number, br->number, DEFAULT_RND);
            }
#endif
            else
            {
                ctx->GetIntArg(0);
            }
        }
        else if (argc != 0)
        {
            ctx->WrongArgCount();
        }
    }
    
    void BigInteger::FromInt(pint_t n)
    {
        mpz_init_set_si(this->number, n);
    }
    
    void BigInteger::Constructor(Engine* eng, Type* obj_type, Value& res)
    {
        BigInteger* bn = BigInteger::Create(eng, obj_type);
        res.Set(bn);
    }
        
    String* BigInteger::ToString()
    {
        return this->ToString(10);
    }
    
    String* BigInteger::ToString(pint_t radix)
    {
        if (radix < 2 or radix > 36)
        {
            RaiseException(Exception::ERROR_type, "Attempt to call BigInteger.toString with an invalid radix "PINT_FMT". Radix must be >= 2 and <= 36.", radix);
        }
        
        String* res = 0;
        char* gmpstr = (char*)Pika_calloc(mpz_sizeinbase(this->number, radix) + 2, sizeof(char));
        const char* resstr = mpz_get_str(gmpstr, radix, this->number);
        if (gmpstr)
        {
            res = engine->GetString(gmpstr);
        }
        else
        {
            res = engine->emptyString;
        }
        Pika_free(gmpstr);
        return res;
    }
    
    BigInteger* BigInteger::Add(Value const& right) const
    {
        BigInteger* res = BigInteger::Create(this->engine, this->GetType());
        if (right.IsDerivedFrom(BigInteger::StaticGetClass()))
        {
            BigInteger const* rhs = static_cast<BigInteger const*>(right.val.object);
            mpz_add(res->number, this->number, rhs->number);
        }
        else if (right.IsInteger())
        {
            pint_t rhs = right.val.integer;
            if (rhs >= 0)
            {
                mpz_add_ui(res->number, this->number, rhs);                
            }
            else
            {
                mpz_sub_ui(res->number, this->number, rhs);
            }
        }
        else
        {
            GCPAUSE_NORUN(this->engine);
            String* typestr = engine->GetTypenameOf(right);
            RaiseException(Exception::ERROR_type, "Attempt to call BigInteger.opAdd with right hand operand of type %s.",
                typestr->GetBuffer());
        }
        return res;
    }
    
    BigInteger* BigInteger::Mul(Value const& right) const
    {
         BigInteger* res = BigInteger::Create(this->engine, this->GetType());
         if (right.IsDerivedFrom(BigInteger::StaticGetClass()))
         {
             BigInteger const* rhs = static_cast<BigInteger const*>(right.val.object);
             mpz_mul(res->number, this->number, rhs->number);
         }
         else if (right.IsInteger())
         {
             pint_t rhs = right.val.integer;
             mpz_mul_si(res->number, this->number, rhs);  
         }
         else
         {
             GCPAUSE_NORUN(this->engine);
             String* typestr = engine->GetTypenameOf(right);
             RaiseException(Exception::ERROR_type, "Attempt to call BigInteger.opMul with right hand operand of type %s.",
                 typestr->GetBuffer());
         }
         return res;
    }
    
    preal_t BigInteger::ToReal() const
    {
        return (preal_t)mpz_get_d(this->number);
    }
        
    pint_t BigInteger::ToInteger() const
    {
        if (!mpz_fits_slong_p(this->number))
        {
            RaiseException(Exception::ERROR_overflow, "BigInteger is too large to conver to an Integer.");
        }
        pint_t res = mpz_get_si(this->number);
        return res;
    }
    
    BigInteger* BigInteger::Div(Value const& right) const
    {
         BigInteger* res = BigInteger::Create(this->engine, this->GetType());
         if (right.IsDerivedFrom(BigInteger::StaticGetClass()))
         {
             BigInteger const* rhs = static_cast<BigInteger const*>(right.val.object);
             mpz_fdiv_q(res->number, this->number, rhs->number);
         }
         else if (right.IsInteger())
         {
             pint_t rhs = right.val.integer;
             if (rhs > 0)
             {
                 mpz_fdiv_q_ui(res->number, this->number, rhs);    
             }
             else if (rhs == 0)
             {
                 RaiseException(Exception::ERROR_dividebyzero, "Attempt to divide BigInteger by zero.");
             }
             else
             {
                 mpz_fdiv_q_ui(res->number, this->number, -rhs);
             }
         }
         else
         {
             GCPAUSE_NORUN(this->engine);
             String* typestr = engine->GetTypenameOf(right);
             RaiseException(Exception::ERROR_type, "Attempt to call BigInteger.opDiv with right hand operand of type %s.",
                 typestr->GetBuffer());
         }
         return res;
    }
    
    BigInteger* BigInteger::Mod(Value const& right) const
    {
         BigInteger* res = BigInteger::Create(this->engine, this->GetType());
         if (right.IsDerivedFrom(BigInteger::StaticGetClass()))
         {
             BigInteger const* rhs = static_cast<BigInteger const*>(right.val.object);
             mpz_mod(res->number, this->number, rhs->number);
         }
         else if (right.IsInteger())
         {
             pint_t rhs = right.val.integer;
             if (rhs > 0)
             {
                 mpz_mod_ui(res->number, this->number, rhs);    
             }
             else if (rhs == 0)
             {
                 RaiseException(Exception::ERROR_dividebyzero, "Attempt to divide BigInteger by zero.");
             }
             else
             {
                 mpz_mod_ui(res->number, this->number, -rhs);
             }
         }
         else
         {
             GCPAUSE_NORUN(this->engine);
             String* typestr = engine->GetTypenameOf(right);
             RaiseException(Exception::ERROR_type, "Attempt to call BigInteger.opMod with right hand operand of type %s.",
                 typestr->GetBuffer());
         }
         return res;
    }
        
    BigInteger* BigInteger::Sub(Value const& right) const
    {
        BigInteger* res = BigInteger::Create(this->engine, this->GetType());
        if (right.IsDerivedFrom(BigInteger::StaticGetClass()))
        {
            BigInteger const* rhs = static_cast<BigInteger const*>(right.val.object);
            mpz_sub(res->number, this->number, rhs->number);
        }
        else if (right.IsInteger())
        {
            pint_t rhs = right.val.integer;
            if (rhs >= 0)
            {
                mpz_sub_ui(res->number, this->number, rhs);                
            }
            else
            {
                mpz_add_ui(res->number, this->number, -rhs);
            }
        }
        else
        {
            GCPAUSE_NORUN(this->engine);
            String* typestr = engine->GetTypenameOf(right);
            RaiseException(Exception::ERROR_type, "Attempt to call BigInteger.opSub with right hand operand of type %s.",
                typestr->GetBuffer());
        }
        return res;
    }
    
    BigInteger* BigInteger::Neg() const
    {
        BigInteger* res = BigInteger::Create(engine, GetType());
        mpz_neg(res->number, this->number);
        return res;
    }
    
    int BigInteger::Comp(Value const& right) const
    {
        int res = 0;
        if (right.IsDerivedFrom(BigInteger::StaticGetClass()))
        {
            BigInteger const* rhs = static_cast<BigInteger const*>(right.val.object);
            res = mpz_cmp(this->number, rhs->number);
        }
        else if (right.IsInteger())
        {
            pint_t rhs = right.val.integer;
            res = mpz_cmp_si(this->number, rhs);
        }
        else if (right.IsReal())
        {
            preal_t rhs = right.val.real;
            res = mpz_cmp_d(this->number, rhs);
        }
        else
        {
            GCPAUSE_NORUN(this->engine);
            String* typestr = engine->GetTypenameOf(right);
            RaiseException(Exception::ERROR_type, "Attempt to compare BigInteger with right hand operand of type %s.",
                typestr->GetBuffer());
        }
        return res;
    }
    
    BigInteger* BigInteger::BitAnd(Value const& right) const
    {
        BigInteger* res = BigInteger::Create(this->engine, this->GetType());
        if (right.IsDerivedFrom(BigInteger::StaticGetClass()))
        {
            BigInteger const* rhs = static_cast<BigInteger const*>(right.val.object);
            mpz_and(res->number, this->number, rhs->number);
        }
        else
        {
            GCPAUSE_NORUN(this->engine);
            String* typestr = engine->GetTypenameOf(right);
            RaiseException(Exception::ERROR_type, "Attempt to call BigInteger.opBitAnd with right hand operand of type %s.",
                typestr->GetBuffer());
        }
        return res;
    }
    
    BigInteger* BigInteger::BitOr(Value const& right) const
    {
        BigInteger* res = BigInteger::Create(this->engine, this->GetType());
        if (right.IsDerivedFrom(BigInteger::StaticGetClass()))
        {
            BigInteger const* rhs = static_cast<BigInteger const*>(right.val.object);
            mpz_ior(res->number, this->number, rhs->number);
        }
        else
        {
            GCPAUSE_NORUN(this->engine);
            String* typestr = engine->GetTypenameOf(right);
            RaiseException(Exception::ERROR_type, "Attempt to call BigInteger.opBitOr with right hand operand of type %s.",
                typestr->GetBuffer());
        }
        return res;
    }
    
    BigInteger* BigInteger::BitXor(Value const& right) const
    {
        BigInteger* res = BigInteger::Create(this->engine, this->GetType());
        if (right.IsDerivedFrom(BigInteger::StaticGetClass()))
        {
            BigInteger const* rhs = static_cast<BigInteger const*>(right.val.object);
            mpz_xor(res->number, this->number, rhs->number);
        }
        else
        {
            GCPAUSE_NORUN(this->engine);
            String* typestr = engine->GetTypenameOf(right);
            RaiseException(Exception::ERROR_type, "Attempt to call BigInteger.opBitXor with right hand operand of type %s.",
                typestr->GetBuffer());
        }
        return res;
    }
    
    BigInteger* BigInteger::BitNot() const
    {
        BigInteger* res = BigInteger::Create(this->engine, this->GetType());
        mpz_com(res->number, this->number);
        return res;
    }
    
    BigInteger* BigInteger::Lsh(Value const& right) const
    {
        BigInteger* res = BigInteger::Create(this->engine, this->GetType());
        pint_t rhs = 0;
        
        if (right.IsInteger())
        {
            rhs = right.val.integer;
        }
        else if (right.IsDerivedFrom(BigInteger::StaticGetClass()))
        {
            BigInteger* rhs_bi = static_cast<BigInteger*>(right.val.object);
            rhs = rhs_bi->ToInteger();
        }
        else
        {
            GCPAUSE_NORUN(this->engine);
            String* typestr = engine->GetTypenameOf(right);
            RaiseException(Exception::ERROR_type, "Attempt to call BigInteger.opLsh with right hand operand of type %s.",
                typestr->GetBuffer());
        }
        
        if (rhs > 0)
        {
            mpz_mul_2exp(res->number, this->number, rhs);
        }
        else
        {
            RaiseException(Exception::ERROR_type, "Attempt to call BigInteger.opLsh with negative right hand operator");
        }
        return res;
    }
    
    BigInteger* BigInteger::Rsh(Value const& right) const
    {
        BigInteger* res = BigInteger::Create(this->engine, this->GetType());
        pint_t rhs = 0;
        
        if (right.IsInteger())
        {
            rhs = right.val.integer;
        }
        else if (right.IsDerivedFrom(BigInteger::StaticGetClass()))
        {
            BigInteger* rhs_bi = static_cast<BigInteger*>(right.val.object);
            rhs = rhs_bi->ToInteger();
        }
        else
        {
            GCPAUSE_NORUN(this->engine);
            String* typestr = engine->GetTypenameOf(right);
            RaiseException(Exception::ERROR_type, "Attempt to call BigInteger.opRsh with right hand operand of type %s.",
                typestr->GetBuffer());
        }
        
        if (rhs > 0)
        {
            mpz_fdiv_q_2exp(res->number, this->number, rhs);
        }
        else
        {
            RaiseException(Exception::ERROR_type, "Attempt to call BigInteger.opRsh with negative right hand operator");
        }
        return res;
    }
    
    BigInteger* BigInteger::Pow(Value const& right) const
    {
        BigInteger* res = BigInteger::Create(this->engine, this->GetType());
        pint_t rhs = 0;
        
        if (right.IsInteger())
        {
            rhs = right.val.integer;
        }
        else if (right.IsDerivedFrom(BigInteger::StaticGetClass()))
        {
            BigInteger* rhs_bi = static_cast<BigInteger*>(right.val.object);
            rhs = rhs_bi->ToInteger();
        }
        else
        {
            GCPAUSE_NORUN(this->engine);
            String* typestr = engine->GetTypenameOf(right);
            RaiseException(Exception::ERROR_type, "Attempt to call BigInteger.opPow with right hand operand of type %s.",
                typestr->GetBuffer());
        }
        
        if (rhs > 0)
        {
            mpz_pow_ui(res->number, this->number, rhs);
        }
        else
        {
            RaiseException(Exception::ERROR_type, "Attempt to call BigInteger.opPow with negative right hand operator");
        }
        
        return res;
    }
    
    pint_t BigInteger::Sign() const
    {
        pint_t sign = mpz_sgn(this->number);
        return sign;
    }
    
    PIKA_IMPL(BigInteger);
}

using namespace pika;

int BigInteger_toInteger(Context* ctx, Value& self)
{
    BigInteger* bn = static_cast<BigInteger*>(self.val.object);
    pint_t res = bn->ToInteger();
    ctx->Push(res);
    return 1;
}

int BigInteger_toReal(Context* ctx, Value& self)
{
    BigInteger* bn = static_cast<BigInteger*>(self.val.object);
    preal_t res = bn->ToReal();
    ctx->Push(res);
    return 1;
}

int BigInteger_opAdd(Context* ctx, Value& self)
{
    BigInteger* lhs = static_cast<BigInteger*>(self.val.object);
    Value& arg = ctx->GetArg(0);
    BigInteger* res = lhs->Add(arg);
    ctx->Push(res);
    return 1;
}

int BigInteger_opDiv(Context* ctx, Value& self)
{
    BigInteger* lhs = static_cast<BigInteger*>(self.val.object);
    Value& arg = ctx->GetArg(0);
    BigInteger* res = lhs->Div(arg);
    ctx->Push(res);
    return 1;
}

int BigInteger_opMod(Context* ctx, Value& self)
{
    BigInteger* lhs = static_cast<BigInteger*>(self.val.object);
    Value& arg = ctx->GetArg(0);
    BigInteger* res = lhs->Mod(arg);
    ctx->Push(res);
    return 1;
}

int BigInteger_opMul(Context* ctx, Value& self)
{
    BigInteger* lhs = static_cast<BigInteger*>(self.val.object);
    Value& arg = ctx->GetArg(0);
    BigInteger* res = lhs->Mul(arg);
    ctx->Push(res);
    return 1;
}

int BigInteger_opSub(Context* ctx, Value& self)
{
    BigInteger* lhs = static_cast<BigInteger*>(self.val.object);
    Value& arg = ctx->GetArg(0);
    BigInteger* res = lhs->Sub(arg);
    ctx->Push(res);
    return 1;
}

int BigInteger_opComp(Context* ctx, Value& self)
{
    BigInteger* lhs = static_cast<BigInteger*>(self.val.object);
    Value& arg = ctx->GetArg(0);
    pint_t res = lhs->Comp(arg);
    ctx->Push(res);
    return 1;
}

int BigInteger_opSub_r(Context* ctx, Value& self)
{
    /* If the right hand side is a BigInteger then opSub will be called instead.
     * Therefore we can assume it is another type, since the only supported type
     * is Integer we check.
        
        Integer - BigInteger
        = Integer + (-BigInteger) -> convert subtraction to addition.
        = -BigInteger + Integer   -> Rearrange since addition is associative.
     */
    
    BigInteger* bigint = static_cast<BigInteger*>(self.val.object);
    pint_t lhs = ctx->GetIntArg(0);
    BigInteger* rhs = bigint->Neg();
    BigInteger* res = rhs->Add(ctx->GetArg(0));
    ctx->Push(res);
    return 1;
}

#define MAKE_BINARY_OP(FN, METHOD) \
    int BigInteger_##FN(Context* ctx, Value& self)\
    {\
        BigInteger* lhs = static_cast<BigInteger*>(self.val.object);\
        Value& arg = ctx->GetArg(0);\
        BigInteger* res = lhs->METHOD(arg);\
        ctx->Push(res);\
        return 1;\
    }

MAKE_BINARY_OP(opBitAnd, BitAnd)
    
MAKE_BINARY_OP(opBitOr, BitOr)
    
MAKE_BINARY_OP(opBitXor, BitXor)

MAKE_BINARY_OP(opLsh, Lsh)

MAKE_BINARY_OP(opRsh, Rsh)

MAKE_BINARY_OP(opPow, Pow)

#define MAKE_UNARY_OP(FN, METHOD) \
    int BigInteger_##FN(Context* ctx, Value& self)\
    {\
        BigInteger* lhs = static_cast<BigInteger*>(self.val.object);\
        BigInteger* res = lhs->METHOD();\
        ctx->Push(res);\
        return 1;\
    }

MAKE_UNARY_OP(opNeg, Neg)

MAKE_UNARY_OP(opBitNot, BitNot)


int BigInteger_toString(Context* ctx, Value& self)
{
    BigInteger* bigint = static_cast<BigInteger*>(self.val.object);
    String* res = 0;
    u2 argc = ctx->GetArgCount();
    
    if (argc == 1)
    {
        pint_t radix = ctx->GetIntArg(0);
        res = bigint->ToString(radix);
    }
    else if (argc == 0)
    {
        res = bigint->ToString();   
    }
    else
    {
        ctx->WrongArgCount();
    }
    ctx->Push(res);
    return 1;
}

int BigInteger_sign(Context* ctx, Value& self)
{
    BigInteger* bi = static_cast<BigInteger*>(self.val.object);
    pint_t res = bi->Sign();
    ctx->Push(res);
    return 1;
}

/*
    For rhs comparison operators we can just use the inverse
    
    opLt_r => MAKE_COMP_REVERSE(opLt_r, >=)
*/
void Initialize_BigInteger(Package* bignum, Engine* eng)
{
    mp_get_memory_functions (NULL, NULL, &gmp_free_func);
    
    String* BigInteger_String = eng->GetString(BIGINTEGER_TYPENAME);
    
    Type* BigInteger_Type = Type::Create(eng, BigInteger_String, eng->Object_Type, BigInteger::Constructor, bignum);
    
    static RegisterFunction BigInteger_Methods[] = {
        { "toString",   BigInteger_toString,      0, DEF_VAR_ARGS, 0 },
        { "opAdd",      BigInteger_opAdd,         1, DEF_STRICT, 0 },
        { "opAdd_r",    BigInteger_opAdd,         1, DEF_STRICT, 0 },
        { "opMul",      BigInteger_opMul,         1, DEF_STRICT, 0 },
        { "opMul_r",    BigInteger_opMul,         1, DEF_STRICT, 0 },
        { "opDiv",      BigInteger_opDiv,         1, DEF_STRICT, 0 },
        { "opMod",      BigInteger_opMod,         1, DEF_STRICT, 0 },
        { "opSub",      BigInteger_opSub,         1, DEF_STRICT, 0 },
        { "opSub_r",    BigInteger_opSub_r,       1, DEF_STRICT, 0 },
        { "opPow",      BigInteger_opPow,         1, DEF_STRICT, 0 },
        { "opLsh",      BigInteger_opLsh,         1, DEF_STRICT, 0 },
        { "opRsh",      BigInteger_opRsh,         1, DEF_STRICT, 0 },
        { "opBitAnd",   BigInteger_opBitAnd,      1, DEF_STRICT, 0 },
        { "opBitOr",    BigInteger_opBitOr,       1, DEF_STRICT, 0 },
        { "opBitXor",   BigInteger_opBitXor,      1, DEF_STRICT, 0 },
        { "opBitNot",   BigInteger_opBitNot,      0, DEF_STRICT, 0 },
        { "opNeg",      BigInteger_opNeg,         0, DEF_STRICT, 0 },
        { "opComp",     BigInteger_opComp,        1, DEF_STRICT, 0 },
        { "toInteger",  BigInteger_toInteger,     0, DEF_STRICT, 0 },  
        { "toReal",     BigInteger_toReal,        0, DEF_STRICT, 0 },
    };

    static RegisterProperty BigInteger_Properties[] = {
       { "sign", BigInteger_sign, "getSign", 0, 0, false, 0, 0, 0 }, 
    };
    
    BigInteger_Type->EnterProperties(BigInteger_Properties, countof(BigInteger_Properties));
    BigInteger_Type->EnterMethods(BigInteger_Methods, countof(BigInteger_Methods));
    
    /*
        bignum.abs
        bignum.sqrt
        bignum.exp
    */    
    bignum->SetSlot(BigInteger_String, BigInteger_Type);
    eng->AddBaseType(BigInteger_String, BigInteger_Type);
}
