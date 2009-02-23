/*
 *  PContext_Ops.inl
 *  See Copyright Notice in Pika.h
 */
// TODO: Change xxx_num(T&,T&) to xxx_num(T&,T&,Value& res)
//       remove gCompiler line 22 code block
//       remove gContext ArithBinary? OP_div code.

////////////////////////////////////////////////////////////////////////////////////////////////////
#define USE_C_ARITH
template<typename T> INLINE void add_num(T& a, T& b) { a += b; }
template<typename T> INLINE void sub_num(T& a, T& b) { a -= b; }
template<typename T> INLINE void mul_num(T& a, T& b) { a *= b; }
template<typename T> INLINE void div_num(T& a, T& b) { a /= b; }
template<typename T> INLINE void mod_num(T& a, T& b) { a %= b; }
template<typename T> INLINE void pow_num(T& a, T& b) { a = (T)Pow((preal_t)a, (preal_t)b); }

template<typename T> INLINE void inc_num(T& a) { a += (T)1; }
template<typename T> INLINE void dec_num(T& a) { a -= (T)1; }
template<typename T> INLINE void neg_num(T& a) { a = -a; }
template<typename T> INLINE void pos_num(T& a) { a = +a; }

#ifdef PIKA_ARITH_OVERFLOW_CHECK

template<>
INLINE void add_num<pint_t>(pint_t& a, pint_t& b)
{
    pint_t res = a + b;
    if ((a ^ b) >= 0)
    {
        if ((res ^ a) < 0)
        {
            RaiseException(Exception::ERROR_arithmetic, "Operator +: integer overflow.");
        }
    }
    a = res;
}

template<>
INLINE void sub_num<pint_t>(pint_t& a, pint_t& b)
{
    pint_t res = a - b;

    if ((a ^ b) < 0)
    {
        if ((res ^ a) < 0)
        {
            RaiseException(Exception::ERROR_arithmetic, "Operator -: integer underflow.");
        }
    }

    a = res;
}

#ifndef PIKA_64BIT_INT

// mul_num with overflow check is broken for 64bit integers.
// in order to use the same alogorithm we need to use 128bit integers

template<>
INLINE void mul_num<pint_t>(pint_t& a, pint_t& b)
{
    s8 res = (s8)a * (s8)b;

    if ((s8)(pint_t)res != res)
    {
        RaiseException(Exception::ERROR_arithmetic, "Operator *: integer overflow.");
    }

    a = (pint_t)res;
}

#endif // PIKA_64BIT_INT

#endif

// divide for integers
template<>
INLINE void div_num<pint_t>(pint_t& a, pint_t& b)
{
#ifndef NO_DIVIDEBYZERO_ERROR
    if (b == 0)
    {
        RaiseException(Exception::ERROR_arithmetic, "OpDiv: division by zero");
    }
#endif
#if defined(USE_C_ARITH)
    a /= b;
#else
	if (((a < 0) != (b < 0)) && (a % b != 0))
    {
		a = a / b - 1;
    }
	else
    {
		a /= b;
    }
#endif
}

// modulo for integers
template<>
INLINE void mod_num<pint_t>(pint_t& a, pint_t& b)
{

#ifndef NO_DIVIDEBYZERO_ERROR
    if (b == 0)
    {
        RaiseException(Exception::ERROR_arithmetic, "OpMod: division by zero");
    }
#endif // NO_DIVIDEBYZERO_ERROR

#if defined(USE_C_ARITH)
    a %= b;
#else
    a = a - (b * Floor((preal_t)a/(preal_t)b));
#endif

}

// modulo for reals
template<> INLINE void mod_num<preal_t>(preal_t& a, preal_t& b) 
{ 
#ifndef NO_DIVIDEBYZERO_ERROR
    if (b == 0)
    {
        RaiseException(Exception::ERROR_arithmetic, "OpMod: division by zero");
    }
#endif // NO_DIVIDEBYZERO_ERROR
#if defined(USE_C_ARITH)
    a = (preal_t)fmod(a, b);
#else
    a = a - (b * (preal_t)Floor((preal_t)a/(preal_t)b));
#endif
}

template<typename T> INLINE bool  eq_num(const T& a, const T& b) { return(a == b); }
template<typename T> INLINE bool neq_num(const T& a, const T& b) { return(a != b); }
template<typename T> INLINE bool  le_num(const T& a, const T& b) { return(a  < b); }
template<typename T> INLINE bool  gr_num(const T& a, const T& b) { return(a  > b); }
template<typename T> INLINE bool lte_num(const T& a, const T& b) { return(a <= b); }
template<typename T> INLINE bool gte_num(const T& a, const T& b) { return(a >= b); }

INLINE void  lsh_num(pint_t& a, pint_t& b) { a <<= b; }
INLINE void  rsh_num(pint_t& a, pint_t& b) { a >>= b; }
INLINE void ursh_num(pint_t& a, pint_t& b)
{
    union {
        pint_t  s;
        puint_t u;
    } s2u;
    s2u.s = a;
    s2u.u >>= b;
    a = s2u.s;
}

INLINE void band_num(pint_t& a, pint_t& b) { a &= b;  }
INLINE void  bor_num(pint_t& a, pint_t& b) { a |= b;  }
INLINE void bxor_num(pint_t& a, pint_t& b) { a ^= b;  }
INLINE void bnot_num(pint_t& a)          { a = !a;  }

///////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef PIKA_STRING_HEADER

template<> INLINE bool  le_num<String>(const String& a, const String& b) { return a.Compare(&b) <  0; }
template<> INLINE bool  gr_num<String>(const String& a, const String& b) { return a.Compare(&b) >  0; }
template<> INLINE bool lte_num<String>(const String& a, const String& b) { return a.Compare(&b) <= 0; }
template<> INLINE bool gte_num<String>(const String& a, const String& b) { return a.Compare(&b) >= 0; }

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
