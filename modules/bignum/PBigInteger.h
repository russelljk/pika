#ifndef PIKA_BIGINTEGER_HEADER
#define PIKA_BIGINTEGER_HEADER

namespace pika {

typedef u1 digit_t;
/*
toBoolean
opPow
opEq -> mpz_cmp, mpz_cmp_si, mpz_cmp_d
opNe
opLt
opGt
opLte
opGte

opBitAnd -> mpz_and
opBitOr -> mpz_ior
opBitXor -> mpz_xor
opLsh -> mpz_mul_2exp
opRsh -> mpz_fdiv_q_2exp
opURsh -> ?
opBitNot -> mpz_com

getSign -> mpz_sgn

opIncr BigInteger + 1
opDecr BigInteger - 1

opIDiv    
*/

class BigInteger : public Object
{
    PIKA_DECL(BigInteger, Object)
public:
    BigInteger(Engine* eng, Type* typeObj);
    
    Buffer<digit_t> digits;
    bool positive;
    mpz_t number;
    
    virtual ~BigInteger();
    virtual String* ToString();
    virtual String* ToString(pint_t radix);
    
    virtual void Init(Context* ctx);
    
    preal_t ToReal() const;
    pint_t ToInteger() const;
    
    static BigInteger*  Create(Engine* eng, Type* type);
    static void         Constructor(Engine* eng, Type* obj_type, Value& res);
    
    int Comp(Value const& right) const;
    
    BigInteger* Add(Value const& right) const;
    BigInteger* Sub(Value const& right) const;
    BigInteger* Mul(Value const& right) const;
    BigInteger* Div(Value const& right) const;
    BigInteger* Mod(Value const& right) const;
    BigInteger* Pow(Value const& right) const;
        
    BigInteger* BitAnd(Value const& right) const;
    BigInteger* BitOr(Value const& right) const;
    BigInteger* BitXor(Value const& right) const;
    
    BigInteger* Lsh(Value const& right) const;
    BigInteger* Rsh(Value const& right) const;
    
    BigInteger* BitNot() const;
    BigInteger* Neg() const;
    pint_t      Sign() const;
    void FromInt(pint_t n);
};

}
#endif
