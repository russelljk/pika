/*
 *  PWorld.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PPlatform.h"
#include "PLocalsObject.h"
#include "PByteArray.h"
#include "PFile.h"
#include "PGenerator.h"

namespace pika {
extern void InitSystemLIB(Engine*);
extern void Initialize_ImportAPI(Engine*);
extern void Init_Annotations(Engine*, Package*);
#define NUMBER2STRING_BUFF_SIZE 64

String* Engine::NumberToString(Engine* eng, const Value& v)
{
    char buff[NUMBER2STRING_BUFF_SIZE + 1];
    buff[0] = buff[NUMBER2STRING_BUFF_SIZE] = '\0';
    
    if (v.tag == TAG_integer)
    {
        Pika_snprintf(buff, NUMBER2STRING_BUFF_SIZE, PINT_FMT, v.val.integer);
    }
    else if (v.tag == TAG_real)
    {
        Pika_snprintf(buff, NUMBER2STRING_BUFF_SIZE, PIKA_REAL_FMT, v.val.real);
    }
    else if (v.tag == TAG_index)
    {
        Pika_snprintf(buff, NUMBER2STRING_BUFF_SIZE, SIZE_T_FMT, v.val.index);
    }
    else if (v.tag >= TAG_basic)
    {
        Pika_snprintf(buff, NUMBER2STRING_BUFF_SIZE, POINTER_FMT, v.val.basic);
    }
    return eng->AllocString(&buff[0]);
}

namespace {    

bool IntegerToString(pint_t i, puint_t radix, Buffer<char>& result)
{
    const char* digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    if (radix < 2 || radix > 36)
    {
        return false;
    }
    
    puint_t j = 0;
    size_t const MAX_SIZE_NEEDED = sizeof(pint_t) * CHAR_BIT + 2; // number of bits in a pint_t + sign + 1
    char* buff = (char*)Pika_malloc(MAX_SIZE_NEEDED);
    char* tail = buff + MAX_SIZE_NEEDED;
    char* end  = tail;
    
    bool neg;
    if (i < 0)
    {
        neg = true;
        j = -i;
    }
    else
    {
        neg = false;
        j = i;
    }
    
    do
    {
        *(--tail) = digits[j % radix];
        j = j / radix;
    }
    while (j != 0 && tail > buff);
    
    if (neg)
    {
        *(--tail) = '-';
    }
    
    result.Resize(end - tail);
    Pika_memcpy(result.GetAt(0), tail, end - tail);
    Pika_free(buff);
    return true;
}

/////////////////////////////////////////////// Null ///////////////////////////////////////////////

int Null_toString(Context* ctx, Value& self)
{
    ctx->Push((ctx->GetEngine()->null_String));
    return 1;
}

int Null_toInteger(Context* ctx, Value& self)
{
    ctx->Push((pint_t)0);
    return 1;
}

int Null_toReal(Context* ctx, Value& self)
{
    ctx->Push((preal_t)0.0);
    return 1;
}

int Null_toNumber(Context* ctx, Value& self)
{
    ctx->Push((pint_t)0);
    return 1;
}

int Null_toBoolean(Context* ctx, Value& self)
{
    ctx->PushFalse();
    return 1;
}

int Null_init(Context* ctx, Value& self)
{
    ctx->PushNull();
    return 1;
}

///////////////////////////////////////////// Integer //////////////////////////////////////////////

int Integer_init(Context* ctx, Value& self)
{
    u2 argc = ctx->GetArgCount();
    if (argc == 0)
    {
        ctx->Push((pint_t)0);
    }
    else if (argc == 1)
    {
        pint_t v = ctx->ArgToInt(0);
        ctx->Push(v);
    }
    else
    {
        ctx->WrongArgCount();
    }
    return 1;
}

// TODO: add radix support.
int Integer_toString(Context* ctx, Value& self)
{
    u2 argc = ctx->GetArgCount();
    pint_t radix = 10;
//  static const size_t maxbuffsize = (sizeof(pint_t) * 8 + 2);

    if (argc == 1)
    {
        if (ctx->IsArgNull(0))
            radix = 10;
        else
            radix = ctx->GetIntArg(0);
    }
    else if (argc != 0)
    {
        ctx->WrongArgCount();
    }
    
    Buffer<char> buff;
    IntegerToString(self.val.integer, radix, buff);
    String* res = ctx->GetEngine()->AllocString(buff.GetAt(0), buff.GetSize());
    if (res)
    {
        ctx->Push(res);
        return 1;
    }
    return 0;
}

int Integer_toInteger(Context* ctx, Value& self)
{
    ctx->Push(self);
    return 1;
}

int Integer_toReal(Context* ctx, Value& self)
{
    ctx->Push((preal_t)(self.val.integer));
    return 1;
}

int Integer_toNumber(Context* ctx, Value& self)
{
    ctx->Push(self);
    return 1;
}

int Integer_toBoolean(Context* ctx, Value& self)
{
    ctx->PushBool(self.val.integer != 0);
    return 1;
}

int Primitive_getType(Context* ctx, Value& self)
{
    Engine* eng = ctx->GetEngine();
    switch (self.tag)
    {
    case TAG_null:      ctx->Push(eng->Null_Type);    return 1;
    case TAG_boolean:   ctx->Push(eng->Boolean_Type); return 1;
    case TAG_integer:   ctx->Push(eng->Integer_Type); return 1;
    case TAG_real:      ctx->Push(eng->Real_Type);    return 1;
    default:
        if (self.tag >= TAG_basic)
        {
            Type* type = 0;
            if (type = self.val.basic->GetType())
            {
                ctx->Push(type);
                return 1;
            }
        }
    }
    return 0;
}

////////////////////////////////////////////// Real ////////////////////////////////////////////////

int Real_init(Context* ctx, Value& self)
{
    u2 argc = ctx->GetArgCount();
    
    if (argc == 0)
    {
        ctx->Push((preal_t)0.0);
    }
    else if (argc == 1)
    {
        preal_t v = ctx->ArgToReal(0);
        ctx->Push(v);
    }
    else
    {
        ctx->WrongArgCount();
    }
    return 1;
}

int Real_toString(Context* ctx, Value& self)
{
    ctx->Push(Engine::NumberToString(ctx->GetEngine(), self));
    return 1;
}

int Real_toInteger(Context* ctx, Value& self)
{
    ctx->Push((pint_t)(self.val.real));
    return 1;
}

int Real_toReal(Context* ctx, Value& self)
{
    ctx->Push(self);
    return 1;
}

int Real_toNumber(Context* ctx, Value& self)
{
    ctx->Push(self);
    return 1;
}

int Real_toBoolean(Context* ctx, Value& self)
{
    ctx->PushBool(Pika_RealToBoolean(self.val.real));
    return 1;
}

int Real_isnan(Context* ctx, Value& self)
{
    ctx->PushBool(Pika_isnan(self.val.real) != 0);
    return 1;
}

int Real_integer(Context* ctx, Value& self)
{
    preal_t r = self.val.real;
    preal_t i = 0.0;
    modf(r, &i);
    ctx->Push(i);
    return 1;
}

int Real_fraction(Context* ctx, Value& self)
{
    preal_t r = self.val.real;
    preal_t i = 0.0;
    preal_t f = modf(r, &i);
    ctx->Push(f);
    return 1;
}

/*
static int Real_IsIntegral(Context* ctx, Value& self)
{
    preal_t r = self.val.real;
    bool integral = Pika_isFinite(r) && Pika_floor(r) == r;
    ctx->PushBool(integral);
    return 1;
}
*/

//////////////////////////////////////////// Boolean ///////////////////////////////////////////////

int Boolean_toString(Context* ctx, Value& self)
{
    if (self.val.index)
    {
        ctx->Push(ctx->GetEngine()->true_String);
    }
    else
    {
        ctx->Push(ctx->GetEngine()->false_String);
    }
    return 1;
}

int Boolean_toInteger(Context* ctx, Value& self)
{
    ctx->Push((pint_t)((self.val.index) ? 1 : 0));
    return 1;
}

int Boolean_toReal(Context* ctx, Value& self)
{
    ctx->Push((preal_t)((self.val.index) ? 1.0 : 0.0));
    return 1;
}

int Boolean_toNumber(Context* ctx, Value& self)
{
    ctx->Push((pint_t)((self.val.index) ? 1 : 0));
    return 1;
}

int Boolean_toBoolean(Context* ctx, Value& self)
{
    ctx->Push(self);
    return 1;
}

int Boolean_init(Context* ctx, Value& self)
{
    u2 argc = ctx->GetArgCount();
    
    if (argc == 0)
    {
        ctx->PushFalse();
        return 1;
    }
    else if (argc == 1)
    {
        bool v = ctx->ArgToBool(0);
        ctx->PushBool(v);
        return 1;
    }
    else
    {
        ctx->WrongArgCount();
    }
    return 0;
}

///////////////////////////////////////////// World ////////////////////////////////////////////////

int Global_Print(Context* ctx, Value& self)
{
    u4 argc = ctx->GetArgCount();
    GCPAUSE(ctx->GetEngine());
    for (u4 i = 0; i < argc; ++i)
    {
        String* str  = ctx->GetEngine()->ToString(ctx, ctx->GetArg(i));
        std::cout << str->GetBuffer();
        if (i + 1 < argc)
            std::cout << " ";
    }
    std::cout << std::endl;
    std::cout.flush();
    return 0;
}

int Global_PrintLn(Context* ctx, Value& self)
{
    u4 argc = ctx->GetArgCount();
    
    for (u4 i = 0; i < argc; ++i)
    {
        String* str = ctx->GetEngine()->ToString(ctx, ctx->GetArg(i));
        std::cout << str->GetBuffer() << std::endl;
    }
    return 0;
}

int Global_printp(Context* ctx, Value& self)
{
    String* strargs[PIKA_MAX_POS_ARGS];
    String* fmt = ctx->GetStringArg(0);
    u2 argc = ctx->GetArgCount();
    
    if (argc > PIKA_MAX_POS_ARGS)
    {
        RaiseException("Too many positional arguments.");
    }    
    if (argc == 1)
    {
        printf("%s", fmt->GetBuffer());
        return 0;
    }
    for (u2 a = 1; a < argc; ++a)
    {
        strargs[ a-1 ] = ctx->ArgToString( a );
        ctx->GetArg( a ).Set(strargs[ a-1 ]);     // Keep it safe from the gc.
    }
    String* res = String::sprintp(ctx->GetEngine(), fmt, argc, strargs);
    ctx->Push( static_cast<pint_t>( res->GetLength() ) );
    printf("%s", res->GetBuffer());
    return 1;
}

int Global_sprintp(Context* ctx, Value& self)
{
    String* strargs[PIKA_MAX_POS_ARGS];
    String* fmt = ctx->GetStringArg(0);
    u2 argc = ctx->GetArgCount();
    
    if (argc > PIKA_MAX_POS_ARGS)
    {
        RaiseException("Too many positional arguments.");
    }
    
    if (argc == 1)
    {
        ctx->Push(fmt);
        return 1;
    }
    
    for (u2 a = 1; a < argc; ++a)
    {
        strargs[ a-1 ] = ctx->ArgToString( a );
        ctx->GetArg( a ).Set(strargs[ a-1 ]);     // Keep it safe from the gc.
    }
    String* res = String::sprintp(ctx->GetEngine(), fmt, argc, strargs);
    
    ctx->Push( res );
    return 1;
}

int Global_gcRun(Context* ctx, Value&)
{
    bool full_run = ctx->ArgToBool(0);
    if (full_run) {
        ctx->GetEngine()->GetGC()->FullRun();
    } else {
        ctx->GetEngine()->GetGC()->IncrementalRun();
    }
    return 0;
}

int Global_each(Context* ctx, Value&)
{
    pint_t lo = ctx->GetIntArg(0);
    pint_t hi = ctx->GetIntArg(1);
    Value  fn = ctx->GetArg(2);
    if (lo > hi)
        Swap(lo, hi);
        
    for (pint_t i = lo; i < hi; ++i)
    {
        ctx->CheckStackSpace(3);
        ctx->Push(i);
        ctx->PushNull();
        ctx->Push(fn);
        if (ctx->SetupCall(1))
        {
            ctx->Run();
        }
        ctx->PopTop();
    }
    return 0;
}

int Error_toString(Context* ctx, Value& self)
{
    Object* obj = self.val.object;
    Value vmsg;
    GCPAUSE(ctx->GetEngine());
    
    if (obj->GetSlot(ctx->GetEngine()->message_String, vmsg))
    {
        if (vmsg.IsString())
        {
            ctx->Push( vmsg );
            return 1;
        }
    }
    ctx->Push(obj->ToString());
    return 1;
}

int Error_init(Context* ctx, Value& self)
{
    GCPAUSE(ctx->GetEngine());
    
    Object* obj = self.val.object;
    String* msg = ctx->GetStringArg(0);
    
    obj->SetSlot(ctx->GetEngine()->message_String, msg);
    ctx->Push(obj);
    return 1;
}


void Error_NewFn(Engine* eng, Type* obj_type, Value& res)
{
    Object* obj = Object::StaticNew(eng, obj_type);
    res.Set(obj);
}

int Global_assert(Context* ctx, Value& self)
{
    u2 argc = ctx->GetArgCount();
    
    for (u2 a =0; a < argc; ++a) 
    {
        Value t  = ctx->GetArg(a);
        bool a_ok = false;
        
        if (t.tag == TAG_boolean)
        {
            a_ok = (t.val.index != 0);
        }
        else
        {
            a_ok = ctx->GetEngine()->ToBoolean(ctx, t);
        }
        if (!a_ok)
        {
            ctx->ReportRuntimeError(Exception::ERROR_assert, "Runtime assertion failed.");
        }
    }
    return 0;
}

Type* CreateErrorType(Engine* eng, const char* name, Type* base)
{
    Package* Pkg_World = eng->GetWorld();
    String* error_name = eng->AllocString(name);
    Type* error_type = Type::Create(eng, error_name, base, Error_NewFn, Pkg_World);
    Pkg_World->SetSlot(error_name, error_type);
    return error_type;
}

}// namespace

class GCPause : public Object
{
    PIKA_DECL(GCPause, Object)
public:
    GCPause(Engine* eng, Type* t)
            : Object(eng, t)
    {
        this->isPaused = false;
    }
    
    virtual void Pause()
    {
        if (!isPaused)
        {
            engine->GetGC()->Pause();
            isPaused = true;
        }
    }
    
    virtual void UnPause()
    {
        if (isPaused)
        {
            engine->GetGC()->Resume();
            isPaused = false;
        }
    }
    
    virtual bool IsPaused() { return isPaused; }
    
    static void onNew(Engine* eng, Type* type, Value& res);
    virtual ~GCPause() { if (isPaused) { engine->GetGC()->Resume(); } }
    virtual bool Finalize() { UnPause(); return true; }
    bool isPaused;
};

PIKA_IMPL(GCPause)

void GCPause::onNew(Engine* eng, Type* type, Value& res)
{
    GCPause* cp = 0;
    GCNEW(eng, GCPause, cp, (eng, type));
    res.Set(cp);
}

void Engine::InitializeWorld()
{
    /* !!! DO NOT RE-ARRANGE WITHOUT CHECKING DEPENDENCIES !!! */
    
    { GCPAUSE(this);
        
        static const char* StaticOpStrings[] = {
        "opAdd",
        "opSub",
        "opMul",
        "opDiv",
        "opIDiv",
        "opMod",
        "opPow",
        "opEq",
        "opNe",
        "opLt",
        "opGt",
        "opLte",
        "opGte",
        "opBitAnd",
        "opBitOr",
        "opBitXor",
        "",         // xor
        "opLsh",
        "opRsh",
        "opURsh",
        "opNot",
        "opBitNot",
        "opIncr",
        "opDecr",
        "opPos",
        "opNeg",
        "opCatSp",
        "opCat",
        "opBind",
        "opUnpack",
        "",         // N/A (tail call)
        "opCall",
        "opGet",
        "opSet",
        "opGetAt",
        "opSetAt",
        OPINIT_CSTR,
        "next",
        "iterate",
        "opAdd_r",
        "opSub_r",
        "opMul_r",
        "opDiv_r",
        "opIDiv_r",
        "opMod_r",
        "opPow_r",
        "opEq_r",
        "opNe_r",
        "opLt_r",
        "opGt_r",
        "opLte_r",
        "opGte_r",
        "opBitAnd_r",
        "opBitOr_r",
        "opBitXor_r",
        "opLsh_r",
        "opRsh_r",
        "opURsh_r",
        "opCatSp_r",
        "opCat_r",
        "opBind_r"
        };
        
        for (size_t i = 0; i < NUM_OVERRIDES; ++i)
        {
            this->override_strings[i] = AllocString(StaticOpStrings[i]);
        }
        
        String* world_String = AllocString("world");
        this->emptyString    = AllocString("");
        this->dot_String     = AllocString(".");
        
        Pkg_World = Package::Create(this, world_String);
        Pkg_World->SetSlot(world_String, Pkg_World);   // access itself by name
        
        this->string_buff.SetCapacity(512);
        this->string_buff.Clear();
                
        this->Array_String      = AllocString("Array");
        this->Enumerator_String = AllocString("Enumerator");
        this->Object_String     = AllocString("Object");
        this->OpDispose_String  = AllocString("onDispose");
        this->OpUse_String      = AllocString("onUse");
        this->Property_String   = AllocString("Property");
        this->elements_String   = AllocString("elements");
        this->false_String      = AllocString("false");
        this->indices_String    = AllocString("indices");
        this->keys_String       = AllocString("keys");
        this->length_String     = AllocString("length");
        this->loading_String    = AllocString("loading");
        this->message_String    = AllocString("message");
        this->null_String       = AllocString("null");
        this->toBoolean_String  = AllocString("toBoolean");
        this->toInteger_String  = AllocString("toInteger");
        this->toNumber_String   = AllocString("toNumber");
        this->toReal_String     = AllocString("toReal");
        this->toString_String   = AllocString("toString");
        this->true_String       = AllocString("true");
        this->userdata_String   = AllocString("UserData");
        this->names_String      = AllocString("names");
        this->values_String     = AllocString("values");
        
        String*  Imports_Str = AllocString(IMPORTS_STR);
        String*  Types_Str   = AllocString("baseTypes");
        
        /* Manually create the most basic Types and their accompanying 'meta' Types. We need to do
         * this because they are all somewhat inter-connected. In order to create a meta type we
         * need to create all the type from Value->Type and then fixup their types.
         *
         * NOTE: If changes are made to the order OR class hierarchy then testing needs to be done
         *       to ensure that the type,base are correct and that type.subtypes has the correct
         *       type.
         */
        Value_Type   = Type::Create(this, AllocString("Value"),     0,            0,                    Pkg_World, 0);
        Basic_Type   = Type::Create(this, AllocString("Basic"),     Value_Type,   0,                    Pkg_World, 0);
        Object_Type  = Type::Create(this, AllocString("Object"),    Basic_Type,   Object::Constructor,  Pkg_World, 0);
        Package_Type = Type::Create(this, AllocString("Package"),   Object_Type,  Package::Constructor, Pkg_World, 0);
        Type_Type    = Type::Create(this, AllocString("Type"),      Package_Type, Type::Constructor,    Pkg_World, 0);
        
        Type* TypeType_Type    = Type::Create(this, AllocString("Type-Type"),    Type_Type,       0, Pkg_World, 0);        
        Type* ValueType_Type   = Type::Create(this, AllocString("Value-Type"),   Type_Type,       0, Pkg_World, TypeType_Type);
        Type* BasicType_Type   = Type::Create(this, AllocString("Basic-Type"),   ValueType_Type,  0, Pkg_World, TypeType_Type);
        Type* ObjectType_Type  = Type::Create(this, AllocString("Object-Type"),  BasicType_Type,  0, Pkg_World, TypeType_Type);
        Type* PackageType_Type = Type::Create(this, AllocString("Package-Type"), ObjectType_Type, 0, Pkg_World, TypeType_Type);
        
        Value_Type   ->SetType( ValueType_Type   );
        Basic_Type   ->SetType( BasicType_Type   );
        Object_Type  ->SetType( ObjectType_Type  );
        Package_Type ->SetType( PackageType_Type );
        Type_Type    ->SetType( TypeType_Type    );
        TypeType_Type->SetType( TypeType_Type    );
        
        Array_Type = Type::Create(this, AllocString("Array"), Object_Type, Array::Constructor, Pkg_World);
        
        /* We need to go back an fix any arrays created before the Array type was created.
         * If we don't do this then the subtypes array of the following will be typeless
         * and useless.
         */
        Basic_Type   ->GetSubtypes()->SetType(Array_Type);
        Object_Type  ->GetSubtypes()->SetType(Array_Type);
        Package_Type ->GetSubtypes()->SetType(Array_Type);
        Type_Type    ->GetSubtypes()->SetType(Array_Type);
        ValueType_Type->GetSubtypes()->SetType(Array_Type);
        BasicType_Type->GetSubtypes()->SetType(Array_Type);
        ObjectType_Type->GetSubtypes()->SetType(Array_Type);
        
        /* Create the Function hierarchy. Initialization happens elsewhere.
         */
        String* Function_String = AllocString("Function");
        Function_Type       = Type::Create(this, Function_String,               Object_Type,   Function::Constructor,       Pkg_World);
        BoundFunction_Type  = Type::Create(this, AllocString("BoundFunction"),  Function_Type, BoundFunction::Constructor,  Pkg_World);
        InstanceMethod_Type = Type::Create(this, AllocString("InstanceMethod"), Function_Type, InstanceMethod::Constructor, Pkg_World);
        ClassMethod_Type    = Type::Create(this, AllocString("ClassMethod"),    Function_Type, ClassMethod::Constructor,    Pkg_World);
        LocalsObject_Type   = Type::Create(this, AllocString("LocalsObject"),   Object_Type,   LocalsObject::Constructor,   Pkg_World);
        
        /* NativeFunction and NativeMethod refer to the same C++ type, much like the Error subtypes.
         */
        NativeFunction_Type = Type::Create(this, AllocString("NativeFunction"), Function_Type, 0, Pkg_World);
        NativeFunction_Type->SetFinal(true);
        NativeFunction_Type->SetAbstract(true);
        
        NativeMethod_Type   = Type::Create(this, AllocString("NativeMethod"),   Function_Type, 0, Pkg_World);
        NativeMethod_Type->SetFinal(true);
        NativeMethod_Type->SetAbstract(true);
        
        Pkg_World->SetSlot(Function_String, Function_Type);
        
        Object::StaticInitType(this);
        Dictionary::StaticInitType(this);
        Package::StaticInitType(this);   
        Module::StaticInitType(this);
        Script::StaticInitType(this);
        Function::StaticInitType(this);
        String::StaticInitType(this);
        PathManager::StaticInitType(this);
        Generator::StaticInitType(this);
        Iterator::StaticInitType(this);
        
        GCNEW(this, PathManager, paths, (this, PathManager_Type));
        
        Pkg_Imports = OpenPackage(Imports_Str, Pkg_World, true, Slot::ATTR_protected);
        Pkg_Types   = OpenPackage(Types_Str, 0, true, Slot::ATTR_protected);
        
        Debugger::StaticInitType(this);
        Array::StaticInitType(this);
        InitSystemLIB  (this);
        Context::StaticInitType(this);
        File::StaticInitType(this);
        ByteArray::StaticInitType(this);
        Init_Annotations(this, Pkg_World);
        // GCPause /////////////////////////////////////////////////////////////////////////////////
        
        String* GCPause_String = AllocString("GCPause");
        Type* GCPause_Type = Type::Create(this, GCPause_String, Object_Type, GCPause::onNew, Pkg_World);
        this->Pkg_World->SetSlot(GCPause_String, GCPause_Type);
        
        SlotBinder<GCPause>(this, GCPause_Type)
        .Method(&GCPause::Pause, "onUse")
        .Method(&GCPause::UnPause, "onDispose")
        .PropertyR("paused?", &GCPause::IsPaused, 0);
        
        // T - methods /////////////////////////////////////////////////////////////////////////
        
        static RegisterProperty Value_Properties[] =
        {
            { "type", Primitive_getType, "getType", 0, 0 },
        };
        
        Value_Type->EnterProperties(Value_Properties, countof(Value_Properties));
        this->Pkg_World->SetSlot(this->AllocString("Basic"), this->Basic_Type);
        
        /* Initialize all the Error types. Each of these is basically an Object, we have
         * no need to create a new C++ type for Errors yet.
         */        
        static RegisterFunction Error_Functions[] =
        {
            { OPINIT_CSTR, Error_init,     1, DEF_STRICT },
            { "toString",  Error_toString, 0, DEF_STRICT },
        };
        
        String* Error_String = AllocString("Error");
        Error_Type           = Type::Create(this, Error_String, Object_Type, Error_NewFn, Pkg_World);
        
        Pkg_World->SetSlot(Error_String, Error_Type);
        Error_Type->EnterMethods(Error_Functions, countof(Error_Functions));
                
        RuntimeError_Type      = CreateErrorType(this, "RuntimeError",      Error_Type);
        TypeError_Type         = CreateErrorType(this, "TypeError",         Error_Type);        
        ReferenceError_Type    = CreateErrorType(this, "ReferenceError",    Error_Type);
        ArithmeticError_Type   = CreateErrorType(this, "ArithmeticError",   Error_Type);
        OverflowError_Type     = CreateErrorType(this, "OverflowError",     ArithmeticError_Type);
        UnderflowError_Type    = CreateErrorType(this, "UnderflowError",    ArithmeticError_Type);
        DivideByZeroError_Type = CreateErrorType(this, "DivideByZeroError", ArithmeticError_Type);
        SyntaxError_Type       = CreateErrorType(this, "SyntaxError",       Error_Type);
        IndexError_Type        = CreateErrorType(this, "IndexError",        Error_Type);      
        SystemError_Type       = CreateErrorType(this, "SystemError",       Error_Type);
        AssertError_Type       = CreateErrorType(this, "AssertError",       Error_Type);
        
        // world ///////////////////////////////////////////////////////////////////////////////////////
        
        static RegisterFunction DummyFunctions[] =
        {
            { "printp",  Global_printp,  0, DEF_VAR_ARGS },
            { "sprintp", Global_sprintp, 0, DEF_VAR_ARGS },
            { "print",   Global_Print,   0, DEF_VAR_ARGS },
            { "println", Global_PrintLn, 0, DEF_VAR_ARGS },
            { "say",     Global_PrintLn, 0, DEF_VAR_ARGS },
            { "each",    Global_each,    3, DEF_STRICT   },
            { "gcRun",   Global_gcRun,   1, DEF_STRICT   },
            { "assert",  Global_assert,  0, DEF_VAR_ARGS },
        };
        
        Pkg_World->AddNative(DummyFunctions, countof(DummyFunctions));
        Pkg_World->SetType(Package_Type);
                
        // Null ////////////////////////////////////////////////////////////////////////////////////////
        
        static RegisterFunction Null_Functions[] =
        {
            { "toString",   Null_toString,  0, 0 },
            { "toInteger",  Null_toInteger, 0, 0 },
            { "toReal",     Null_toReal,    0, 0 },
            { "toNumber",   Null_toNumber,  0, 0 },
            { "toBoolean",  Null_toBoolean, 0, 0 },
        };
        
        static RegisterFunction Null_ClassMethods[] =
        {
            { NEW_CSTR,  Null_init, 0, DEF_VAR_ARGS },
        };
        
        Null_Type = Type::Create(this, AllocString("Null"), Value_Type, 0, Pkg_World);
        Null_Type->EnterMethods(Null_Functions, countof(Null_Functions));
        Null_Type->EnterClassMethods(Null_ClassMethods, countof(Null_ClassMethods));
        Null_Type->SetFinal(true);
        Null_Type->SetAbstract(true);
        Pkg_World->SetSlot("Null", Null_Type);
        
        // Boolean /////////////////////////////////////////////////////////////////////////////////////
        
        static RegisterFunction Boolean_Functions[] =
        {
            { "toString",   Boolean_toString,  0, 0 },
            { "toInteger",  Boolean_toInteger, 0, 0 },
            { "toReal",     Boolean_toReal,    0, 0 },
            { "toNumber",   Boolean_toNumber,  0, 0 },
            { "toBoolean",  Boolean_toBoolean, 0, 0 },
        };
        
        static RegisterFunction Boolean_ClassMethods[] =
        {
            { NEW_CSTR,   Boolean_init,      0, DEF_VAR_ARGS },
        };
        
        Boolean_Type  = Type::Create(this, AllocString("Boolean"), Value_Type, 0, Pkg_World);
        Boolean_Type->EnterMethods(Boolean_Functions, countof(Boolean_Functions));
        Boolean_Type->EnterClassMethods(Boolean_ClassMethods, countof(Boolean_ClassMethods));
        Pkg_World->SetSlot("Boolean", Boolean_Type);
        Boolean_Type->SetFinal(true);
        Boolean_Type->SetAbstract(true);
        
        // Integer ////////////////////////////////////////////////////////////////////////////////
        
        static RegisterFunction Integer_Functions[] =
        {
            { "toString",   Integer_toString,  0, DEF_VAR_ARGS },
            { "toInteger",  Integer_toInteger, 0, 0 },
            { "toReal",     Integer_toReal,    0, 0 },
            { "toNumber",   Integer_toNumber,  0, 0 },
            { "toBoolean",  Integer_toBoolean, 0, 0 },
        };
        
        static RegisterFunction Integer_ClassMethods[] =
        {
            { NEW_CSTR, Integer_init, 0, DEF_VAR_ARGS },
        };
        
        Integer_Type  = Type::Create(this, AllocString("Integer"), Value_Type, 0, Pkg_World);
        Integer_Type->EnterMethods(Integer_Functions, countof(Integer_Functions));
        Integer_Type->EnterClassMethods(Integer_ClassMethods, countof(Integer_ClassMethods));
        Integer_Type->SetSlot("MAX", (pint_t)PINT_MAX);
        Integer_Type->SetSlot("MIN", (pint_t)PINT_MIN);
        Integer_Type->SetFinal(true);
        Integer_Type->SetAbstract(true);
        Pkg_World->SetSlot("Integer", Integer_Type);
        
        // Real ///////////////////////////////////////////////////////////////////////////////////
        
        static RegisterFunction Real_Functions[] =
        {
            { "toString",   Real_toString,  0, 0 },
            { "toInteger",  Real_toInteger, 0, 0 },
            { "toReal",     Real_toReal,    0, 0 },
            { "toNumber",   Real_toNumber,  0, 0 },
            { "toBoolean",  Real_toBoolean, 0, 0 },
            { "nan?",       Real_isnan,     0, 0 },
            // TODO:: finite? infinite? SignBit etc...
        };
        
        static RegisterProperty Real_Properties[] =
        {
            { "integer",  Real_integer,  "getInteger",  0, 0 },
            { "fraction", Real_fraction, "getFraction", 0, 0 },
        };
        
        static RegisterFunction Real_ClassMethods[] =
        {
            { NEW_CSTR, Real_init, 0, DEF_VAR_ARGS },
        };
        
        Real_Type  = Type::Create(this, AllocString("Real"), Value_Type, 0, Pkg_World);
        Real_Type->EnterMethods(Real_Functions, countof(Real_Functions));
        Real_Type->EnterClassMethods(Real_ClassMethods, countof(Real_ClassMethods));
        Real_Type->EnterProperties(Real_Properties, countof(Real_Properties));
        Real_Type->SetFinal(true);
        Real_Type->SetAbstract(true);
        Pkg_World->SetSlot("Real", Real_Type);
        //----------------------------------------------------------------------------------------------
        
        Initialize_ImportAPI(this);
        
        CreateRoots();
        
    }// GCPAUSE
}// InitializeWorld

}// pika
