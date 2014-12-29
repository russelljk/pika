/*
 *  PNativeStaticMethodDecls.h
 *  See Copyright Notice in Pika.h
 *
 *  DO NOT INCLUDE DIRECTLY!
 */
// StaticMethod0 ////////////////////////////////////////////////////////////////////////////////

template<typename TRet>
struct StaticMethod0 : NativeMethodBase
{
    typedef TRet(*TMETHOD)();

    TMETHOD function;

    StaticMethod0(TMETHOD m) : function(m) { }
    
    virtual void Invoke(void* obj, Context* ctx)
    {
        RetType<TRet>(ctx, (*function)());
    }
    
    virtual int GetRetCount() const { return RetType<TRet>::ReturnCount; }
    virtual int GetArgCount() const { return 0; }
};

template<>
struct StaticMethod0<void> : NativeMethodBase
{
    typedef void(*TMETHOD)();
    TMETHOD function;

    StaticMethod0(TMETHOD m) : function(m) { }
    
    virtual void Invoke(void* obj, Context* ctx)
    {
        (*function)();
    }
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 0; }
};

// StaticMethod1 ////////////////////////////////////////////////////////////////////////////////

template<typename TRet, typename TParam0>
struct StaticMethod1 : NativeMethodBase
{
    typedef TRet(*TMETHOD)(TParam0);

    TMETHOD function;
    char signature[1];

    StaticMethod1(TMETHOD m) : function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
    }
    
    virtual void Invoke(void* obj, Context* ctx)
    {
        VarType<TParam0> param0(ctx, (u2)0);
        TRet ret = (*function)(param0);
        RetType<TRet>(ctx, ret);
    }
    
    virtual int GetRetCount() const { return RetType<TRet>::ReturnCount; }
    virtual int GetArgCount() const { return 1; }
};

template<typename TParam0>
struct StaticMethod1<void, TParam0> : NativeMethodBase
{
    typedef void(*TMETHOD)(TParam0);

    TMETHOD function;
    char signature[1];

    StaticMethod1(TMETHOD m) : function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
    }
    
    virtual void Invoke(void* obj, Context* ctx)
    {
        VarType<TParam0> param0(ctx, (u2)0);
        (*function)(param0);
    }
    
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 1; }
};

template<typename TRet>
struct StaticMethodVA : NativeMethodBase
{
    typedef TRet(*TMETHOD)(Context*);

    TMETHOD function;

    StaticMethodVA(TMETHOD m) : function(m)
    {
    }
    


    virtual void Invoke(void* obj, Context* ctx)
    {
        TRet ret = (*function)(ctx);
        RetType<TRet>(ctx, ret);
    }
    
    virtual int GetRetCount() const { return RetType<TRet>::ReturnCount; }
    virtual int GetArgCount() const { return 0; }
};

template<>
struct StaticMethodVA<void> : NativeMethodBase
{
    typedef void(*TMETHOD)(Context*);

    TMETHOD function;

    StaticMethodVA(TMETHOD m) : function(m)
    {
    }
    
    virtual void Invoke(void* obj, Context* ctx)
    {
        (*function)(ctx);
    }
    
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 0; }
};

// StaticMethod2 ////////////////////////////////////////////////////////////////////////////////

template<typename TRet, typename TParam0, typename TParam1>
struct StaticMethod2 : NativeMethodBase
{
    typedef TRet(*TMETHOD)(TParam0, TParam1);

    TMETHOD function;
    char signature[2];

    StaticMethod2(TMETHOD m) : function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
    }
    
    virtual void Invoke(void* obj, Context* ctx)
    {

        VarType<TParam0> param0(ctx, (u2)0);
        VarType<TParam1> param1(ctx, (u2)1);
        TRet ret = (*function)(param0, param1);
        RetType<TRet>(ctx, ret);
    }
    
    virtual int GetRetCount() const { return RetType<TRet>::ReturnCount; }
    virtual int GetArgCount() const { return 2; }
};

template<typename TParam0, typename TParam1>
struct StaticMethod2<void, TParam0, TParam1> : NativeMethodBase
{
    typedef void(*TMETHOD)(TParam0, TParam1);

    TMETHOD function;
    char signature[2];

    StaticMethod2(TMETHOD m): function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
    }
    
    virtual void Invoke(void* obj, Context* ctx)
    {
        VarType<TParam0> param0(ctx, (u2)0);
        VarType<TParam1> param1(ctx, (u2)1);
        (*function)(param0, param1);
    }
    
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 2; }
};

// StaticMethod3 ////////////////////////////////////////////////////////////////////////////////

template<typename TRet, typename TParam0, typename TParam1, typename TParam2>
struct StaticMethod3 : NativeMethodBase
{
    typedef TRet(*TMETHOD)(TParam0, TParam1, TParam2);

    TMETHOD function;
    char signature[3];

    StaticMethod3(TMETHOD m): function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
        signature[2] = VarType<TParam2>::eSig;
    }
    
    virtual void Invoke(void* obj, Context* ctx)
    {
        VarType<TParam0> param0(ctx, (u2)0);
        VarType<TParam1> param1(ctx, (u2)1);
        VarType<TParam2> param2(ctx, (u2)2);
        TRet ret = (*function)(param0, param1, param2);
        RetType<TRet>(ctx, ret);
    }
    
    virtual int GetRetCount() const { return RetType<TRet>::ReturnCount; }
    virtual int GetArgCount() const { return 3; }
};

template<typename TParam0, typename TParam1, typename TParam2>
struct StaticMethod3<void, TParam0, TParam1, TParam2> : NativeMethodBase
{
    typedef void(*TMETHOD)(TParam0, TParam1, TParam2);

    TMETHOD function;
    char signature[3];

    StaticMethod3(TMETHOD m) : function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
        signature[2] = VarType<TParam2>::eSig;
    }
    
    virtual void Invoke(void* obj, Context* ctx)
    {
        VarType<TParam0> param0(ctx, (u2)0);
        VarType<TParam1> param1(ctx, (u2)1);
        VarType<TParam2> param2(ctx, (u2)2);
        (*function)(param0, param1, param2);
    }
    
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 3; }
};

// StaticMethod4 ////////////////////////////////////////////////////////////////////////////////

template<typename TRet, typename TParam0, typename TParam1, typename TParam2, typename TParam3>
struct StaticMethod4 : NativeMethodBase
{
    typedef TRet(*TMETHOD)(TParam0, TParam1, TParam2, TParam3);

    TMETHOD function;
    char signature[4];

    StaticMethod4(TMETHOD m) : function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
        signature[2] = VarType<TParam2>::eSig;
        signature[3] = VarType<TParam3>::eSig;
    }
    
    virtual void Invoke(void* obj, Context* ctx)
    {
        ctx->ParseArgsInPlace(&signature[0], GetArgCount());
        Value* args = ctx->GetArgs();

        VarType<TParam0> param0(args, (u2)0);
        VarType<TParam1> param1(args, (u2)1);
        VarType<TParam2> param2(args, (u2)2);
        VarType<TParam3> param3(args, (u2)3);

        TRet ret = (*function)(param0, param1, param2, param3);
        RetType<TRet>(ctx, ret);
    }

    virtual int GetRetCount() const { return RetType<TRet>::ReturnCount; }
    virtual int GetArgCount() const { return 4; }
};

template<typename TParam0, typename TParam1, typename TParam2, typename TParam3>
struct StaticMethod4<void, TParam0, TParam1, TParam2, TParam3> : NativeMethodBase
{
    typedef void(*TMETHOD)(TParam0, TParam1, TParam2, TParam3);

    TMETHOD function;
    char signature[4];

    StaticMethod4(TMETHOD m) : function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
        signature[2] = VarType<TParam2>::eSig;
        signature[3] = VarType<TParam3>::eSig;
    }
    
    virtual void Invoke(void* obj, Context* ctx)
    {
        ctx->ParseArgsInPlace(&signature[0], GetArgCount());
        Value* args = ctx->GetArgs();

        VarType<TParam0> param0(args, (u2)0);
        VarType<TParam1> param1(args, (u2)1);
        VarType<TParam2> param2(args, (u2)2);
        VarType<TParam3> param3(args, (u2)3);

        (*function)(param0, param1, param2, param3);
    }
    
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 4; }
};

// StaticMethod5 ////////////////////////////////////////////////////////////////////////////////

template<typename TRet, typename TParam0, typename TParam1, typename TParam2, typename TParam3, typename TParam4>
struct StaticMethod5 : NativeMethodBase
{
    typedef TRet(*TMETHOD)(TParam0, TParam1, TParam2, TParam3, TParam4);

    TMETHOD function;
    char signature[5];

    StaticMethod5(TMETHOD m) : function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
        signature[2] = VarType<TParam2>::eSig;
        signature[3] = VarType<TParam3>::eSig;
        signature[4] = VarType<TParam4>::eSig;
    }
    
    virtual void Invoke(void* obj, Context* ctx)
    {
        ctx->ParseArgsInPlace(&signature[0], GetArgCount());
        Value* args = ctx->GetArgs();

        VarType<TParam0> param0(args, (u2)0);
        VarType<TParam1> param1(args, (u2)1);
        VarType<TParam2> param2(args, (u2)2);
        VarType<TParam3> param3(args, (u2)3);
        VarType<TParam4> param4(args, (u2)4);

        TRet ret = (*function)(param0, param1, param2, param3, param4);
        RetType<TRet>(ctx, ret);
    }

    virtual int GetRetCount() const { return RetType<TRet>::ReturnCount; }
    virtual int GetArgCount() const { return 5; }
};

template<typename TParam0, typename TParam1, typename TParam2, typename TParam3, typename TParam4>
struct StaticMethod5<void, TParam0, TParam1, TParam2, TParam3, TParam4> : NativeMethodBase
{
    typedef void(*TMETHOD)(TParam0, TParam1, TParam2, TParam3, TParam4);

    TMETHOD function;
    char signature[5];

    StaticMethod5(TMETHOD m): function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
        signature[2] = VarType<TParam2>::eSig;
        signature[3] = VarType<TParam3>::eSig;
        signature[4] = VarType<TParam4>::eSig;
    }
    
    virtual void Invoke(void* obj, Context* ctx)
    {
        ctx->ParseArgsInPlace(&signature[0], GetArgCount());
        Value* args = ctx->GetArgs();

        VarType<TParam0> param0(args, (u2)0);
        VarType<TParam1> param1(args, (u2)1);
        VarType<TParam2> param2(args, (u2)2);
        VarType<TParam3> param3(args, (u2)3);
        VarType<TParam4> param4(args, (u2)4);

        (*function)(param0, param1, param2, param3, param4);
    }
    
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 5; }
};

// StaticMethod6 ////////////////////////////////////////////////////////////////////////////////

template<typename TRet, typename TParam0, typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5>
struct StaticMethod6 : NativeMethodBase
{
    typedef TRet(*TMETHOD)(TParam0, TParam1, TParam2, TParam3, TParam4, TParam5);

    TMETHOD function;
    char signature[6];

    StaticMethod6(TMETHOD m) : function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
        signature[2] = VarType<TParam2>::eSig;
        signature[3] = VarType<TParam3>::eSig;
        signature[4] = VarType<TParam4>::eSig;
        signature[5] = VarType<TParam5>::eSig;
    }
    
    virtual void Invoke(void* obj, Context* ctx)
    {
        ctx->ParseArgsInPlace(&signature[0], GetArgCount());
        Value* args = ctx->GetArgs();

        VarType<TParam0> param0(args, (u2)0);
        VarType<TParam1> param1(args, (u2)1);
        VarType<TParam2> param2(args, (u2)2);
        VarType<TParam3> param3(args, (u2)3);
        VarType<TParam4> param4(args, (u2)4);
        VarType<TParam5> param5(args, (u2)5);

        TRet ret = (*function)(param0, param1, param2, param3,
                                param4, param5);
        RetType<TRet>(ctx, ret);
    }

    virtual int GetRetCount() const { return RetType<TRet>::ReturnCount; }
    virtual int GetArgCount() const { return 6; }
};

template<typename TParam0, typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5>
struct StaticMethod6<void, TParam0, TParam1, TParam2, TParam3, TParam4, TParam5> : NativeMethodBase
{
    typedef void(*TMETHOD)(TParam0, TParam1, TParam2, TParam3, TParam4, TParam5);

    TMETHOD function;
    char signature[6];

    StaticMethod6(TMETHOD m): function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
        signature[2] = VarType<TParam2>::eSig;
        signature[3] = VarType<TParam3>::eSig;
        signature[4] = VarType<TParam4>::eSig;
        signature[5] = VarType<TParam5>::eSig;
    }
    
    virtual void Invoke(void* obj, Context* ctx)
    {
        ctx->ParseArgsInPlace(&signature[0], GetArgCount());
        Value* args = ctx->GetArgs();

        VarType<TParam0> param0(args, (u2)0);
        VarType<TParam1> param1(args, (u2)1);
        VarType<TParam2> param2(args, (u2)2);
        VarType<TParam3> param3(args, (u2)3);
        VarType<TParam4> param4(args, (u2)4);
        VarType<TParam5> param5(args, (u2)5);

        (*function)(param0, param1, param2, param3,
                     param4, param5);
    }
    
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 6; }
};

// StaticMethod7 ////////////////////////////////////////////////////////////////////////////////

template<typename TRet, typename TParam0, typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5, typename TParam6>
struct StaticMethod7 : NativeMethodBase
{
    typedef TRet(*TMETHOD)(TParam0, TParam1, TParam2, TParam3, TParam4, TParam5, TParam6);

    TMETHOD function;
    char signature[7];

    StaticMethod7(TMETHOD m): function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
        signature[2] = VarType<TParam2>::eSig;
        signature[3] = VarType<TParam3>::eSig;
        signature[4] = VarType<TParam4>::eSig;
        signature[5] = VarType<TParam5>::eSig;
        signature[6] = VarType<TParam6>::eSig;
    }
    
    virtual void Invoke(void* obj, Context* ctx)
    {
        ctx->ParseArgsInPlace(&signature[0], GetArgCount());
        Value* args = ctx->GetArgs();

        VarType<TParam0> param0(args, (u2)0);
        VarType<TParam1> param1(args, (u2)1);
        VarType<TParam2> param2(args, (u2)2);
        VarType<TParam3> param3(args, (u2)3);
        VarType<TParam4> param4(args, (u2)4);
        VarType<TParam5> param5(args, (u2)5);
        VarType<TParam6> param6(args, (u2)6);

        TRet ret = (*function)(param0, param1, param2, param3,
                                param4, param5, param6);
        RetType<TRet>(ctx, ret);
    }

    virtual int GetRetCount() const { return RetType<TRet>::ReturnCount; }
    virtual int GetArgCount() const { return 7; }
};

template<typename TParam0, typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5, typename TParam6>
struct StaticMethod7<void, TParam0, TParam1, TParam2, TParam3, TParam4, TParam5, TParam6> : NativeMethodBase
{
    typedef void(*TMETHOD)(TParam0, TParam1, TParam2, TParam3, TParam4, TParam5, TParam6);

    TMETHOD function;
    char signature[7];

    StaticMethod7(TMETHOD m): function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
        signature[2] = VarType<TParam2>::eSig;
        signature[3] = VarType<TParam3>::eSig;
        signature[4] = VarType<TParam4>::eSig;
        signature[5] = VarType<TParam5>::eSig;
        signature[6] = VarType<TParam6>::eSig;
    }
    
    virtual void Invoke(void* obj, Context* ctx)
    {
        ctx->ParseArgsInPlace(&signature[0], GetArgCount());
        Value* args = ctx->GetArgs();

        VarType<TParam0> param0(args, (u2)0);
        VarType<TParam1> param1(args, (u2)1);
        VarType<TParam2> param2(args, (u2)2);
        VarType<TParam3> param3(args, (u2)3);
        VarType<TParam4> param4(args, (u2)4);
        VarType<TParam5> param5(args, (u2)5);
        VarType<TParam6> param6(args, (u2)6);

        (*function)(param0, param1, param2, param3,
                     param4, param5, param6);
    }
    
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 7; }
};

// StaticMethod8 ////////////////////////////////////////////////////////////////////////////////

template<typename TRet, typename TParam0, typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5, typename TParam6, typename TParam7>
struct StaticMethod8 : NativeMethodBase
{
    typedef TRet(*TMETHOD)(TParam0, TParam1, TParam2, TParam3, TParam4, TParam5, TParam6, TParam7);

    TMETHOD function;
    char signature[8];

    StaticMethod8(TMETHOD m) : function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
        signature[2] = VarType<TParam2>::eSig;
        signature[3] = VarType<TParam3>::eSig;
        signature[4] = VarType<TParam4>::eSig;
        signature[5] = VarType<TParam5>::eSig;
        signature[6] = VarType<TParam6>::eSig;
        signature[7] = VarType<TParam7>::eSig;
    }
    
    virtual void Invoke(void* obj, Context* ctx)
    {
        ctx->ParseArgsInPlace(&signature[0], GetArgCount());
        Value* args = ctx->GetArgs();

        VarType<TParam0> param0(args, (u2)0);
        VarType<TParam1> param1(args, (u2)1);
        VarType<TParam2> param2(args, (u2)2);
        VarType<TParam3> param3(args, (u2)3);
        VarType<TParam4> param4(args, (u2)4);
        VarType<TParam5> param5(args, (u2)5);
        VarType<TParam6> param6(args, (u2)6);
        VarType<TParam7> param7(args, (u2)7);

        TRet ret = (*function)(param0, param1, param2, param3,
                                param4, param5, param6,
                                param7);
        RetType<TRet>(ctx, ret);
    }

    virtual int GetRetCount() const { return RetType<TRet>::ReturnCount; }
    virtual int GetArgCount() const { return 8; }
};

template<typename TParam0, typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5, typename TParam6, typename TParam7>
struct StaticMethod8<void, TParam0, TParam1, TParam2, TParam3, TParam4, TParam5, TParam6, TParam7> : NativeMethodBase
{
    typedef void(*TMETHOD)(TParam0, TParam1, TParam2, TParam3, TParam4, TParam5, TParam6, TParam7);

    TMETHOD function;
    char signature[8];

    StaticMethod8(TMETHOD m) : function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
        signature[2] = VarType<TParam2>::eSig;
        signature[3] = VarType<TParam3>::eSig;
        signature[4] = VarType<TParam4>::eSig;
        signature[5] = VarType<TParam5>::eSig;
        signature[6] = VarType<TParam6>::eSig;
        signature[7] = VarType<TParam7>::eSig;
    }
    
    virtual void Invoke(void* obj, Context* ctx)
    {
        ctx->ParseArgsInPlace(&signature[0], GetArgCount());
        Value* args = ctx->GetArgs();

        VarType<TParam0> param0(args, (u2)0);
        VarType<TParam1> param1(args, (u2)1);
        VarType<TParam2> param2(args, (u2)2);
        VarType<TParam3> param3(args, (u2)3);
        VarType<TParam4> param4(args, (u2)4);
        VarType<TParam5> param5(args, (u2)5);
        VarType<TParam6> param6(args, (u2)6);
        VarType<TParam7> param7(args, (u2)7);

        (*function)(param0, param1, param2, param3,
                     param4, param5, param6,
                     param7);
    }
    
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 8; }
};

// StaticMethod9 ////////////////////////////////////////////////////////////////////////////////

template<typename TRet, typename TParam0, typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5, typename TParam6, typename TParam7, typename TParam8>
struct StaticMethod9 : NativeMethodBase
{
    typedef TRet(*TMETHOD)(TParam0, TParam1, TParam2, TParam3, TParam4, TParam5, TParam6, TParam7, TParam8);

    TMETHOD function;
    char signature[9];

    StaticMethod9(TMETHOD m) : function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
        signature[2] = VarType<TParam2>::eSig;
        signature[3] = VarType<TParam3>::eSig;
        signature[4] = VarType<TParam4>::eSig;
        signature[5] = VarType<TParam5>::eSig;
        signature[6] = VarType<TParam6>::eSig;
        signature[7] = VarType<TParam7>::eSig;
        signature[8] = VarType<TParam8>::eSig;
    }
    
    virtual void Invoke(void* obj, Context* ctx)
    {
        ctx->ParseArgsInPlace(&signature[0], GetArgCount());
        Value* args = ctx->GetArgs();

        VarType<TParam0> param0(args, (u2)0);
        VarType<TParam1> param1(args, (u2)1);
        VarType<TParam2> param2(args, (u2)2);
        VarType<TParam3> param3(args, (u2)3);
        VarType<TParam4> param4(args, (u2)4);
        VarType<TParam5> param5(args, (u2)5);
        VarType<TParam6> param6(args, (u2)6);
        VarType<TParam7> param7(args, (u2)7);
        VarType<TParam8> param8(args, (u2)8);

        TRet ret = (*function)(param0, param1, param2, param3,
                                param4, param5, param6,
                                param7, param8);
        RetType<TRet>(ctx, ret);
    }
    
    virtual int GetRetCount() const { return RetType<TRet>::ReturnCount; }
    virtual int GetArgCount() const { return 9; }
};

template<typename TParam0, typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5, typename TParam6, typename TParam7, typename TParam8>
struct StaticMethod9<void, TParam0, TParam1, TParam2, TParam3, TParam4, TParam5, TParam6, TParam7, TParam8> : NativeMethodBase
{
    typedef void(*TMETHOD)(TParam0, TParam1, TParam2, TParam3, TParam4, TParam5, TParam6, TParam7, TParam8);

    TMETHOD function;
    char signature[9];

    StaticMethod9(TMETHOD m) : function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
        signature[2] = VarType<TParam2>::eSig;
        signature[3] = VarType<TParam3>::eSig;
        signature[4] = VarType<TParam4>::eSig;
        signature[5] = VarType<TParam5>::eSig;
        signature[6] = VarType<TParam6>::eSig;
        signature[7] = VarType<TParam7>::eSig;
        signature[8] = VarType<TParam8>::eSig;
    }
    
    virtual void Invoke(void* obj, Context* ctx)
    {
        ctx->ParseArgsInPlace(&signature[0], GetArgCount());
        Value* args = ctx->GetArgs();

        VarType<TParam0> param0(args, (u2)0);
        VarType<TParam1> param1(args, (u2)1);
        VarType<TParam2> param2(args, (u2)2);
        VarType<TParam3> param3(args, (u2)3);
        VarType<TParam4> param4(args, (u2)4);
        VarType<TParam5> param5(args, (u2)5);
        VarType<TParam6> param6(args, (u2)6);
        VarType<TParam7> param7(args, (u2)7);
        VarType<TParam8> param8(args, (u2)8);

        (*function)(param0, param1, param2, param3,
                     param4, param5, param6,
                     param7, param8);
    }
    
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 9; }
};

// StaticMethod10 ///////////////////////////////////////////////////////////////////////////////

template<typename TRet, typename TParam0, typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5, typename TParam6, typename TParam7, typename TParam8, typename TParam9>
struct StaticMethod10 : NativeMethodBase
{
    typedef TRet(*TMETHOD)(TParam0, TParam1, TParam2, TParam3, TParam4, TParam5, TParam6, TParam7, TParam8, TParam9);

    TMETHOD function;
    char signature[10];

    StaticMethod10(TMETHOD m) : function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
        signature[2] = VarType<TParam2>::eSig;
        signature[3] = VarType<TParam3>::eSig;
        signature[4] = VarType<TParam4>::eSig;
        signature[5] = VarType<TParam5>::eSig;
        signature[6] = VarType<TParam6>::eSig;
        signature[7] = VarType<TParam7>::eSig;
        signature[8] = VarType<TParam8>::eSig;
        signature[9] = VarType<TParam9>::eSig;
    }
    
    virtual void Invoke(void* obj, Context* ctx)
    {
        ctx->ParseArgsInPlace(&signature[0], GetArgCount());
        Value* args = ctx->GetArgs();

        VarType<TParam0> param0(args, (u2)0);
        VarType<TParam1> param1(args, (u2)1);
        VarType<TParam2> param2(args, (u2)2);
        VarType<TParam3> param3(args, (u2)3);
        VarType<TParam4> param4(args, (u2)4);
        VarType<TParam5> param5(args, (u2)5);
        VarType<TParam6> param6(args, (u2)6);
        VarType<TParam7> param7(args, (u2)7);
        VarType<TParam8> param8(args, (u2)8);
        VarType<TParam9> param9(args, (u2)9);
        TRet ret = (*function)(param0, param1, param2, param3,
                                param4, param5, param6,
                                param7, param8, param9);
        RetType<TRet>(ctx, ret);
    }
    
    virtual int GetRetCount() const { return RetType<TRet>::ReturnCount; }
    virtual int GetArgCount() const { return 10; }
};

template<typename TParam0, typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5, typename TParam6, typename TParam7, typename TParam8, typename TParam9>
struct StaticMethod10<void, TParam0, TParam1, TParam2, TParam3, TParam4, TParam5, TParam6, TParam7, TParam8, TParam9> : NativeMethodBase
{
    typedef void(*TMETHOD)(TParam0, TParam1, TParam2, TParam3, TParam4, TParam5, TParam6, TParam7, TParam8, TParam9);

    TMETHOD function;
    char signature[10];

    StaticMethod10(TMETHOD m) : function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
        signature[2] = VarType<TParam2>::eSig;
        signature[3] = VarType<TParam3>::eSig;
        signature[4] = VarType<TParam4>::eSig;
        signature[5] = VarType<TParam5>::eSig;
        signature[6] = VarType<TParam6>::eSig;
        signature[7] = VarType<TParam7>::eSig;
        signature[8] = VarType<TParam8>::eSig;
        signature[9] = VarType<TParam9>::eSig;
    }
    
    virtual void Invoke(void* obj, Context* ctx)
    {
        ctx->ParseArgsInPlace(&signature[0], GetArgCount());
        Value* args = ctx->GetArgs();

        VarType<TParam0> param0(args, (u2)0);
        VarType<TParam1> param1(args, (u2)1);
        VarType<TParam2> param2(args, (u2)2);
        VarType<TParam3> param3(args, (u2)3);
        VarType<TParam4> param4(args, (u2)4);
        VarType<TParam5> param5(args, (u2)5);
        VarType<TParam6> param6(args, (u2)6);
        VarType<TParam7> param7(args, (u2)7);
        VarType<TParam8> param8(args, (u2)8);
        VarType<TParam9> param9(args, (u2)9);

        (*function)(param0, param1, param2, param3,
                     param4, param5, param6,
                     param7, param8, param9);
    }
    
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 10; }
};
