/*
 *  PBigReal.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_BIGREAL_HEADER
#define PIKA_BIGREAL_HEADER

typedef int    (mpfr_fn_t)(mpfr_ptr, mpfr_srcptr, mpfr_srcptr, mpfr_rnd_t);
typedef int  (mpfr_d_fn_t)(mpfr_ptr, mpfr_srcptr, double,      mpfr_rnd_t);
typedef int (mpfr_si_fn_t)(mpfr_ptr, mpfr_srcptr, long int,    mpfr_rnd_t);
typedef int  (mpfr_z_fn_t)(mpfr_ptr, mpfr_srcptr, mpz_srcptr,  mpfr_rnd_t);

namespace pika {

    class BigReal : public Object {
        PIKA_DECL(BigReal, Object)
            
        BigReal* DoBinaryOp(Value const& right, 
                            const char* ovrName,
                            mpfr_fn_t mpfr_fn,
                            mpfr_d_fn_t mpfr_d_fn,
                            mpfr_si_fn_t mpfr_si_fn,
                            mpfr_z_fn_t mpfr_z_fn) const;
    public:
        BigReal(Engine* eng, Type* typeObj);
        mpfr_t number;
        
        virtual ~BigReal();
            
        virtual void Init(Context* ctx);
            
        static BigReal* Create(Engine* eng, Type* type);
        static void Constructor(Engine* eng, Type* obj_type, Value& res);
        
        virtual String* ToString();
        virtual String* ToString(pint_t radix);
        virtual preal_t ToReal() const;
        virtual pint_t ToInteger() const;
        
        virtual bool ToBoolean() const
        {
            return this->IsRegular();
        }
        
        int Comp(Value const& right) const;
        bool Equals(Value const& right) const;
        bool NotEquals(Value const& right) const;
                
        //
        BigReal* Add(Value const& right) const;
        BigReal* Sub(Value const& right) const;
        BigReal* Sub_r(Value const& right) const;
        BigReal* Mul(Value const& right) const;
        BigReal* Div(Value const& right) const;
        BigReal* Mod(Value const& right) const;
        
        BigReal* Pow(Value const& right) const;
        
        BigReal* Neg() const;
        
        void FromReal(preal_t n);
        
        pint_t Sign() const;
        
        bool IsNan() const;
        bool IsInfinite() const;
        bool IsIntegral() const;
        bool IsZero() const;
        bool IsRegular() const;
    };
    
}// pika

#endif
