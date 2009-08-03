/*
 *  PNativeMethodDecls.h
 *  See Copyright Notice in Pika.h
 *
 *  DO NOT INCLUDE DIRECTLY!
 */
// Method0 /////////////////////////////////////////////////////////////////////////////////////////

template<typename TClass, typename TRet>
struct Method0 : NativeMethodBase
{
    typedef TRet(TClass::*TMETHOD)();

    TMETHOD function;

    Method0(TMETHOD m) : function(m) { }

    virtual void Invoke(void* obj, Context* ctx)
    {
        TClass* ptr = (TClass*)obj;
        RetType<TRet>(ctx, (ptr->*function)());
    }

    virtual int GetArgCount() const { return 0; }
};

template<typename TClass>
struct Method0<TClass, void> : NativeMethodBase
{
    typedef void(TClass::*TMETHOD)();
    TMETHOD function;

    Method0(TMETHOD m) : function(m) { }

    virtual void Invoke(void* obj, Context* ctx)
    {
        TClass* ptr = (TClass*)obj;
        (ptr->*function)();
    }
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 0; }
};

// Method1 /////////////////////////////////////////////////////////////////////////////////////////

template<typename TClass, typename TRet, typename TParam0>
struct Method1 : NativeMethodBase
{
    typedef TRet(TClass::*TMETHOD)(TParam0);

    TMETHOD function;
    char signature[1];

    Method1(TMETHOD m) : function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
    }

    virtual void Invoke(void* obj, Context* ctx)
    {
        TClass* ptr = (TClass*)obj;
        VarType<TParam0> param0(ctx, (u2)0);
        TRet ret = (ptr->*function)(param0);
        RetType<TRet>(ctx, ret);
    }

    virtual int GetArgCount() const { return 1; }
};

template<typename TClass, typename TParam0>
struct Method1<TClass, void, TParam0> : NativeMethodBase
{
    typedef void(TClass::*TMETHOD)(TParam0);

    TMETHOD function;
    char signature[1];

    Method1(TMETHOD m) : function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
    }

    virtual void Invoke(void* obj, Context* ctx)
    {
        TClass* ptr = (TClass*)obj;
        VarType<TParam0> param0(ctx, (u2)0);
        (ptr->*function)(param0);
    }
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 1; }
};

// MethodVA ////////////////////////////////////////////////////////////////////////////////////////

template<typename TClass, typename TRet>
struct MethodVA : NativeMethodBase
{
    typedef TRet(TClass::*TMETHOD)(Context*);

    TMETHOD function;

    MethodVA(TMETHOD m) : function(m)
    {
    }

    virtual void Invoke(void* obj, Context* ctx)
    {
        TClass* ptr = (TClass*)obj;
        TRet ret = (ptr->*function)(ctx);
        RetType<TRet>(ctx, ret);
    }

    virtual int GetArgCount() const { return 0; }
};

template<typename TClass>
struct MethodVA<TClass, void> : NativeMethodBase
{
    typedef void(TClass::*TMETHOD)(Context*);

    TMETHOD function;

    MethodVA(TMETHOD m) : function(m)
    {
    }

    virtual void Invoke(void* obj, Context* ctx)
    {
        TClass* ptr = (TClass*)obj;
        (ptr->*function)(ctx);
    }
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 0; }
};

// Method2 /////////////////////////////////////////////////////////////////////////////////////////

template<typename TClass, typename TRet, typename TParam0, typename TParam1>
struct Method2 : NativeMethodBase
{
    typedef TRet(TClass::*TMETHOD)(TParam0, TParam1);

    TMETHOD function;
    char signature[2];

    Method2(TMETHOD m) : function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
    }

    virtual void Invoke(void* obj, Context* ctx)
    {
        TClass* ptr = (TClass*)obj;
        VarType<TParam0> param0(ctx, (u2)0);
        VarType<TParam1> param1(ctx, (u2)1);
        TRet ret = (ptr->*function)(param0, param1);
        RetType<TRet>(ctx, ret);
    }

    virtual int GetArgCount() const { return 2; }
};

template<typename TClass, typename TParam0, typename TParam1>
struct Method2<TClass, void, TParam0, TParam1> : NativeMethodBase
{
    typedef void(TClass::*TMETHOD)(TParam0, TParam1);

    TMETHOD function;
    char signature[2];

    Method2(TMETHOD m): function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
    }

    virtual void Invoke(void* obj, Context* ctx)
    {
        TClass* ptr = (TClass*)obj;
        VarType<TParam0> param0(ctx, (u2)0);
        VarType<TParam1> param1(ctx, (u2)1);
        (ptr->*function)(param0, param1);
    }
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 2; }
};

// Method3 /////////////////////////////////////////////////////////////////////////////////////////

template<typename TClass, typename TRet, typename TParam0, typename TParam1, typename TParam2>
struct Method3 : NativeMethodBase
{
    typedef TRet(TClass::*TMETHOD)(TParam0, TParam1, TParam2);

    TMETHOD function;
    char signature[3];

    Method3(TMETHOD m): function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
        signature[2] = VarType<TParam2>::eSig;
    }

    virtual void Invoke(void* obj, Context* ctx)
    {
        TClass* ptr = (TClass*)obj;
        VarType<TParam0> param0(ctx, (u2)0);
        VarType<TParam1> param1(ctx, (u2)1);
        VarType<TParam2> param2(ctx, (u2)2);
        TRet ret = (ptr->*function)(param0, param1, param2);
        RetType<TRet>(ctx, ret);
    }

    virtual int GetArgCount() const { return 3; }
};

template<typename TClass, typename TParam0, typename TParam1, typename TParam2>
struct Method3<TClass, void, TParam0, TParam1, TParam2>: NativeMethodBase
{
    typedef void(TClass::*TMETHOD)(TParam0, TParam1, TParam2);

    TMETHOD function;
    char signature[3];

    Method3(TMETHOD m): function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
        signature[2] = VarType<TParam2>::eSig;
    }

    virtual void Invoke(void* obj, Context* ctx)
    {
        TClass* ptr = (TClass*)obj;
        VarType<TParam0> param0(ctx, (u2)0);
        VarType<TParam1> param1(ctx, (u2)1);
        VarType<TParam2> param2(ctx, (u2)2);
        (ptr->*function)(param0, param1, param2);
    }
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 3; }
};

// Method4 /////////////////////////////////////////////////////////////////////////////////////////

template<typename TClass, typename TRet, typename TParam0, typename TParam1, typename TParam2, typename TParam3>
struct Method4 : NativeMethodBase
{
    typedef TRet(TClass::*TMETHOD)(TParam0, TParam1, TParam2, TParam3);

    TMETHOD function;
    char signature[4];

    Method4(TMETHOD m) : function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
        signature[2] = VarType<TParam2>::eSig;
        signature[3] = VarType<TParam3>::eSig;
    }

    virtual void Invoke(void* obj, Context* ctx)
    {
        TClass* ptr = (TClass*)obj;

        ctx->ParseArgsInPlace(&signature[0], GetArgCount());
        Value* args = ctx->GetArgs();

        VarType<TParam0> param0(args, (u2)0);
        VarType<TParam1> param1(args, (u2)1);
        VarType<TParam2> param2(args, (u2)2);
        VarType<TParam3> param3(args, (u2)3);

        TRet ret = (ptr->*function)(param0, param1, param2, param3);
        RetType<TRet>(ctx, ret);
    }

    virtual int GetArgCount() const { return 4; }
};

template<typename TClass, typename TParam0, typename TParam1, typename TParam2, typename TParam3>
struct Method4<TClass, void, TParam0, TParam1, TParam2, TParam3> : NativeMethodBase
{
    typedef void(TClass::*TMETHOD)(TParam0, TParam1, TParam2, TParam3);

    TMETHOD function;
    char signature[4];

    Method4(TMETHOD m) : function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
        signature[2] = VarType<TParam2>::eSig;
        signature[3] = VarType<TParam3>::eSig;
    }

    virtual void Invoke(void* obj, Context* ctx)
    {
        TClass* ptr = (TClass*)obj;

        ctx->ParseArgsInPlace(&signature[0], GetArgCount());
        Value* args = ctx->GetArgs();

        VarType<TParam0> param0(args, (u2)0);
        VarType<TParam1> param1(args, (u2)1);
        VarType<TParam2> param2(args, (u2)2);
        VarType<TParam3> param3(args, (u2)3);

        (ptr->*function)(param0, param1, param2, param3);
    }
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 4; }
};

// Method5 /////////////////////////////////////////////////////////////////////////////////////////

template<typename TClass, typename TRet, typename TParam0, typename TParam1, typename TParam2, typename TParam3, typename TParam4>
struct Method5 : NativeMethodBase
{
    typedef TRet(TClass::*TMETHOD)(TParam0, TParam1, TParam2, TParam3, TParam4);

    TMETHOD function;
    char signature[5];

    Method5(TMETHOD m) : function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
        signature[2] = VarType<TParam2>::eSig;
        signature[3] = VarType<TParam3>::eSig;
        signature[4] = VarType<TParam4>::eSig;
    }

    virtual void Invoke(void* obj, Context* ctx)
    {
        TClass* ptr = (TClass*)obj;

        ctx->ParseArgsInPlace(&signature[0], GetArgCount());
        Value* args = ctx->GetArgs();

        VarType<TParam0> param0(args, (u2)0);
        VarType<TParam1> param1(args, (u2)1);
        VarType<TParam2> param2(args, (u2)2);
        VarType<TParam3> param3(args, (u2)3);
        VarType<TParam4> param4(args, (u2)4);

        TRet ret = (ptr->*function)(param0, param1, param2, param3, param4);
        RetType<TRet>(ctx, ret);
    }

    virtual int GetArgCount() const { return 5; }
};

template<typename TClass, typename TParam0, typename TParam1, typename TParam2, typename TParam3, typename TParam4>
struct Method5<TClass, void, TParam0, TParam1, TParam2, TParam3, TParam4> : NativeMethodBase
{
    typedef void(TClass::*TMETHOD)(TParam0, TParam1, TParam2, TParam3, TParam4);

    TMETHOD function;
    char signature[5];

    Method5(TMETHOD m) : function(m)
    {
        signature[0] = VarType<TParam0>::eSig;
        signature[1] = VarType<TParam1>::eSig;
        signature[2] = VarType<TParam2>::eSig;
        signature[3] = VarType<TParam3>::eSig;
        signature[4] = VarType<TParam4>::eSig;
    }

    virtual void Invoke(void* obj, Context* ctx)
    {
        TClass* ptr = (TClass*)obj;

        ctx->ParseArgsInPlace(&signature[0], GetArgCount());
        Value* args = ctx->GetArgs();

        VarType<TParam0> param0(args, (u2)0);
        VarType<TParam1> param1(args, (u2)1);
        VarType<TParam2> param2(args, (u2)2);
        VarType<TParam3> param3(args, (u2)3);
        VarType<TParam4> param4(args, (u2)4);

        (ptr->*function)(param0, param1, param2, param3, param4);
    }
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 5; }
};

// Method6 /////////////////////////////////////////////////////////////////////////////////////////

template<typename TClass, typename TRet, typename TParam0, typename TParam1, typename TParam2,
typename TParam3, typename TParam4, typename TParam5>
struct Method6: NativeMethodBase
{
    typedef TRet(TClass::*TMETHOD)(TParam0, TParam1, TParam2, TParam3,
                                   TParam4, TParam5);

    TMETHOD function;
    char signature[6];

    Method6(TMETHOD m): function(m)
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
        TClass* ptr = (TClass*)obj;

        ctx->ParseArgsInPlace(&signature[0], GetArgCount());
        Value* args = ctx->GetArgs();

        VarType<TParam0> param0(args, (u2)0);
        VarType<TParam1> param1(args, (u2)1);
        VarType<TParam2> param2(args, (u2)2);
        VarType<TParam3> param3(args, (u2)3);
        VarType<TParam4> param4(args, (u2)4);
        VarType<TParam5> param5(args, (u2)5);

        TRet ret = (ptr->*function)(param0, param1, param2, param3,
                                    param4, param5);
        RetType<TRet>(ctx, ret);
    }

    virtual int GetArgCount() const { return 6; }
};

template<typename TClass,  typename TParam0, typename TParam1, typename TParam2,
         typename TParam3, typename TParam4, typename TParam5>
struct Method6 < TClass,  void,    TParam0, TParam1, TParam2,
                 TParam3, TParam4, TParam5 > 
        : NativeMethodBase
{
    typedef void(TClass::*TMETHOD)(TParam0, TParam1, TParam2, TParam3,
                                   TParam4, TParam5);

    TMETHOD function;
    char signature[6];

    Method6(TMETHOD m): function(m)
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
        TClass* ptr = (TClass*)obj;

        ctx->ParseArgsInPlace(&signature[0], GetArgCount());
        Value* args = ctx->GetArgs();

        VarType<TParam0> param0(args, (u2)0);
        VarType<TParam1> param1(args, (u2)1);
        VarType<TParam2> param2(args, (u2)2);
        VarType<TParam3> param3(args, (u2)3);
        VarType<TParam4> param4(args, (u2)4);
        VarType<TParam5> param5(args, (u2)5);

        (ptr->*function)(param0, param1, param2, param3,
                         param4, param5);
    }
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 6; }
};

// Method7 /////////////////////////////////////////////////////////////////////////////////////////

template<typename TClass,  typename TRet,    typename TParam0, typename TParam1, typename TParam2,
         typename TParam3, typename TParam4, typename TParam5, typename TParam6>
struct Method7 : NativeMethodBase
{
    typedef TRet(TClass::*TMETHOD)(TParam0, TParam1, TParam2, TParam3,
                                   TParam4, TParam5, TParam6);

    TMETHOD function;
    char signature[7];

    Method7(TMETHOD m) : function(m)
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
        TClass* ptr = (TClass*)obj;

        ctx->ParseArgsInPlace(&signature[0], GetArgCount());
        Value* args = ctx->GetArgs();

        VarType<TParam0> param0(args, (u2)0);
        VarType<TParam1> param1(args, (u2)1);
        VarType<TParam2> param2(args, (u2)2);
        VarType<TParam3> param3(args, (u2)3);
        VarType<TParam4> param4(args, (u2)4);
        VarType<TParam5> param5(args, (u2)5);
        VarType<TParam6> param6(args, (u2)6);

        TRet ret = (ptr->*function)(param0, param1, param2, param3,
                                    param4, param5, param6);
        RetType<TRet>(ctx, ret);
    }

    virtual int GetArgCount() const { return 7; }
};

template<typename TClass,  typename TParam0, typename TParam1, typename TParam2,
         typename TParam3, typename TParam4, typename TParam5, typename TParam6>
struct Method7 < TClass,  void,    TParam0, TParam1, TParam2,
                 TParam3, TParam4, TParam5, TParam6 > 
        : NativeMethodBase
{
    typedef void(TClass::*TMETHOD)(TParam0, TParam1, TParam2, TParam3,
                                   TParam4, TParam5, TParam6);

    TMETHOD function;
    char signature[7];

    Method7(TMETHOD m) : function(m)
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
        TClass* ptr = (TClass*)obj;

        ctx->ParseArgsInPlace(&signature[0], GetArgCount());
        Value* args = ctx->GetArgs();

        VarType<TParam0> param0(args, (u2)0);
        VarType<TParam1> param1(args, (u2)1);
        VarType<TParam2> param2(args, (u2)2);
        VarType<TParam3> param3(args, (u2)3);
        VarType<TParam4> param4(args, (u2)4);
        VarType<TParam5> param5(args, (u2)5);
        VarType<TParam6> param6(args, (u2)6);

        (ptr->*function)(param0, param1, param2, param3,
                         param4, param5, param6);
    }
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 7; }
};

// Method8 /////////////////////////////////////////////////////////////////////////////////////////

template<typename TClass,  typename TRet,    typename TParam0, typename TParam1, typename TParam2,
         typename TParam3, typename TParam4, typename TParam5, typename TParam6, typename TParam7>
struct Method8 : NativeMethodBase
{
    typedef TRet(TClass::*TMETHOD)(TParam0, TParam1, TParam2, TParam3,
                                   TParam4, TParam5, TParam6, TParam7);

    TMETHOD function;
    char signature[8];

    Method8(TMETHOD m): function(m)
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
        TClass* ptr = (TClass*)obj;

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

        TRet ret = (ptr->*function)(param0, param1, param2, param3,
                                    param4, param5, param6,
                                    param7);
        RetType<TRet>(ctx, ret);
    }

    virtual int GetArgCount() const { return 8; }
};

template<typename TClass,  typename TParam0, typename TParam1, typename TParam2, typename TParam3,
         typename TParam4, typename TParam5, typename TParam6, typename TParam7>
struct Method8 < TClass,  void,    TParam0, TParam1, TParam2, TParam3,
                 TParam4, TParam5, TParam6, TParam7 >
        : NativeMethodBase
{
    typedef void(TClass::*TMETHOD)(TParam0, TParam1, TParam2, TParam3,
                                   TParam4, TParam5, TParam6, TParam7);

    TMETHOD function;
    char signature[8];

    Method8(TMETHOD m): function(m)
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
        TClass* ptr = (TClass*)obj;

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

        (ptr->*function)(param0, param1, param2, param3,
                         param4, param5, param6, param7);
    }
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 8; }
};

// Method9 /////////////////////////////////////////////////////////////////////////////////////////

template<typename TClass,  typename TRet,    typename TParam0, typename TParam1, typename TParam2,
         typename TParam3, typename TParam4, typename TParam5, typename TParam6, typename TParam7, 
         typename TParam8>
struct Method9: NativeMethodBase
{
    typedef TRet(TClass::*TMETHOD)(TParam0, TParam1, TParam2, TParam3,
                                   TParam4, TParam5, TParam6,
                                   TParam7, TParam8);

    TMETHOD function;
    char signature[9];

    Method9(TMETHOD m): function(m)
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
        TClass* ptr = (TClass*)obj;

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

        TRet ret = (ptr->*function)(param0, param1, param2, param3,
                                    param4, param5, param6, param7,
                                    param8);
        RetType<TRet>(ctx, ret);
    }

    virtual int GetArgCount() const { return 9; }
};

template<typename TClass,  typename TParam0, typename TParam1, typename TParam2,
         typename TParam3, typename TParam4, typename TParam5, typename TParam6, 
         typename TParam7, typename TParam8>
struct Method9 < TClass,  void,    TParam0, TParam1, TParam2,
                 TParam3, TParam4, TParam5, TParam6, TParam7, 
                 TParam8 > 
        : NativeMethodBase
{
    typedef void(TClass::*TMETHOD)(TParam0, TParam1, TParam2, TParam3,
                                   TParam4, TParam5, TParam6,
                                   TParam7, TParam8);

    TMETHOD function;
    char signature[9];

    Method9(TMETHOD m): function(m)
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
        TClass* ptr = (TClass*)obj;

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

        (ptr->*function)(param0, param1, param2, param3,
                         param4, param5, param6, param7, 
                         param8);
    }
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 9; }
};

// Method10 ////////////////////////////////////////////////////////////////////////////////////////

template<typename TClass,  typename TRet,    typename TParam0, typename TParam1, typename TParam2,
         typename TParam3, typename TParam4, typename TParam5, typename TParam6, typename TParam7,
         typename TParam8, typename TParam9>
struct Method10: NativeMethodBase
{
    typedef TRet(TClass::*TMETHOD)(TParam0, TParam1, TParam2, TParam3,
                                   TParam4, TParam5, TParam6,
                                   TParam7, TParam8, TParam9);

    TMETHOD function;
    char signature[10];

    Method10(TMETHOD m): function(m)
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
        TClass* ptr = (TClass*)obj;

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
        TRet ret = (ptr->*function)(param0, param1, param2, param3,
                                    param4, param5, param6, param7, 
                                    param8, param9);
        RetType<TRet>(ctx, ret);
    }

    virtual int GetArgCount() const { return 10; }
};

template<typename TClass,  typename TParam0, typename TParam1, typename TParam2,
         typename TParam3, typename TParam4, typename TParam5, typename TParam6,
         typename TParam7, typename TParam8, typename TParam9>
struct Method10 < TClass,  void,    TParam0, TParam1, TParam2,
                  TParam3, TParam4, TParam5, TParam6, TParam7, 
                  TParam8, TParam9 > 
        : NativeMethodBase
{
    typedef void(TClass::*TMETHOD)(TParam0, TParam1, TParam2, TParam3,
                                   TParam4, TParam5, TParam6, TParam7,
                                   TParam8, TParam9);

    TMETHOD function;
    char signature[10];

    Method10(TMETHOD m): function(m)
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
        TClass* ptr = (TClass*)obj;

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

        (ptr->*function)(param0, param1, param2, param3,
                         param4, param5, param6, param7, 
                         param8, param9);
    }
    virtual int GetRetCount() const { return 0; }
    virtual int GetArgCount() const { return 10; }
};

