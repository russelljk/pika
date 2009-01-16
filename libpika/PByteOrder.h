/*
 *  PByteorder.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_BYTEORDER_HEADER
#define PIKA_BYTEORDER_HEADER

INLINE u2 Pika_SwapU2(u2 x)
{
    u2 result;
    result = ((x << 8) & 0xFF00) | ((x >> 8) & 0xFF);
    return result;
}

INLINE u4 Pika_SwapU4(u4 x)
{
    u4 result;
    result = ((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x >> 8) & 0xFF00) | ((x >> 24) & 0xFF);
    return result;
}

INLINE u8 Pika_SwapU8(u8 x)
{
    union U8Swap 
    {
        u8 u;
        u4 v[2];
    } tmp, result;

    tmp.u = x;

    result.v[0] = Pika_SwapU4(tmp.v[1]);
    result.v[1] = Pika_SwapU4(tmp.v[0]);
    return result.u;
}

#if defined(PIKA_BIG_ENDIAN)

// Big endian /////////////////////////////////////////////////////////////////////////////////////

INLINE u2 Pika_HostToBigU2(u2 x)       { return x; }
INLINE u4 Pika_HostToBigEndianU4(u4 x) { return x; }
INLINE u8 Pika_HostToBigEndianU8(u8 x) { return x; }

INLINE u2 Pika_HostToLittleU2(u2 x) { return Pika_SwapU2(x); }
INLINE u4 Pika_HostToLittleU4(u4 x) { return Pika_SwapU4(x); }
INLINE u8 Pika_HostToLittleU8(u8 x) { return Pika_SwapU8(x); }

INLINE u2 Pika_BigToHostU2(u2 x) { return x; }
INLINE u4 Pika_BigToHostU4(u4 x) { return x; }
INLINE u8 Pika_BigToHostU8(u8 x) { return x; }

INLINE u2 Pika_LittleToHostU2(u2 x) { return Pika_SwapU2(x); }
INLINE u4 Pika_LittleToHostU4(u4 x) { return Pika_SwapU4(x); }
INLINE u8 Pika_LittleToHostU8(u8 x) { return Pika_SwapU8(x); }

#if defined(PIKA_64)

// 64bit pointer //////////////////////////////////////////////////////////////////////////////////

INLINE size_t Pika_HostToBigSizeT(size_t x)    { return x; }
INLINE size_t Pika_HostToLittleSizeT(size_t x) { return Pika_SwapU8(x); }

INLINE size_t Pika_BigToHostSizeT(size_t x)    { return x; }
INLINE size_t Pika_LittleToHostSizeT(size_t x) { return Pika_SwapU8(x); }

#else

// 32bit pointer //////////////////////////////////////////////////////////////////////////////////

INLINE size_t Pika_HostToBigSizeT(size_t x)    { return x; }
INLINE size_t Pika_HostToLittleSizeT(size_t x) { return Pika_SwapU4(x); }

INLINE size_t Pika_BigToHostSizeT(size_t x)    { return x; }
INLINE size_t Pika_LittleToHostSizeT(size_t x) { return Pika_SwapU4(x); }

#endif

#else

// Little endian //////////////////////////////////////////////////////////////////////////////////

INLINE u2 Pika_HostToBigU2(u2 x) { return Pika_SwapU2(x); }
INLINE u4 Pika_HostToBigU4(u4 x) { return Pika_SwapU4(x); }
INLINE u8 Pika_HostToBigU8(u8 x) { return Pika_SwapU8(x); }

INLINE u2 Pika_HostToLittleU2(u2 x) { return x; }
INLINE u4 Pika_HostToLittleU4(u4 x) { return x; }
INLINE u8 Pika_HostToLittleU8(u8 x) { return x; }

INLINE u2 Pika_BigToHostU2(u2 x) { return Pika_SwapU2(x); }
INLINE u4 Pika_BigToHostU4(u4 x) { return Pika_SwapU4(x); }
INLINE u8 Pika_BigToHostU8(u8 x) { return Pika_SwapU8(x); }

INLINE u2 Pika_LittleToHostU2(u2 x) { return x; }
INLINE u4 Pika_LittleToHostU4(u4 x) { return x; }
INLINE u8 Pika_LittleToHostU8(u8 x) { return x; }

#if defined(PIKA_64)

// 64bit pointer //////////////////////////////////////////////////////////////////////////////////

INLINE size_t Pika_HostToBigSizeT(size_t x)    { return Pika_SwapU8(x); }
INLINE size_t Pika_HostToLittleSizeT(size_t x) { return x; }

INLINE size_t Pika_BigToHostSizeT(size_t x)    { return Pika_SwapU8(x); }
INLINE size_t Pika_LittleToHostSizeT(size_t x) { return x; }

#else

// 32bit pointer //////////////////////////////////////////////////////////////////////////////////

INLINE size_t Pika_HostToBigSizeT(size_t x)    { return Pika_SwapU4(x); }
INLINE size_t Pika_HostToLittleSizeT(size_t x) { return x; }

INLINE size_t Pika_BigToHostSizeT(size_t x)    { return Pika_SwapU4(x); }
INLINE size_t Pika_LittleToHostSizeT(size_t x) { return x; }

#endif

#endif

#define Pika_ToHostU2(x) Pika_BigToHostU2(x)
#define HostToKolaU2(x) Pika_HostToBigU2(x)

#define Pika_ToHostU4(x) Pika_BigToHostU4(x)
#define HostToKolaU4(x) Pika_HostToBigU4(x)

#define Pika_ToHostU8(x) Pika_BigToHostU8(x)
#define HostToKolaU8(x) Pika_HostToBigU8(x)

#endif
