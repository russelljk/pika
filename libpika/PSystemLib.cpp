/*
 *  PSystemlib.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PNativeBind.h"
#include "PPlatform.h"
#include "PRandom.h"

#include <cmath>
#include <cfloat>
#include <ctime>

/* Uncomment to include "." and ".." when reading directory entries. */
/* #define PIKA_READDIR_INCLUDE_SPECIAL_ENTRIES */

#define PIKA_PI         ((preal_t) 3.14159265358979323846264338327950288  )
#define PIKA_E          ((preal_t) 2.71828182845904523536028747135266250  )
#define PIKA_LN10       ((preal_t) 2.30258509299404568401799145468436421  )
#define PIKA_LN2        ((preal_t) 0.693147180559945309417232121458176568 )
#define PIKA_LOG2E      ((preal_t) 1.44269504088896340735992468100189214  )
#define PIKA_LOG10E     ((preal_t) 0.434294481903251827651128918916605082 )
#define PIKA_SQRT1_2    ((preal_t) 0.707106781186547524400844362104849039 )
#define PIKA_SQRT2      ((preal_t) 1.41421356237309504880168872420969808  )

namespace pika {

namespace {
int os_sleep(Context* ctx, Value&)
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

preal_t os_clock()
{
    return static_cast<preal_t>(clock()) / static_cast<preal_t>(CLOCKS_PER_SEC);
}

pint_t os_time()
{
    return (pint_t)Pika_Milliseconds();
}

pint_t math_round(preal_t r)
{
    return ((pint_t)CopySign(Floor(r + (preal_t)0.5), r));
}

preal_t math_power(preal_t f, preal_t p)
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

int math_Abs(Context* ctx, Value&)
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

int os_system(Nullable<const char*> str)
{
    return system(str);
}

// TODO: math_max should handle >= 2 args and any number type
pint_t math_max(pint_t a, pint_t b)
{
    return (a >= b) ? a : b;
}

// TODO: math_min should handle >= 2 args and any number type
pint_t math_min(pint_t a, pint_t b)
{
    return (a <= b) ? a : b;
}

int os_getFullPath(Context* ctx, Value&)
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

int os_getCurrentDir(Context* ctx, Value&)
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

int os_removeFile(Context* ctx, Value&)
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

int os_moveFile(Context* ctx, Value&)
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

int os_copyFile(Context* ctx, Value&)
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

int os_readDir(Context* ctx, Value&)
{
    Engine*  eng  = ctx->GetEngine();
    String*  path = ctx->GetStringArg(0);
    ReadDir* dir  = 0;
    GCNEW(eng, ReadDir, dir, (eng, path));
    ctx->Push( dir );
    return 1;
}

int os_fileExtOf(Context* ctx, Value&)
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

int os_addPath(Context* ctx, Value&)
{
    Engine* eng = ctx->GetEngine();
    for (size_t a = 0; a < ctx->GetArgCount(); ++a)
    {
        String* path = ctx->GetStringArg(a);
        if ( !path->HasNulls() )
            eng->AddSearchPath(path);
    }
    return 0;
}

int os_addEnvPath(Context* ctx, Value&)
{
    Engine* eng = ctx->GetEngine();
    for (size_t a = 0; a < ctx->GetArgCount(); ++a)
    {
        String* path = ctx->GetStringArg(a);
        if ( !path->HasNulls() && path->GetLength() > 0)
            eng->AddEnvPath(path->GetBuffer());
    }
    return 0;
}

String* FindFileName(Engine* eng, String* path)
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

int os_fileNameOf(Context* ctx, Value&)
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

#define _lrotl(x, n)    ((((puint_t)(x)) << ((pint_t) ((n) & ROTN))) | (((puint_t)(x)) >> ((pint_t) ((-(n)) & ROTN))))
#define _lrotr(x, n)    ((((puint_t)(x)) >> ((pint_t) ((n) & ROTN))) | (((puint_t)(x)) << ((pint_t) ((-(n)) & ROTN))))

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
    GCPAUSE(eng);
    
    Package* world_Package = eng->GetWorld();
    String*  math_String   = eng->AllocString("math");
    Package* math_Package  = Package::Create(eng, math_String, world_Package);
    
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
    .StaticMethod( math_min,     "min")
    .StaticMethod( math_max,     "max")
    .StaticMethod( math_round,   "round")
    .StaticMethod( math_power,   "power")
    .StaticMethod( Log10,        "log10")
    .StaticMethod( Log,          "log")
    .StaticMethod( Floor,        "floor")
    .StaticMethod( Ceil,         "ceiling")
    .StaticMethod( Exp,          "exp")
    .Register    ( math_Abs,     "abs")
    .Constant    ( PIKA_PI,      "PI")
    .Constant    ( PIKA_E,       "E")
    .Constant    ( PIKA_LN10,    "LN10")
    .Constant    ( PIKA_LN2,     "LN2")
    .Constant    ( PIKA_LOG2E,   "LOG2E")
    .Constant    ( PIKA_LOG10E,  "LOG10E")
    .Constant    ( PIKA_SQRT1_2, "SQRT1_2")
    .Constant    ( PIKA_SQRT2,   "SQRT2")
    ;
    
    Random::StaticInitType(math_Package, eng);
    eng->PutImport(math_String, math_Package);
    ctx->Push(math_Package);
    return 1;
}

int os_lib_load(Context* ctx, Value&)
{
    Engine* eng = ctx->GetEngine();
    GCPAUSE(eng);
    
    Package* world_Package = eng->GetWorld();
    String*  os_String     = eng->AllocString("os");
    Package* os_Package    = eng->OpenPackage(os_String, world_Package, false);
    
	SlotBinder<Object>(eng, os_Package, os_Package)
    .StaticMethod( os_clock,                 "clock")
    .StaticMethod( os_system,                "system")
    .StaticMethod( os_time,                  "time")
    .StaticMethod( getenv,                   "getEnv")
    .StaticMethod( setenv,                   "setEnv")
    .StaticMethod( unsetenv,                 "unSetEnv")
    .StaticMethod( Pika_FileExists,          "fileExists?") // Checks for the existence of a file or directory.
    .StaticMethod( Pika_IsFile,              "file?")       // Checks for the existence of a file.
    .StaticMethod( Pika_IsDirectory,         "dir?")        // Checks for the existence of a directory.
    .StaticMethod( Pika_CreateDirectory,     "makeDir")     // Makes a new directory.
    .StaticMethod( Pika_RemoveDirectory,     "removeDir")   // Removes a directory.
    .StaticMethod( Pika_SetCurrentDirectory, "setCurrDir")  // Changes the current directory.
    .Register    ( os_sleep,                 "sleep")       // Sleep a given number of ms.
    .Register    ( os_readDir,               "readDir")     // Provides an enumerator for a directory's contents.
    .Register    ( os_moveFile,              "moveFile")    // Moves a file to a new location.
    .Register    ( os_copyFile,              "copyFile")    // Copies a file to a new location.
    .Register    ( os_removeFile,            "removeFile")  // Removes a file.
    .Register    ( os_getCurrentDir,         "getCurrDir")  // Returns the current directory.
    .Register    ( os_getFullPath,           "getFullPath")
    .Register    ( os_fileExtOf,             "fileExtOf")
    .Register    ( os_fileNameOf,            "fileNameOf")
    .Register    ( os_addPath,               "addPath")
    .Register    ( os_addEnvPath,            "addEnvPath")
    ;
    
    eng->PutImport(os_String, os_Package);
    ctx->Push(os_Package);
    return 1;
}

}// namespace

void InitSystemLIB(Engine* eng)
{
    Package* World_Package = eng->GetWorld();
    { 
        // Setup 'math' package as an import.
        String* math_String = eng->AllocString("math");
        static RegisterFunction math_FuncDef = { "math", math_lib_load, 0, 0, 0 };
        Value mathfn(Function::Create(eng, &math_FuncDef, World_Package));
        eng->PutImport(math_String, mathfn);
    }    
    { 
        // Setup 'os' package as an import.
        String*  os_String = eng->AllocString("os");
        static RegisterFunction os_FuncDef = { "os", os_lib_load, 0, 0, 0 };    
        Value osfn(Function::Create(eng, &os_FuncDef, World_Package));
        eng->PutImport(os_String, osfn);
    }
}

}// pika

