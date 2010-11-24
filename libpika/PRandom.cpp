/*
    gRandom.cpp
    Copyright (c) 2007 Russell Kyle. All rights reserved.

    Random number generator based on Mersenne Twister (see license below.)
 ---------------------------------------------------------------------------------------------------
    A C-program for MT19937, with initialization improved 2002/1/26.
    Coded by Takuji Nishimura and Makoto Matsumoto.

    Before using, initialize the state by using init_genrand(seed)
    or init_by_array(init_key, key_length).

    Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote
        products derived from this software without specific prior written
        permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
    LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
    NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
*/

#include "Pika.h"
#include "PRandom.h"

namespace pika {

PIKA_IMPL(Random)

Random::Random(Engine* eng, Type* randType)
        : ThisSuper(eng, randType),
        mti(N + 1),
        seed(5489)        
{
    SeedRandom();
}

Random::Random(const Random* rhs) :
    ThisSuper(rhs),
    mti(rhs->mti),
    seed(rhs->seed)
{
    Pika_memcpy(mt, rhs->mt, sizeof(randint_t) * N);
}

Random::~Random() {}

Object* Random::Clone()
{
    Random* r = 0;
    GCNEW(engine, Random, r, (this));
    return r;
}

void Random::SeedRandom()
{
#ifndef RANDOM_64_BIT
    mt[0] = seed & 0xffffffff;
    for (mti = 1; mti < N; mti++)
    {
        mt[mti]  = (1812433253 * (mt[ mti - 1 ] ^ (mt[ mti - 1 ] >> 30)) + mti);
        mt[mti] &=  0xffffffff;
    }
#else
    mt[0] = seed;
    for (mti = 1; mti < N; mti++)
    {
        mt[mti] = (6364136223846793005ULL * (mt[mti-1] ^ (mt[mti-1] >> 62)) + mti);
    }
#endif
}

Random* Random::Create(Engine* eng, Type* type)
{

    Random* random = 0;
    GCNEW(eng, Random, random, (eng, type));
    return random;
}

randuint_t Random::NextRandom()
{
#ifndef RANDOM_64_BIT
    randuint_t y;
    static randuint_t mag01[2] = { 0x0, MATRIX_A };
    
    // mag01[x] = x * MATRIX_A  for x = 0, 1
    
    if (mti >= N)
    {
        // Generate N words at one time
        randint_t kk;
        
        if (mti == N + 1)   // If SeedRandom() has not been called,
            SetSeed(seed);  // a default initial seed is used.
            
        for (kk = 0; kk < N - M; kk++)
        {
            y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
            mt[kk] = mt[kk + M] ^ (y >> 1) ^ mag01[y & 0x1];
        }
        
        for (; kk < N - 1; kk++)
        {
            y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
            mt[kk] = mt[kk + (M - N)] ^ (y >> 1) ^ mag01[y & 0x1];
        }
        y = (mt[N - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
        mt[N - 1] = mt[M - 1] ^ (y >> 1) ^ mag01[y & 0x1];
        
        mti = 0;
    }
    
    y = mt[ mti++ ];
    
    // Tempering
    y ^= (y >> 11);
    y ^= (y <<  7) & 0x9d2c5680;
    y ^= (y << 15) & 0xefc60000;
    y ^= (y >> 18);
    
    return y;
#else
    int i;
    randuint_t x;
    static randuint_t mag01[2] = { 0ULL, MATRIX_A };
    
    if (mti >= N) // generate N words at one time
    {
        // ifSetSeed has not been called,
        // a default initial seed is used
        if (mti == N + 1)
            SetSeed(seed);
    
        for (i = 0; i < N - M; i++)
        {
            x = (mt[i] & UPPER_MASK) | (mt[i + 1] & LOWER_MASK);
            mt[i] = mt[i+M] ^ (x >> 1) ^ mag01[(int)(x & 1ULL)];
        }
        for (; i < N - 1; i++)
        {
            x = (mt[i] & UPPER_MASK) | (mt[i + 1] & LOWER_MASK);
            mt[i] = mt[i + (M - N)] ^ (x >> 1) ^ mag01[(int)(x & 1ULL)];
        }
        x = (mt[N - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
        mt[N - 1] = mt[M - 1] ^ (x >> 1) ^ mag01[(int)(x & 1ULL)];
    
        mti = 0;
    }
    
    x = mt[ mti++ ];
    
    x ^= (x >> 29) & 0x5555555555555555ULL;
    x ^= (x << 17) & 0x71D67FFFEDA60000ULL;
    x ^= (x << 37) & 0xFFF7EEE000000000ULL;
    x ^= (x >> 43);
    
    return x;
#endif
}

preal_t Random::NextReal()
{
    randuint_t r = NextRandom();
#ifndef RANDOM_64_BIT
    return (preal_t)(r * (1.0 / 4294967295.0));    // 24 bits of precision.
#else
    return (preal_t)((r >> 11) * (1.0/9007199254740992.0));
#endif
}

randint_t Random::NextUInt()
{
    return (NextRandom() >> 1) & MAX_RANDOM;
}

void Random::Init(Context* ctx)
{
    u2 argc = ctx->GetArgCount();
    
    if (argc == 1)
    {
        // Initialize with a seed
        seed = ctx->GetIntArg(0);
        SeedRandom();
    }
    else if (argc != 0)
    {
        // more than 1 argument
        ctx->WrongArgCount();
    }
}

pint_t Random::Next(Context* ctx)
{
    u2 argc = ctx->GetArgCount();
    
    if (argc == 0)
    {
        return NextUInt();
    }
    else if (argc == 1)
    {
        pint_t arg0 = ctx->GetIntArg(0);
        return NextUInt() % arg0;
    }
    else if (argc == 2)
    {
        pint_t minArg = ctx->GetIntArg(0);
        pint_t maxArg = ctx->GetIntArg(1);
        
        // Make sure minArg is actually the smallest.
        if (minArg >= maxArg)
        {
            Swap(minArg, maxArg);
        }
        
        // Find the range
        pint_t range = maxArg - minArg;
        
        // Convert the number to (0-range] then add minArg to
        // return the range (minArg-maxArg]
        return (NextUInt() % range) + minArg;
    }
    else
    {
        ctx->WrongArgCount();
    }
    return 0;
}

pint_t Random::GetSeed() { return seed; }

void Random::SetSeed(pint_t s)
{
    seed = s;
    mti = N + 1;
    SeedRandom();
}

Array* Random::Generate(pint_t amt)
{
    if (amt < 0)
        RaiseException("Attempt to generate "PINT_FMT" random numbers. The amount should be > 0.", amt);
        
    GCPAUSE_NORUN(engine);
    Array* v = Array::Create(engine, 0, amt, 0);
    Value* elems = v->GetAt(0);
    
    for (pint_t i = 0; i < amt; ++i)
    {
        elems[i].Set((pint_t)NextUInt());
    }
    return v;
}

String* Random::ToString()
{
    return ThisSuper::ToString();
}

void Random::Constructor(Engine* eng, Type* obj_type, Value& res)
{
    Random* ra = Random::Create(eng, obj_type);
    res.Set(ra);
}

PIKA_DOC(Random, "\
A psuedo-random number generator (PRNG) based on '''Mersenne Twister'''. \n\
[[[Random.new([seed])]]]\n\
You can create multiple instances to have different PRNGs \
with distinct states.");

PIKA_DOC(random_obj, "/()\
\n\
Returns the next psuedo-random number. \
This is an instance of the type [imports.math.Random Random] which implements opCall.\n\
[[[math = import 'math'\n\
print(math.random())]]]\n\
You can easily change the random numbers seed as well.\n\
[[[math.random.seed = 1024]]]\n\
Please refer to [imports.math.Random Random's] documentation for more information \
on other methods and properties."
);

PIKA_DOC(Random_next, "/()\
\n\
Returns the next psuedo-randomly generated [Integer integer]. This method is also used to override the call operator.");

PIKA_DOC(Random_nextReal, "/()\
\n\
Returns the next psuedo-randomly generated [Real real number].");

PIKA_DOC(Random_generate, "/(amt)\
\n\
Returns a [Array] of size |amt|, filled with psuedo-randomly generated numbers. If |amt| is negative or not an [Integer integer] an exception will be raised.");

PIKA_DOC(Random_setSeed, "/(s)\
\n\
Re-seed the PRNG (Pseudo-Random Number Generator) with the seed |s|. You can reset the state of the PRNG by setting |s| to the current value of [seed].");

PIKA_DOC(Random_getSeed, "/()\
\n\
Return the current seed.");

void Random::StaticInitType(Package* module, Engine* eng)
{
    GCPAUSE_NORUN(eng);
    String* Random_String = eng->AllocString("Random");
    Type*   Random_Type  = Type::Create(eng, Random_String, eng->Object_Type, Random::Constructor, module);
    
    module->SetSlot(Random_String, Random_Type, Slot::ATTR_protected);
    
    SlotBinder<Random>(eng, Random_Type)
    .MethodVA(&Random::Next, "opCall", PIKA_GET_DOC(Random_next))
    .Alias("next", "opCall")
    .Method(&Random::NextReal,      "nextReal", PIKA_GET_DOC(Random_nextReal))
    .Method(&Random::Generate,      "generate", PIKA_GET_DOC(Random_generate))
    .Constant((pint_t)MAX_RANDOM,   "MAX")
    .PropertyRW("seed",     
            &Random::GetSeed,   "getSeed", 
            &Random::SetSeed,   "setSeed", PIKA_GET_DOC(Random_getSeed), PIKA_GET_DOC(Random_setSeed))
    ;
    Random_Type->SetDoc(eng->AllocStringNC(PIKA_GET_DOC(Random)));
    Object* random_obj = Random::Create(eng, Random_Type);
    module->SetSlot(eng->AllocString("random"), random_obj, Slot::ATTR_protected);
    random_obj->SetSlot(eng->AllocString("__doc"), eng->AllocStringNC(PIKA_GET_DOC(random_obj)), Slot::ATTR_forcewrite); 
}

}// pika
