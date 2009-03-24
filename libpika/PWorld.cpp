/*
 *  PWorld.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PPlatform.h"
#include "PLocalsObject.h"
#include <iostream>

extern void InitArrayAPI(Engine*);
extern void InitFunctionAPI(Engine*);
extern void InitObjectAPI(Engine*);
extern void InitContextAPI(Engine*);
extern void InitSystemLIB(Engine*);
extern void InitDebuggerAPI(Engine*);
extern void Initialize_ImportAPI(Engine*);
extern void InitFileAPI(Engine*);
extern void InitBytesAPI(Engine* eng);
extern void InitPackageAPI(Engine*);

static const char* StaticOpStrings[] =
    {
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
        OPNEW_CSTR,
        "",         // N/A (tail call)
        "opCall",
        "opGet",
        "opSet",
        "opDel",
        OPINIT_CSTR,
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
    
#define NUMBER2STRING_BUFF_SIZE 64
    
String* NumberToString(Engine* eng, const Value& v)
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

static int Null_toString(Context* ctx, Value& self)
{
    ctx->Push((ctx->GetEngine()->null_String));
    return 1;
}

static int Null_toInteger(Context* ctx, Value& self)
{
    ctx->Push((pint_t)0);
    return 1;
}

static int Null_toReal(Context* ctx, Value& self)
{
    ctx->Push((preal_t)0.0);
    return 1;
}

static int Null_toNumber(Context* ctx, Value& self)
{
    ctx->Push((pint_t)0);
    return 1;
}

static int Null_toBoolean(Context* ctx, Value& self)
{
    ctx->PushFalse();
    return 1;
}

static int Null_init(Context* ctx, Value& self)
{
    ctx->PushNull();
    return 1;
}

///////////////////////////////////////////// Integer //////////////////////////////////////////////

static int Integer_init(Context* ctx, Value& self)
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
static int Integer_toString(Context* ctx, Value& self)
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

static int Integer_toInteger(Context* ctx, Value& self)
{
    ctx->Push(self);
    return 1;
}

static int Integer_toReal(Context* ctx, Value& self)
{
    ctx->Push((preal_t)(self.val.integer));
    return 1;
}

static int Integer_toNumber(Context* ctx, Value& self)
{
    ctx->Push(self);
    return 1;
}

static int Integer_toBoolean(Context* ctx, Value& self)
{
    ctx->PushBool(self.val.integer != 0);
    return 1;
}

static int Primitive_getType(Context* ctx, Value& self)
{
    Engine* eng = ctx->GetEngine();
    switch (self.tag)
    {
    case TAG_null:      ctx->Push(eng->Null_Type);    return 1;
    case TAG_boolean:   ctx->Push(eng->Boolean_Type); return 1;
    case TAG_integer:   ctx->Push(eng->Integer_Type); return 1;
    case TAG_real:      ctx->Push(eng->Real_Type);    return 1;
    }
    return 0;
}

////////////////////////////////////////////// Real ////////////////////////////////////////////////

static int Real_init(Context* ctx, Value& self)
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

static int Real_toString(Context* ctx, Value& self)
{
    ctx->Push(NumberToString(ctx->GetEngine(), self));
    return 1;
}

static int Real_toInteger(Context* ctx, Value& self)
{
    ctx->Push((pint_t)(self.val.real));
    return 1;
}

static int Real_toReal(Context* ctx, Value& self)
{
    ctx->Push(self);
    return 1;
}

static int Real_toNumber(Context* ctx, Value& self)
{
    ctx->Push(self);
    return 1;
}

static int Real_toBoolean(Context* ctx, Value& self)
{
    ctx->PushBool(Pika_RealToBoolean(self.val.real));
    return 1;
}

static int Real_isnan(Context* ctx, Value& self)
{
    ctx->PushBool(Pika_isnan(self.val.real) != 0);
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

static int Boolean_toString(Context* ctx, Value& self)
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

static int Boolean_toInteger(Context* ctx, Value& self)
{
    ctx->Push((pint_t)((self.val.index) ? 1 : 0));
    return 1;
}

static int Boolean_toReal(Context* ctx, Value& self)
{
    ctx->Push((preal_t)((self.val.index) ? 1.0 : 0.0));
    return 1;
}

static int Boolean_toNumber(Context* ctx, Value& self)
{
    ctx->Push((pint_t)((self.val.index) ? 1 : 0));
    return 1;
}

static int Boolean_toBoolean(Context* ctx, Value& self)
{
    ctx->Push(self);
    return 1;
}

static int Boolean_init(Context* ctx, Value& self)
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

int PrintValue(Context* ctx, Value& v)
{
    int obj_type = v.tag;
    switch (obj_type)
    {
    case TAG_null:
        std::cout << "null";
        break;
    case TAG_boolean:
        if (v.val.index)
            std::cout << "true";
        else
            std::cout << "false";
        break;
    case TAG_integer:
        std::cout << v.val.integer;
        break;
    case TAG_real:
        std::cout << v.val.real;
        break;
    case TAG_index:
        std::cout << v.val.index;
        break;
    case TAG_string:
        std::cout << v.val.str->GetBuffer();
        break;
    case TAG_object:
    {
        String* str = ctx->GetEngine()->ToString(ctx, v);
        std::cout << str->GetBuffer();
    }
    break;
    default:
        std::cout << std::hex << v.val.index;
    }
    return 0;
}

static int Dummy_Print(Context* ctx, Value& self)
{
    u4 argc = ctx->GetArgCount();
    GCPAUSE(ctx->GetEngine());
    for (u4 i = 0; i < argc; ++i)
    {
        PrintValue(ctx, ctx->GetArg(i));
        if (i + 1 < argc)
            std::cout << " ";
    }
    std::cout << std::endl;
    return 0;
}

static int Dummy_List(Context* ctx, Value& self)
{
    u4 argc = ctx->GetArgCount();
    GCPAUSE(ctx->GetEngine());
    for (u4 i = 0; i < argc; ++i)
    {
        PrintValue(ctx, ctx->GetArg(i));
        if (i + 1 < argc)
            std::cout << ", ";
    }
    std::cout << std::endl;
    return 0;
}

static int Dummy_PrintLn(Context* ctx, Value& self)
{
    u4 argc = ctx->GetArgCount();
    
    for (u4 i = 0; i < argc; ++i)
    {
        Value*  argv = ctx->GetArgs();
        String* str  = ctx->GetEngine()->ToString(ctx, argv[i]);
        puts(str->GetBuffer());
    }
    return 0;
}

// a simple positional formatter used by sprintp and printp. not general purpose.

#define PIKA_MAX_POS_ARGS 16

static String* Pika_sprintp(Context* ctx,    // context
                             String*  fmt,    // format string
                             u2       argc,   // argument count + 1
                             String*  args[]) // arguments, args[0] is ignored
{
    Engine* eng = ctx->GetEngine();
    TStringBuffer& buff = eng->string_buff;
    
    if (fmt->GetLength())
    {
        buff.Clear();
        
        const char* cfmt = fmt->GetBuffer();
        const char* fmtend = cfmt + fmt->GetLength();
        
        while (cfmt < fmtend)
        {
            int ch = *cfmt++;
            if (ch == '\\' && cfmt < fmtend)
            {
                ch = *cfmt++;
                if (ch != '%')
                    buff.Push('\\');
                buff.Push(ch);
                continue;
            }
            else if (ch == '%')
            {
                unsigned pos = 0;
                ch = *cfmt;
                
                if (!isdigit(ch))
                    RaiseException("Expected number after %c.", '%');
                    
                while (cfmt++ < fmtend && isdigit(ch))
                {
                    pos = pos * 10 + (ch - '0');
                    ch = *cfmt;
                }
                
                if (pos < argc && pos > 0)
                {
                    String* posstr = args[pos];
                    size_t oldsize = buff.GetSize();
                    buff.Resize(oldsize + posstr->GetLength());
                    Pika_memcpy(buff.GetAt((int)oldsize), posstr->GetBuffer(), posstr->GetLength());
                }
                else
                {
                    if (argc > 1)
                    {
                        RaiseException("positional argument %u out of range [1-%u].", pos, argc - 1);
                    }
                    else
                    {
                        RaiseException("positional argument %u used but not specified.", pos);
                    }
                }
                cfmt--;
            }
            else
            {
                buff.Push(ch);
            }
        }
        buff.Push('\0');
        return eng->AllocString(buff.GetAt(0), buff.GetSize());
    }
    return fmt;
}

static int Dummy_printp(Context* ctx, Value& self)
{
    String* strargs[PIKA_MAX_POS_ARGS];
    String* fmt = ctx->GetStringArg(0);
    u2 argc = ctx->GetArgCount();
    
    if (argc > PIKA_MAX_POS_ARGS)
    {
        RaiseException("Too many positional arguments.");
    }
    strargs[0] = fmt;
    
    for (u2 a = 1 ; a < argc ; ++a)
    {
        strargs[a] = ctx->ArgToString(a);
        ctx->GetArg(a).Set(strargs[ a ]);     // Keep it safe from the gc.
    }
    String* res = Pika_sprintp(ctx, fmt, argc, strargs);
    ctx->Push( static_cast<pint_t>( res->GetLength() ) );
    printf("%s", res->GetBuffer());
    return 1;
}

static int Dummy_sprintp(Context* ctx, Value& self)
{
    String* strargs[PIKA_MAX_POS_ARGS];
    String* fmt = ctx->GetStringArg(0);
    u2 argc = ctx->GetArgCount();
    
    if (argc > PIKA_MAX_POS_ARGS)
    {
        RaiseException("Too many positional arguments.");
    }
    strargs[0] = fmt;
    
    for (u2 a = 1 ; a < argc ; ++a)
    {
        strargs[ a ] = ctx->ArgToString(a);
        ctx->GetArg(a).Set(strargs[ a ]);     // Keep it safe from the gc.
    }
    String* res = Pika_sprintp(ctx, fmt, argc, strargs);
    
    ctx->Push( res );
    return 1;
}
    
static int Global_range(Context* ctx, Value& self)
{
    u2 argc = ctx->GetArgCount();
    pint_t from = 0;
    pint_t to = 0;
    pint_t step = 0;
    
    if (argc == 1)
    {
        to   = ctx->GetIntArg(0);
    }
    else if (argc == 2)
    {
        from = ctx->GetIntArg(0);
        to   = ctx->GetIntArg(1);
    }
    else if (argc == 3)
    {
        from = ctx->GetIntArg(0);
        to   = ctx->GetIntArg(1);
        step = ctx->GetIntArg(2);
    }
    else
    {
        ctx->WrongArgCount();
    }
    
    Enumerator* e = CreateRangeEnumerator(ctx->GetEngine(), from, to, step);
    ctx->Push(e);
    return 1;
}

static int Global_each(Context* ctx, Value&)
{
    pint_t lo = ctx->GetIntArg(0);
    pint_t hi = ctx->GetIntArg(1);
    Value  fn = ctx->GetArg(2);
    if (lo > hi)
        Swap(lo,hi);
    
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

static int Global_addSearchPath(Context* ctx, Value&)
{
    u2 a = 0;
    pint_t count = 0;
    Engine* eng = ctx->GetEngine();
    for ( a = 0; a < ctx->GetArgCount(); ++a )
    {
        Value& v = ctx->GetArg( a );
        
        if (v.tag != TAG_string)
            continue;
            
        String* path = v.val.str;
        if ( !path->HasNulls() && Pika_FileExists( path->GetBuffer() ) )
        {
            eng->AddSearchPath( path );
            ++count;
        }
    }
    ctx->Push( count );
    return 1;
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
    
int null_Function(Context*, Value&) { return 0; }

static void Error_NewFn(Engine* eng, Type* obj_type, Value& res)
{
    Object* obj = Object::StaticNew(eng, obj_type);
    res.Set(obj);
}

static void Function_NewFn(Engine* eng, Type* obj_type, Value& res)
{
    Package* pkg = eng->GetWorld();
    Def* def = Def::CreateWith(eng, eng->emptyString, null_Function, 0, true, false, 0);
    Context* ctx = eng->GetActiveContext();
    if (ctx)
    {
        Package* currpkg = ctx->GetPackage();
        if (currpkg)
        {
            pkg = currpkg;
        }
    }
    Object* obj = Function::Create(eng, def, pkg, 0);
    obj->SetType(obj_type);
    res.Set(obj);
}

static int Basic_getType(Context* ctx, Value& self)
{
    if (self.tag >= TAG_basic)
    {
        ctx->Push(self.val.basic->GetType());
        return 1;
    }
    return 0;
}

extern void Object_NewFn(Engine*, Type*, Value&);
extern void Package_NewFn(Engine*, Type*, Value&);
extern void TypeObj_NewFn(Engine*, Type*, Value&);
extern void Array_NewFn(Engine*, Type*, Value&);

namespace pika
{

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
        
        GCNEW(this, PathManager, paths, (this));
        
        this->missing_String    = AllocString("missing!");
        this->OpDispose_String  = AllocString("onDispose");
        this->OpUse_String      = AllocString("onUse");
        
        this->loading_String    = AllocString("loading");
        this->length_String     = AllocString("length");
        
        this->Object_String     = AllocString("Object");        
        this->Enumerator_String = AllocString("Enumerator");
        this->Property_String   = AllocString("Property");
        this->userdata_String   = AllocString("UserData");
        this->null_String       = AllocString("null");
        this->values_String     = AllocString("values");
        this->keys_String       = AllocString("keys");
        this->elements_String   = AllocString("elements");
        this->indices_String    = AllocString("indices");
        this->Array_String      = AllocString("Array");
        
        this->toNumber_String  = AllocString("toNumber");
        this->toString_String  = AllocString("toString");
        this->toInteger_String = AllocString("toInteger");
        this->toReal_String    = AllocString("toReal");
        this->toBoolean_String = AllocString("toBoolean");
        this->true_String      = AllocString("true");
        this->false_String     = AllocString("false");
        this->message_String   = AllocString("message");
        
        String*  Imports_Str = AllocString(IMPORTS_STR);
        String*  Types_Str   = AllocString("baseTypes");
                
        Basic_Type   = Type::Create(this, AllocString("BasicObj"),  0,            0,             Pkg_World, 0);
        Object_Type  = Type::Create(this, AllocString("Object"),    Basic_Type,   Object_NewFn,  Pkg_World, 0);
        Package_Type = Type::Create(this, AllocString("Package"),   Object_Type,  Package_NewFn, Pkg_World, 0);
        Type_Type    = Type::Create(this, AllocString("Type"),      Package_Type, TypeObj_NewFn, Pkg_World, 0);        
        
        Type* TypeType_Type    = Type::Create(this, AllocString("Type Type"),     Type_Type, 0, Pkg_World, 0);
        Type* PackageType_Type = Type::Create(this, AllocString("Package Type"),  Type_Type, 0, Pkg_World, TypeType_Type);
        Type* BasicType_Type   = Type::Create(this, AllocString("BasicObj Type"), Type_Type, 0, Pkg_World, TypeType_Type);
        Type* ObjectType_Type  = Type::Create(this, AllocString("Object Type"),   Type_Type, 0, Pkg_World, TypeType_Type);
        
        Basic_Type   ->SetType( BasicType_Type  );
        Object_Type  ->SetType( ObjectType_Type  );
        Package_Type ->SetType( PackageType_Type );
        Type_Type    ->SetType( TypeType_Type    );
        TypeType_Type->SetType( TypeType_Type    );
        
        Array_Type = Type::Create(this, AllocString("Array"), Object_Type, Array_NewFn, Pkg_World);
        
        Basic_Type   ->GetSubtypes()->SetType(Array_Type);
        Object_Type  ->GetSubtypes()->SetType(Array_Type);
        Package_Type ->GetSubtypes()->SetType(Array_Type);
        Type_Type    ->GetSubtypes()->SetType(Array_Type);
        
        //Basic_Type->GetType()->GetSubtypes()->SetType(Array_Type);
        Object_Type->GetType()->GetSubtypes()->SetType(Array_Type);
        
        String* Function_String = AllocString("Function");
        Function_Type       = Type::Create(this, Function_String,               Object_Type,   Function_NewFn, Pkg_World);
        BoundFunction_Type  = Type::Create(this, AllocString("BoundFunction"),  Function_Type, 0, Pkg_World); // TODO: Add native constructor.
        InstanceMethod_Type = Type::Create(this, AllocString("InstanceMethod"), Function_Type, 0, Pkg_World); // TODO: Add native constructor.
        ClassMethod_Type    = Type::Create(this, AllocString("ClassMethod"),    Function_Type, 0, Pkg_World); // TODO: Add native constructor.
        LocalsObject_Type   = Type::Create(this, AllocString("LocalsObject"),   Object_Type,   0, Pkg_World); // TODO: Add native constructor.
        
        NativeFunction_Type = Type::Create(this, AllocString("NativeFunction"), Function_Type, 0, Pkg_World);
        NativeFunction_Type->SetFinal(true);
        NativeFunction_Type->SetAbstract(true);
        
        NativeMethod_Type   = Type::Create(this, AllocString("NativeMethod"),   Function_Type, 0, Pkg_World);        
        NativeMethod_Type->SetFinal(true);
        NativeMethod_Type->SetAbstract(true);
        
        Pkg_World->SetSlot(Function_String, Function_Type);
        
        InitObjectAPI  (this);
        InitPackageAPI (this);
        InitFunctionAPI(this);
        InitStringAPI  (this);
        
        Pkg_Imports = OpenPackage(Imports_Str, Pkg_World, true, Slot::ATTR_protected);
        Pkg_Types   = OpenPackage(Types_Str, 0, true, Slot::ATTR_protected);
        
        InitDebuggerAPI(this);
        InitArrayAPI   (this);
        InitSystemLIB  (this);
        InitContextAPI (this);
        InitFileAPI    (this);
        InitBytesAPI   (this);
        
        // GCPause /////////////////////////////////////////////////////////////////////////////////
        
        String* GCPause_String = AllocString("GCPause");
        Type* GCPause_Type = Type::Create(this, GCPause_String, Object_Type, GCPause::onNew, Pkg_World, 0);
        this->Pkg_World->SetSlot(GCPause_String, GCPause_Type);
        
        SlotBinder<GCPause>(this, GCPause_Type, this->Pkg_World)
        .Method(&GCPause::Pause, "onUse")
        .Method(&GCPause::UnPause, "onDispose")
        .PropertyR("paused?", &GCPause::IsPaused, 0);
        
        // Basic - methods /////////////////////////////////////////////////////////////////////////
        
        static RegisterProperty Basic_properties[] =
        {
            { "type",  Basic_getType,  "getType", 0, 0 },           
        };
        
        Basic_Type->EnterProperties(Basic_properties, countof(Basic_properties));
        this->Pkg_World->SetSlot(this->AllocString("BasicObj"), this->Basic_Type);
        
        // Error ///////////////////////////////////////////////////////////////////////////////////
        
        static RegisterFunction Error_Functions[] =
        {
            { OPINIT_CSTR,  Error_init,     1, 0, 1 },
            {"toString",    Error_toString, 0, 0, 1 },
        };
        
        String* Error_String = AllocString("Error");
        Error_Type           = Type::Create(this, Error_String, Object_Type, Error_NewFn, Pkg_World);
        
        Pkg_World->SetSlot(Error_String, Error_Type);
        Error_Type->EnterMethods(Error_Functions, countof(Error_Functions));
        
        String* error_name = AllocString("RuntimeError");
        RuntimeError_Type = Type::Create(this, error_name, Error_Type, Error_NewFn, Pkg_World);
        Pkg_World->SetSlot(error_name, RuntimeError_Type);
        
        error_name = AllocString("TypeError");
        TypeError_Type = Type::Create(this, error_name, Error_Type, Error_NewFn, Pkg_World);
        Pkg_World->SetSlot(error_name, TypeError_Type);
        
        error_name = AllocString("ReferenceError");
        ReferenceError_Type = Type::Create(this, error_name, Error_Type, Error_NewFn, Pkg_World);
        Pkg_World->SetSlot(error_name, ReferenceError_Type);
        
        error_name = AllocString("ArithmeticError");
        ArithmeticError_Type = Type::Create(this, error_name, Error_Type, Error_NewFn, Pkg_World);
        Pkg_World->SetSlot(error_name, ArithmeticError_Type);
        
        error_name = AllocString("OverflowError");
        OverflowError_Type = Type::Create(this, error_name, ArithmeticError_Type, Error_NewFn, Pkg_World);
        Pkg_World->SetSlot(error_name, OverflowError_Type);
        
        error_name = AllocString("SyntaxError");
        SyntaxError_Type = Type::Create(this, error_name, Error_Type, Error_NewFn, Pkg_World);
        Pkg_World->SetSlot(error_name, SyntaxError_Type);
        
        error_name = AllocString("IndexError");
        IndexError_Type = Type::Create(this, error_name, Error_Type, Error_NewFn, Pkg_World);
        Pkg_World->SetSlot(error_name, IndexError_Type);
        
        error_name = AllocString("SystemError");
        SystemError_Type = Type::Create(this, error_name, Error_Type, Error_NewFn, Pkg_World);
        Pkg_World->SetSlot(error_name, SystemError_Type);
        
        error_name = AllocString("AssertError");
        AssertError_Type = Type::Create(this, error_name, RuntimeError_Type, Error_NewFn, Pkg_World);
        Pkg_World->SetSlot(error_name, AssertError_Type);
        
        // World ...////////////////////////////////////////////////////////////////////////////////////
        
        static RegisterFunction DummyFunctions[] =
        {
            { "printp",        Dummy_printp,         0, 1, 0 },
            { "sprintp",       Dummy_sprintp,        0, 1, 0 },
            { "print",         Dummy_Print,          0, 1, 0 },
            { "list",          Dummy_List,           0, 1, 0 },            
            { "println",       Dummy_PrintLn,        0, 1, 0 },
            { "say",           Dummy_PrintLn,        0, 1, 0 },
            { "range",         Global_range,         0, 1, 0 },
            { "addSearchPath", Global_addSearchPath, 0, 1, 0 },
            { "each",          Global_each,          3, 0, 1 },
        };
        Pkg_World->AddNative(DummyFunctions, countof(DummyFunctions));
        Pkg_World->SetType(Package_Type);
        
        static RegisterProperty Value_properties[] =
        {
            { "type", Primitive_getType, "getType", 0, 0 },
        };
        
        // Null ////////////////////////////////////////////////////////////////////////////////////////

        static RegisterFunction Null_Functions[] =
        {
            { "toString",   Null_toString,  0, 0, 0 },
            { "toInteger",  Null_toInteger, 0, 0, 0 },
            { "toReal",     Null_toReal,    0, 0, 0 },
            { "toNumber",   Null_toNumber,  0, 0, 0 },
            { "toBoolean",  Null_toBoolean, 0, 0, 0 },
        };

        static RegisterFunction Null_ClassMethods[] =
        {
            { OPNEW_CSTR,   Null_init,      0, 1, 0 },
        };

        Null_Type = Type::Create(this, AllocString("Null"), 0, 0, Pkg_World);
        Null_Type->EnterMethods(Null_Functions, countof(Null_Functions));
        Null_Type->EnterClassMethods(Null_ClassMethods, countof(Null_ClassMethods));
        Pkg_World->SetSlot("Null", Null_Type);
        Null_Type->SetFinal(true);
        Null_Type->SetAbstract(true);
        Null_Type->SetType(Type_Type);/// XXX: ????
        
        Null_Type->EnterProperties(Value_properties, countof(Value_properties));
        
        // Boolean /////////////////////////////////////////////////////////////////////////////////////

        static RegisterFunction Boolean_Functions[] =
        {
            { "toString",   Boolean_toString,  0, 0, 0 },
            { "toInteger",  Boolean_toInteger, 0, 0, 0 },
            { "toReal",     Boolean_toReal,    0, 0, 0 },
            { "toNumber",   Boolean_toNumber,  0, 0, 0 },
            { "toBoolean",  Boolean_toBoolean, 0, 0, 0 },
        };          
        static RegisterFunction Boolean_ClassMethods[] =
        {
            { OPNEW_CSTR,   Boolean_init,      0, 1, 0 },
        };          
        Boolean_Type  = Type::Create(this, AllocString("Boolean"), 0, 0, Pkg_World);
        Boolean_Type->EnterMethods(Boolean_Functions, countof(Boolean_Functions));
        Boolean_Type->EnterClassMethods(Boolean_ClassMethods, countof(Boolean_ClassMethods));
        Pkg_World->SetSlot("Boolean", Boolean_Type);
        Boolean_Type->SetFinal(true);
        Boolean_Type->SetAbstract(true);
        
        Boolean_Type->EnterProperties(Value_properties, countof(Value_properties));        
        
        // Integer ////////////////////////////////////////////////////////////////////////////////

        static RegisterFunction Integer_Functions[] =
        {
            { "toString",   Integer_toString,  0, 1, 0 },
            { "toInteger",  Integer_toInteger, 0, 0, 0 },
            { "toReal",     Integer_toReal,    0, 0, 0 },
            { "toNumber",   Integer_toNumber,  0, 0, 0 },
            { "toBoolean",  Integer_toBoolean, 0, 0, 0 },            
        };        
        static RegisterFunction Integer_ClassMethods[] =
        {
        { OPNEW_CSTR,       Integer_init,      0, 1, 0 },
        };        
        Integer_Type  = Type::Create(this, AllocString("Integer"), 0, 0, Pkg_World);
        Integer_Type->EnterMethods(Integer_Functions, countof(Integer_Functions));
        Integer_Type->EnterClassMethods(Integer_ClassMethods, countof(Integer_ClassMethods));
        Integer_Type->EnterProperties(Value_properties, countof(Value_properties));
        Integer_Type->SetSlot("MAX", (pint_t)PINT_MAX);
        Integer_Type->SetSlot("MIN", (pint_t)PINT_MIN);
        Pkg_World->SetSlot("Integer", Integer_Type);
        Integer_Type->SetFinal(true);
        Integer_Type->SetAbstract(true);
   
        // Real ///////////////////////////////////////////////////////////////////////////////////

        static RegisterFunction Real_Functions[] =
        {
            { "toString",   Real_toString,  0, 0, 0 },
            { "toInteger",  Real_toInteger, 0, 0, 0 },
            { "toReal",     Real_toReal,    0, 0, 0 },
            { "toNumber",   Real_toNumber,  0, 0, 0 },
            { "toBoolean",  Real_toBoolean, 0, 0, 0 },
            { "nan?",       Real_isnan,     0, 0, 0 },
            // TODO:: Finite? Infinite? SignBit etc...
        };
        static RegisterFunction Real_ClassMethods[] =
        {
            { OPNEW_CSTR,   Real_init,      0, 1, 0 },
        };
        Real_Type  = Type::Create(this, AllocString("Real"), 0, 0, Pkg_World);
        Real_Type->EnterMethods(Real_Functions, countof(Real_Functions));
        Real_Type->EnterClassMethods(Real_ClassMethods, countof(Real_ClassMethods));
        Pkg_World->SetSlot("Real", Real_Type);
        Real_Type->SetFinal(true);
        Real_Type->SetAbstract(true);
        Real_Type->EnterProperties(Value_properties, countof(Value_properties));        
        //----------------------------------------------------------------------------------------------
        Initialize_ImportAPI(this);
                
        CreateRoots();
        
    }// GCPAUSE
}// Engine::InitializeWorld

}// pika
