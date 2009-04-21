/*
 *  PUtil.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_UTIL_HEADER
#define PIKA_UTIL_HEADER

namespace pika {

/** The maximum size a buffer of type T can be. */
template<typename T>
INLINE size_t GetMaxSize() { return size_t(-1) / sizeof(T); }

template<typename T>
struct Nullable
{
    INLINE Nullable(T t_in): t(t_in) {}

    INLINE      operator T()             { return  t;      }
    INLINE      operator const T() const { return  t;      }
    INLINE bool operator!() const        { return !t;      }
    INLINE      operator bool() const    { return  t != 0; }
    INLINE T    operator->()             { return  t;      }
private:
    T t;
};

template<typename T> INLINE T    Max(const T a, const T b) { return (a >= b) ? a : b; }
template<typename T> INLINE T    Min(const T a, const T b) { return (a <= b) ? a : b; }
template<typename T> INLINE T    Clamp(const T t, const T lo, const T hi) { return ((t < lo) ? lo : ((t < hi) ? t : hi)); }
template<typename T> INLINE void Swap(T& a, T& b) { T t = a; a = b; b = t; }

template<typename T>
INLINE T LeastCommonMultiple(T a, T b)
{
    if (b > a)
        Swap(a, b);
    else if (a == b)
        return a;

    T tmax   = a * b;
    T tmult  = a;
    T factor = 2;

    while ((tmult % b) && (tmult <= tmax))
    {
        tmult = a * factor;
        factor++;
    }
    return tmult;
}

#define PIKA_NODESTRUCTOR(T)                    \
    template <> struct TNeedsDestructor< T >    \
    {                                           \
        INLINE bool operator()()                \
        {                                       \
            return false;                       \
        }                                       \
    };

template <typename T>
struct TNeedsDestructor
{
    INLINE bool operator()() { return true; }
};

template <typename T>
struct TNeedsDestructor<T*>
{
    INLINE bool operator()() { return false; }
};

PIKA_NODESTRUCTOR(char)
PIKA_NODESTRUCTOR(u1)
PIKA_NODESTRUCTOR(u2)
PIKA_NODESTRUCTOR(u4)
PIKA_NODESTRUCTOR(s1)
PIKA_NODESTRUCTOR(s2)
PIKA_NODESTRUCTOR(s4)
PIKA_NODESTRUCTOR(float)
PIKA_NODESTRUCTOR(double)
PIKA_NODESTRUCTOR(bool)

template <typename T>
struct TAlignIt
{
    T    t;
    char c;
};

#define alignof(T) (( sizeof( TAlignIt<T> ) > sizeof( T )) ? sizeof( TAlignIt<T> ) - sizeof( T ) : sizeof( T ))

template <typename T, size_t N>
INLINE size_t countof(const T (&arr) [N]) { return N; }

INLINE size_t MemAlign(size_t sz, size_t a)
{
    return ((sz + a - 1) / (a)) * (a);
}

/** Finds a common alignment and size for two objects. */

INLINE void CommonAlign(size_t  aSize,   size_t  aAlign,
                        size_t  bSize,   size_t  bAlign,
                        size_t& cmnSize, size_t& cmnAlign)
{
    size_t maxSize = (bSize > aSize) ? bSize : aSize;
    
    cmnAlign = LeastCommonMultiple(aAlign, bAlign);
    cmnSize  = MemAlign(maxSize, cmnAlign);
}

template<typename T>
struct TLinked
{
    INLINE TLinked() : next(0) { }

    void Attach(T* nxt)
    {
        if (!next)
        {
            next = nxt;
        }
        else
        {
            T* curr = next;
            while (curr->next)
                curr = curr->next;
            curr->next = nxt;
        }
    }

    T* next;
};// struct TLinked

}// namespace pika

#endif
