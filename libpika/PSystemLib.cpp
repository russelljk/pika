/*
 *  PSystemlib.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PNativeBind.h"
#include "PPlatform.h"
#include <cmath>
#include <cfloat>
#include <ctime>

/* Uncomment to include "." and ".." when reading directory entries. */
/* #define PIKA_READDIR_INCLUDE_SPECIAL_ENTRIES */

extern void InitRandomAPI(Package*, Engine* eng);

// Math Constants /////////////////////////////////////////////////////////////////////////////////

#define PIKA_PI         ((preal_t) 3.14159265358979323846264338327950288  )
#define PIKA_E          ((preal_t) 2.71828182845904523536028747135266250  )
#define PIKA_LN10       ((preal_t) 2.30258509299404568401799145468436421  )
#define PIKA_LN2        ((preal_t) 0.693147180559945309417232121458176568 )
#define PIKA_LOG2E      ((preal_t) 1.44269504088896340735992468100189214  )
#define PIKA_LOG10E     ((preal_t) 0.434294481903251827651128918916605082 )
#define PIKA_SQRT1_2    ((preal_t) 0.707106781186547524400844362104849039 )
#define PIKA_SQRT2      ((preal_t) 1.41421356237309504880168872420969808  )

// OS.Sleep ///////////////////////////////////////////////////////////////////////////////////////
//
// TODO: sys.sleep: We should offer a version where amt is a Real measured in seconds.
//       ie: pint_t ms = (pint_t)(amt * 1000.0)
//
///////////////////////////////////////////////////////////////////////////////////////////////////

static int OS_sleep(Context* ctx, Value&)
{
    if (1 != ctx->GetArgCount())
        ctx->WrongArgCount();
    Value& arg0 = ctx->GetArg(0);
    pint_t amt = 0;
    
    if (arg0.IsReal())
    {
        amt = (pint_t)(arg0.val.real * 1000.0);
    }
    else
    {
        amt = ctx->GetIntArg(0);
    }
    
    if (amt >= 0)
    {
        Pika_Sleep((u4)amt);
    }
    return 0;
}

// OS.Clock ///////////////////////////////////////////////////////////////////////////////////////

static preal_t OS_Clock()
{
    return static_cast<preal_t>(clock()) / static_cast<preal_t>(CLOCKS_PER_SEC);
}

// OS.Time ////////////////////////////////////////////////////////////////////////////////////////

static pint_t OS_Time()
{
    return (pint_t)Pika_Milliseconds();
}

// Math.Round /////////////////////////////////////////////////////////////////////////////////////

static pint_t Math_Round(preal_t r)
{
    return ((pint_t)CopySign(Floor(r + (preal_t)0.5), r));
}

// Math.Power /////////////////////////////////////////////////////////////////////////////////////

static preal_t Math_Power(preal_t f, preal_t p)
{
    int    sign = (f < 0 ? -1 : 1);
    preal_t absf = (f < 0 ? -f : f);
    
    if (absf < FLT_EPSILON)
    {
        return (preal_t)0.0;
    }
    else
    {
        return (sign * Pow(absf, p));
    }
}

// Math.Abs ///////////////////////////////////////////////////////////////////////////////////////

static int Math_Abs(Context* ctx, Value&)
{
    ctx->CheckParamCount(1);
    Value& arg0 = ctx->GetArg(0);
    
    if (arg0.IsInteger())
    {
        pint_t res = Pika_Abs(arg0.val.integer);
        ctx->Push(res);
        return 1;
    }
    else if (arg0.IsReal())
    {
        preal_t res = Pika_Absf(arg0.val.real);
        ctx->Push(res);
        return 1;
    }
    else
    {
        ctx->GetIntArg(0);
    }
    return 0;
}

// OS.System **************************************************************************************

static int OS_System(Nullable<const char*> str)
{
    return system(str);
}

// TODO: Math.max should handle >= 2 args and any number type
static pint_t Math_max(pint_t a, pint_t b)
{
    return (a >= b) ? a : b;
}

// TODO: Math.min should handle >= 2 args and any number type
static pint_t Math_min(pint_t a, pint_t b)
{
    return (a <= b) ? a : b;
}

static int OS_getFullPath(Context* ctx, Value&)
{
    Engine* eng  = ctx->GetEngine();
    String* arg0 = ctx->GetStringArg(0);
    char* res    = Pika_GetFullPath(arg0->GetBuffer());
    
    if (res)
    {
        String* str = eng->AllocString(res);
        Pika_free(res);
        ctx->Push(str);
    }
    else
    {
        ctx->Push(eng->emptyString);
    }
    return 1;
}

static int OS_getCurrentDir(Context* ctx, Value&)
{
    Engine* eng = ctx->GetEngine();
    char* res = Pika_GetCurrentDirectory();
    if (res)
    {
        String* str = eng->AllocString(res);
        Pika_free(res);
        ctx->Push(str);
    }
    else
    {
        ctx->Push(eng->emptyString);
    }
    return 1;
}

static int OS_removeFile(Context* ctx, Value&)
{
    u4      argc = ctx->GetArgCount();
    String* src  = 0;
    bool    ero  = false;
    
    switch (argc)
    {
    case 2: ero = ctx->GetBoolArg(1);
    case 1: src = ctx->GetStringArg(0); 
            break;
    default:
        ctx->WrongArgCount();
        return 0;
    }
    
    ctx->PushBool(Pika_DeleteFile(src->GetBuffer(), ero));
    return 1;
}

static int OS_moveFile(Context* ctx, Value&)
{
    u4      argc    = ctx->GetArgCount();
    String* src     = 0;
    String* dest    = 0;
    bool    replace = true;
    
    switch (argc)
    {
    case 3: replace = ctx->GetBoolArg(2);
    case 2: dest    = ctx->GetStringArg(1);
            src     = ctx->GetStringArg(0);
            break;
    default:
        ctx->WrongArgCount();
        return 0;
    }
    
    ctx->PushBool(Pika_MoveFile(src->GetBuffer(), dest->GetBuffer(), replace));
    return 1;
}

static int OS_copyFile(Context* ctx, Value&)
{
    u4      argc    = ctx->GetArgCount();
    String* src     = 0;
    String* dest    = 0;
    bool    replace = true;
        
    switch (argc)
    {
    case 3: replace = ctx->GetBoolArg(2);
    case 2: dest    = ctx->GetStringArg(1);
            src     = ctx->GetStringArg(0);
            break;
    default:
        ctx->WrongArgCount();
        return 0;
    }    
    
    ctx->PushBool(Pika_CopyFile(src->GetBuffer(), dest->GetBuffer(), replace));
    return 1;
}

/** Enumerates the files in a given directory. */
class ReadDir : public Enumerator
{
public:
    ReadDir(Engine* eng, String* p)
            : Enumerator(eng),
            current(0),
            path(p),
            dir(0),
            entry(0)
    {
        if (path)
        {
            dir = Pika_OpenDirectory( path->GetBuffer() );
        }
    }
    
    virtual ~ReadDir()
    {
        if (dir)
            Pika_CloseDirectory(dir);
    }
    
    virtual void MarkRefs(Collector* c)
    {
        Enumerator::MarkRefs(c);
        if (path)    path->Mark(c);
        if (current) current->Mark(c);
    }
    
    virtual bool Rewind()
    {
        if (dir)
        {
            Pika_RewindDirectory(dir);
            entry   = 0;
            current = 0;
            Advance();
            return true;
        }
        return false;
    }
    
    virtual bool IsValid()
    {
        return dir != 0 && entry != 0;
    }
    
    virtual void GetCurrent(Value& v)
    {
        if ( current )
        {
            v.Set( current );
        }
        else
        {
            v.SetNull();
        }
    }
    
    virtual void Advance()
    {
        if (dir)
        {
            if ( (entry = Pika_ReadDirectoryEntry(dir)) )
            {
#if !defined(PIKA_READDIR_INCLUDE_SPECIAL_ENTRIES)
                if (StrCmp(".", entry) == 0 || StrCmp("..", entry) == 0)
                    return Advance();
#endif
                current = engine->AllocString(entry);
            }
            else
            {
                current = engine->emptyString;
            }
        }
        else
        {
            current = engine->emptyString;
        }
    }
private:
    String*         current;
    String*         path;
    Pika_Directory* dir;
    const char*     entry;
};

static int OS_readDir(Context* ctx, Value&)
{
    Engine*  eng  = ctx->GetEngine();
    String*  path = ctx->GetStringArg(0);
    ReadDir* dir  = 0;
    GCNEW(eng, ReadDir, dir, (eng, path));
    ctx->Push( dir );
    return 1;
}

static int OS_fileExtOf(Context* ctx, Value&)
{
    String* path = ctx->GetStringArg(0);
    const char* extension = Pika_rindex(path->GetBuffer(), '.');
    if (extension)
    {
        if (++extension < (path->GetBuffer() + path->GetLength()))
        {
            ctx->Push( ctx->GetEngine()->AllocString(extension, path->GetLength() - (extension - path->GetBuffer())) );
            return 1;
        }
    }
    ctx->Push(ctx->GetEngine()->emptyString);
    return 1;
}

static int OS_addSearchPath(Context* ctx, Value&)
{
    Engine* eng = ctx->GetEngine();
    for (size_t a = 0; a < ctx->GetArgCount(); ++a)
    {
        String* path = ctx->GetStringArg(a);
        eng->AddSearchPath(path);
    }
    return 0;
}

static String* FindFileName(Engine* eng, String* path)
{
    const char* fullpath  = path->GetBuffer();
    const char* extension = Pika_rindex(fullpath, '.');
    const char* filename  = Pika_rindex(fullpath, PIKA_PATH_SEP_CHAR);
    
    if (!filename)
    {
        filename = fullpath;
    }
    else
    {
        filename++;
    }
    
    if (extension &&
       (extension > filename))
    {
        return eng->AllocString(filename, extension - filename);
    }
    else
    {
        return eng->AllocString(filename);
    }
    return 0;
}

static int OS_fileNameOf(Context* ctx, Value&)
{
    String* path = ctx->GetStringArg(0);
    String* name = FindFileName(ctx->GetEngine(), path);
    if (name)
    {
        ctx->Push( name );
    }
    else
    {
        ctx->Push(ctx->GetEngine()->emptyString);
    }
    return 1;
}

#if defined( PIKA_64BIT_INT )
#   define ROTN 63
#else
#   define ROTN 31
#endif

#define _lrotl(x, n)        ((((puint_t)(x)) << ((pint_t) ((n) & ROTN))) | (((puint_t)(x)) >> ((pint_t) ((-(n)) & ROTN))))
#define _lrotr(x, n)        ((((puint_t)(x)) >> ((pint_t) ((n) & ROTN))) | (((puint_t)(x)) << ((pint_t) ((-(n)) & ROTN))))

pint_t Rotl(pint_t x, pint_t n)
{
    return _lrotl(x,n);
}

pint_t Rotr(pint_t x, pint_t n)
{
    return _lrotr(x,n);
}

int math_lib_load(Context* ctx, Value&)
{
    Engine* eng = ctx->GetEngine();
    Package* world_Package = eng->GetWorld();
    String*  math_String = eng->AllocString("math");
    Package* math_Package = Package::Create(eng, math_String, world_Package);
    
    ////////////////////////////////////////////////////////////////////////////////////////////////
        
    SlotBinder<Object>(eng, math_Package, math_Package)
    .StaticMethod( Rotl,         "rotl")
    .StaticMethod( Rotr,         "rotr")
    .StaticMethod( Cos,          "cos")
    .StaticMethod( Cosh,         "cosh")
    .StaticMethod( Sin,          "sin")
    .StaticMethod( Sinh,         "sinh")
    .StaticMethod( Tan,          "tan")
    .StaticMethod( Tanh,         "tanh")
    .StaticMethod( ArcCos,       "arcCos")
    .StaticMethod( ArcSin,       "arcSin")
    .StaticMethod( ArcTan,       "arcTan")
    .StaticMethod( ArcTan2,      "arcTan2")
    .StaticMethod( Sqrt,         "sqrt")
    .StaticMethod( Math_min,     "min")
    .StaticMethod( Math_max,     "max")
    .StaticMethod( Math_Round,   "round")
    .StaticMethod( Math_Power,   "power")
    .StaticMethod( Log10,        "log10")
    .StaticMethod( Log,          "log")
    .StaticMethod( Floor,        "floor")
    .StaticMethod( Ceil,         "ceiling")
    .StaticMethod( Exp,          "exp")
    .Register    ( Math_Abs,     "abs")
    .Constant    ( PIKA_PI,      "PI")
    .Constant    ( PIKA_E,       "E")
    .Constant    ( PIKA_LN10,    "LN10")
    .Constant    ( PIKA_LN2,     "LN2")
    .Constant    ( PIKA_LOG2E,   "LOG2E")
    .Constant    ( PIKA_LOG10E,  "LOG10E")
    .Constant    ( PIKA_SQRT1_2, "SQRT1_2")
    .Constant    ( PIKA_SQRT2,   "SQRT2")
    ;
    
    InitRandomAPI(math_Package, eng);
    eng->PutImport(math_String, math_Package);
    ctx->Push(math_Package);
    return 1;
}

void InitSystemLIB(Engine* eng)
{

    Package* World_Package = eng->GetWorld();
    String*  math_String   = eng->AllocString("math");
    //Package* Math_Package  = eng->OpenPackage(Math_String, World_Package, false);
    String*  OS_String     = eng->AllocString("os");
    Package* OS_Package    = eng->OpenPackage(OS_String, World_Package, false);
    
    ////////////////////////////////////////////////////////////////////////////////////////////////
    
	SlotBinder<Object>(eng, OS_Package, OS_Package)
    .Register    ( OS_sleep,                 "sleep")
    .StaticMethod( OS_Clock,                 "clock")
    .StaticMethod( OS_System,                "system")
    .StaticMethod( OS_Time,                  "time")
    .StaticMethod( getenv,                   "getEnv")
    .StaticMethod( setenv,                   "setEnv")
    .StaticMethod( unsetenv,                 "unSetEnv")
    .StaticMethod( Pika_FileExists,          "fileExists?") // Checks for the existence of a file or directory.
    .StaticMethod( Pika_IsFile,              "file?")       // Checks for the existence of a file.
    .StaticMethod( Pika_IsDirectory,         "dir?")        // Checks for the existence of a directory.
    .StaticMethod( Pika_CreateDirectory,     "makeDir")     // Makes a new directory.
    .StaticMethod( Pika_RemoveDirectory,     "RemoveDir")   // Removes a directory.
    .StaticMethod( Pika_SetCurrentDirectory, "setCurrDir")  // Changes the current directory.
    .Register    ( OS_readDir,               "readDir")     // Provides an enumerator for a directory's contents.
    .Register    ( OS_moveFile,              "moveFile")    // Moves a file to a new location.
    .Register    ( OS_copyFile,              "copyFile")    // Copies a file to a new location.
    .Register    ( OS_removeFile,            "removeFile")  // Removes a file.
    .Register    ( OS_getCurrentDir,         "getCurrDir")  // Returns the current directory.
    .Register    ( OS_getFullPath,           "getFullPath")
    .Register    ( OS_fileExtOf,             "fileExtOf")
    .Register    ( OS_fileNameOf,            "fileNameOf")
    .Register    ( OS_addSearchPath,         "addSearchPath")
    ;
    
    static RegisterFunction math_FuncDef = { "math", math_lib_load, 0, 0, 0 };
    
    Value mathfn(Function::Create(eng, &math_FuncDef, World_Package));
    
    eng->PutImport(math_String, mathfn);
    ////////////////////////////////////////////////////////////////////////////////////////////////
        
}


