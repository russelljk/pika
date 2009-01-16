/*
 *  PRandom.h
 *  Copyright (c) 2007 Russell Kyle. All rights reserved.
 *  
 *  Random number generator based on Mersenne Twister (see license below.)    
 *  ----------------------------------------------------------------------
 *  A C-program for MT19937, with initialization improved 2002/1/26.
 *  Coded by Takuji Nishimura and Makoto Matsumoto.
 *
 *  Before using, initialize the state by using init_genrand(seed)
 *  or init_by_array(init_key, key_length).
 *
 *  Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   3. The names of its contributors may not be used to endorse or promote
 *      products derived from this software without specific prior written
 *      permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
 */
#ifndef PIKA_RANDOM_HEADER
#define PIKA_RANDOM_HEADER

#include "Pika.h"

#ifdef PIKA_64BIT_INT
#   define RANDOM_64_BIT
#endif

#ifndef RANDOM_64_BIT
#   define N           624
#   define M           397
#   define MATRIX_A    0x9908B0DF
#   define UPPER_MASK  0x80000000
#   define LOWER_MASK  0x7FFFFFFF
#   define MAX_RANDOM  0x7FFFFFFF
#else // 64bit
#   define N           312
#   define M           156
#   define MATRIX_A    0xB5026F5AA96619E9ULL
#   define UPPER_MASK  0xFFFFFFFF80000000ULL
#   define LOWER_MASK  0x7FFFFFFFULL
#   define MAX_RANDOM  0x7FFFFFFFFFFFFFFFULL
#endif

namespace pika
{
#ifndef RANDOM_64_BIT
typedef s4      randint_t;
typedef u4      randuint_t;
typedef float   randf_t;
#else
typedef s8      randint_t;
typedef u8      randuint_t;
typedef double  randf_t;
#endif

class Random : public Object
{
    PIKA_DECL(Random, Object)
protected:
    explicit        Random(Engine* eng, Type* randType);
public:    
    virtual        ~Random();
    
    static Random*  Create(Engine* eng, Type* type);
    
    randuint_t      NextRandom();
    preal_t           NextReal();
    randint_t       NextUInt();
    
    virtual void    Init(Context* ctx);    
    virtual String* ToString();
    pint_t            Next(Context* ctx);
    
    pint_t            GetSeed();
    void            SetSeed(pint_t s);
        
    Array*          Generate(pint_t amt);
protected:
    void            SeedRandom();

    s4              mti;    
    randint_t       mt[N];
    randuint_t      seed;
};

}

#endif
