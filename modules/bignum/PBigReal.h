#ifndef PIKA_BIGREAL_HEADER
#define PIKA_BIGREAL_HEADER

namespace pika {

    class BigReal : public Object {
        PIKA_DECL(BigReal, Object)
    public:
        BigReal(Engine* eng, Type* typeObj);
        mpfr_t number;
        
        virtual ~BigReal();
            
        virtual void Init(Context* ctx);
            
        static BigReal* Create(Engine* eng, Type* type);
        static void Constructor(Engine* eng, Type* obj_type, Value& res);
        
        virtual String* ToString();
        virtual String* ToString(pint_t radix);
        preal_t ToReal() const;
        pint_t ToInteger() const;
        
        //
        int Comp(Value const& right) const;
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
