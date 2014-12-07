/*
 *  PSystemlib.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PNativeBind.h"
#include "PPlatform.h"
#include "PRandom.h"
#include "PDate.h"

#include <cmath>
#include <cfloat>
#include <ctime>

/* Uncomment to include "." and ".." when reading directory entries. */

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

PIKA_DOC(os_sleep, "/(amt)\
\n\
Suspend the program for a given number of milliseconds. If |amt| is a [Real] \
then it will be treated as a seconds. Otherwise |amt| should be an [Integer] \
value containing the number of milliseconds.\
");

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

PIKA_DOC(os_clock, "/()"
"\n"
"Returns the number of seconds since the start of the program."
" This has no relation to the execution time of the current [Script] or [Context]."
)

preal_t os_clock()
{
    return static_cast<preal_t>(clock()) / static_cast<preal_t>(CLOCKS_PER_SEC);
}

PIKA_DOC(os_time, "/()"
"\n"
"Returns the time in milliseconds."
" The value returned and its precision may vary across platforms."
" As a result programs should deal with relative values instead of absolute ones when used as a timer."
)

pint_t os_time()
{
    return (pint_t)Pika_Milliseconds();
}

PIKA_DOC(math_round, "/(x)\
\n\
Round the [Real real] |x| to the nearest [Integer integer].")

pint_t math_round(preal_t r)
{
    return ((pint_t)CopySign(Floor(r + (preal_t)0.5), r));
}

PIKA_DOC(math_power, "/(x, y)\
\n\
Return |x| to the power of |y|.")

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

PIKA_DOC(math_abs, "/(x)\
\n\
Returns the absolute value of |x|.")

int math_Abs(Context* ctx, Value&)
{
    ctx->CheckArgCount(1);
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

PIKA_DOC(os_system, "/([cmd])"
"\n"
"Performs the system command |cmd|."
" The return value will be a system depended value that determines if the command was successfully executed."
" Call with no arguments to check whether or not a command '''can''' be executed."
)
int os_system(Context* ctx, Value&)
{
    pint_t res = 0;
    u2 argc = ctx->GetArgCount();
    if (argc == 1) {
        String* str = ctx->GetStringArg(0);
        res = system(str->GetBuffer());
    } else if (argc == 0) {
        res = system(0);
    } else {
        ctx->WrongArgCount();
        return 0;
    }
    ctx->Push((pint_t)res);
    return 1;
}

int OpArguments(Context* ctx, const Opcode op, const OpOverride ovr, const OpOverride ovr_r)
{
    u2 argc = ctx->GetArgCount();
    if (argc == 0)
    {
        ctx->WrongArgCount();
    }
    
    Value start = ctx->GetArg(0);
    u2 curr = 1;
    ctx->CheckStackSpace(3);
    while (curr < argc)
    {
        int numcalls = 0;
        Value next = ctx->GetArg(curr);
        ctx->Push(start);
        ctx->Push(next);
        
        ctx->OpArithBinary(op, ovr, ovr_r, numcalls);
        if (numcalls > 0)
        {
            ctx->Run();
        }
        start = ctx->PopTop();
        curr++;
    }
    ctx->Push(start);
    return 1;
}

PIKA_DOC(math_sum, "/(:va)\
\n\
Returns the summation of all the arguments given. If two values cannot be \
added an exception will be raised. All arguments should implement '''opAdd''' and '''opAdd_r''' \
since they will be called to add the values.")

int math_sum(Context* ctx, Value&)
{
    return OpArguments(ctx, OP_add, OVR_add, OVR_add_r);
}

int CompareArguments(Context* ctx, const Opcode op, const OpOverride ovr, const OpOverride ovr_r)
{
    u2 argc = ctx->GetArgCount();
    if (argc == 0)
    {
        ctx->WrongArgCount();
    }
    
    Value start = ctx->GetArg(0);
    u2 curr = 1;
    Engine* engine = ctx->GetEngine();
    ctx->CheckStackSpace(3);
    
    while (curr < argc)
    {
        int numcalls = 0;
        Value next = ctx->GetArg(curr);
        ctx->Push(next);
        ctx->Push(start);
        ctx->OpCompBinary(op, ovr, ovr_r, numcalls);
        if (numcalls > 0)
        {
            ctx->Run();
        }
        Value& b = ctx->PopTop();
        bool is_gr = false;
        if (!b.IsBoolean())
        {
            is_gr = engine->ToBoolean(ctx, b);
        }
        else
        {
            is_gr = b.val.index != 0;
        }
        
        if (is_gr)
        {
            start = next;
        }
        curr++;
    }
    ctx->Push(start);
    return 1;
}

PIKA_DOC(math_min, "/(:va)\
\n\
Returns the minimum value from the arguments given. If two values cannot be \
compared an exception will be raised. All arguments should implement '''opLt''' and '''opLt_r''' \
since they will be called to compare the values.")

int math_min(Context* ctx, Value&)
{
    return CompareArguments(ctx, OP_lt, OVR_lt, OVR_lt_r);
}

PIKA_DOC(math_max, "/(:va)\
\n\
Returns the maximum value from the arguments given. If two values cannot be \
compared an exception will be raised. All arguments should implement '''opGt''' and '''opGt_r''' \
since they will be called to compare the values.")

int math_max(Context* ctx, Value&)
{
    return CompareArguments(ctx, OP_gt, OVR_gt, OVR_gt_r);
}

PIKA_DOC(os_fullpath, "/(path)"
"\n"
"Returns the full or absolute path for |path|. If the path does not exist then \
the empty string will be returned."
)

int os_fullpath(Context* ctx, Value&)
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

PIKA_DOC(os_getcwd, "/()"
"\n"
"Returns the current working directory."
)

int os_getcwd(Context* ctx, Value&)
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

PIKA_DOC(os_removeFile, "/(src, evenReadOnly=false)\
\n\
Remove the file located at |src|.\
")

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

PIKA_DOC(os_moveFile, "/(src, dest, replace=true)\
\n\
Moves the file |src| to |dest|. If a file is already at |dest| then |replace| \
will determine if it is replaced with |src|. In comparision to |copy| this \
function will completely move the file located at |src|.\
")

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

PIKA_DOC(os_copyFile, "/(src, dest, replace=true)\
\n\
Moves the file |src| to |dest|. If a file is already at |dest| then |replace| \
will determine if it is overwritten.\
")

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

/** Iterators over the files in a given directory. */
class ReadDir : public Iterator
{
public:
    ReadDir(Engine* eng, Type* readtype, String* p, bool skip=false)
            : Iterator(eng, readtype),
            valid(false),
            skip_hidden(skip),
            current(0),
            path(p),
            dir(0),
            entry(0)
    {
        if (path)
        {
            dir = Pika_OpenDirectory( path->GetBuffer() );
            valid = Rewind();
        }
    }
    
    virtual ~ReadDir()
    {
        if (dir)
            Pika_CloseDirectory(dir);
    }
    
    virtual void MarkRefs(Collector* c)
    {
        Iterator::MarkRefs(c);
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
    
    virtual bool ToBoolean()
    {
        return valid;
    }
    
    virtual int Next(Context* ctx)
    {
        if ( current && dir != 0 && entry != 0 )
        {
            ctx->Push( current );
            Advance();
            valid = true;
            return 1;
        }
        else
        {
            valid = false;
        }
        return 0;
    }
    
    virtual void Advance()
    {
        if (dir)
        {
            if ( (entry = Pika_ReadDirectoryEntry(dir)) )
            {                
                if (skip_hidden && strchr(entry, '.') == entry)
                    return Advance();
                current = engine->GetString(entry);
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
    bool valid;
    bool skip_hidden;
    String*         current;
    String*         path;
    Pika_Directory* dir;
    const char*     entry;
};

PIKA_DOC(os_readdir, "/(path, skiphidden=false)\
\n\
Returns an [Iterator] that will enumerate all entries in the |path| given. \
If |skiphidden| is true then entries beginning with '.' will be skipped.\
")

int os_readdir(Context* ctx, Value&)
{
    Engine*  eng  = ctx->GetEngine();
    bool skip = false;
    String*  path = eng->emptyString;
    switch (ctx->GetArgCount()) {
    case 2:
        skip = ctx->GetBoolArg(1);
    case 1:
        path = ctx->GetStringArg(0);
        break;
    default:
        ctx->WrongArgCount();
    }
    ReadDir* dir  = 0;
    GCNEW(eng, ReadDir, dir, (eng, eng->Iterator_Type, path, skip));
    ctx->Push( dir );
    return 1;
}

PIKA_DOC(os_extname, "/(path)\
\n\
Returns the file exentsion of the |path|. If there is no extension then the \
empty string will be returned. If the path has multiple .'s such as file.tar.gz only the last part, 'gz', is returned.\
")

int os_extname(Context* ctx, Value&)
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

String* Basename(Engine* eng, String* path)
{
    const char* fullpath  = path->GetBuffer();
    const char* pathname  = std::max(Pika_rindex(fullpath, '/'), Pika_rindex(fullpath, '\\'));
    if (!pathname || pathname + 1 < fullpath)
    {
        return path;
    }
    else
    {
        pathname++;
    }
    return eng->GetString(fullpath, pathname - fullpath);
}

String* Filename(Engine* eng, String* path)
{
    const char* fullpath  = path->GetBuffer();
    const char* extension = Pika_rindex(fullpath, '.');
    const char* filename  = std::max(Pika_rindex(fullpath, '\\'), Pika_rindex(fullpath, '/'));
    
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

PIKA_DOC(os_filename, "/(path)\
\n\
Returns the file name of |path| with out the path or extension.\
")

int os_filename(Context* ctx, Value&)
{
    String* path = ctx->GetStringArg(0);
    String* name = Filename(ctx->GetEngine(), path);
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

PIKA_DOC(os_basename, "/(path)\
\n\
Returns the base name of |path| with out the file name or extension.\
")

int os_basename(Context* ctx, Value&)
{
    String* path = ctx->GetStringArg(0);
    String* name = Basename(ctx->GetEngine(), path);
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

PIKA_DOC(math_rotl, "/(x, n)\
\n\
Rotate the bits in |x| left by |n| positions.\
")

pint_t Rotl(pint_t x, pint_t n)
{
    return _lrotl(x,n);
}

PIKA_DOC(math_rotr, "/(x, n)\
\n\
Rotate the bits in |x| right by |n| positions.\
")

pint_t Rotr(pint_t x, pint_t n)
{
    return _lrotr(x,n);
}

PIKA_DOC(math_cos, "/(x)\
\n\
Returns the cosine of |x| in radians.")

PIKA_DOC(math_cosh, "/(x)\
\n\
Returns the hyperbolic cosine of |x|.")

PIKA_DOC(math_sin, "/(x)\
\n\
Returns the sine of |x| in radians.")

PIKA_DOC(math_sinh, "/(x)\
\n\
Returns the hyperbolic sine of |x|.")

PIKA_DOC(math_tan, "/(x)\
\n\
Returns the tangent of |x| in radians.")

PIKA_DOC(math_tanh, "/(x)\
\n\
Returns the hyperbolic tangent of |x|.")

PIKA_DOC(math_acos, "/(x)\
\n\
Returns the arc cosine of |x|.")

PIKA_DOC(math_asin, "/(x)\
\n\
Returns the arc sine of |x|.")

PIKA_DOC(math_atan, "/(x)\
\n\
Returns the arc tangent of |x|.")

PIKA_DOC(math_atan2, "/(x, y)\
\n\
Returns the arc tangent of |x|/|y|.")

PIKA_DOC(math_sqrt, "/(x)\
\n\
Returns the square root of |x|.")

PIKA_DOC(math_hypot, "/(x, y)\
\n\
Returns <tt>sqrt(|x|*|x| + |y|*|y|)</tt>.")

PIKA_DOC(math_log, "/(x)\
\n\
Returns the natural logarithm of |x|.")

PIKA_DOC(math_log10, "/(x)\
\n\
Returns the logarithm of |x| to base 10.")

PIKA_DOC(math_floor, "/(x)\
\n\
Returns |x| rounded down to the nearest [Integer integer].")

PIKA_DOC(math_ceiling, "/(x)\
\n\
Returns |x| rounded up to the nearest [Integer integer].")

PIKA_DOC(math_exp, "/(x)\
\n\
Returns <tt>e**|x|</tt>.")

int math_lib_load(Context* ctx, Value&)
{
    Engine* eng = ctx->GetEngine();
    GCPAUSE(eng);
    
    Package* world_Package = eng->GetWorld();
    String*  math_String   = eng->AllocString("math");
    Package* math_Package  = Package::Create(eng, math_String, world_Package);
    
    SlotBinder<Object>(eng, math_Package, math_Package)
    .StaticMethod( Rotl,         "rotl",  PIKA_GET_DOC(math_rotl))
    .StaticMethod( Rotr,         "rotr",  PIKA_GET_DOC(math_rotr))
    .StaticMethod( Cos,          "cos",   PIKA_GET_DOC(math_cos))
    .StaticMethod( Cosh,         "cosh",  PIKA_GET_DOC(math_cosh))
    .StaticMethod( Sin,          "sin",   PIKA_GET_DOC(math_sin))
    .StaticMethod( Sinh,         "sinh",  PIKA_GET_DOC(math_sinh))
    .StaticMethod( Tan,          "tan",   PIKA_GET_DOC(math_tan))
    .StaticMethod( Tanh,         "tanh",  PIKA_GET_DOC(math_tanh))
    .StaticMethod( ArcCos,       "acos",  PIKA_GET_DOC(math_acos))
    .StaticMethod( ArcSin,       "asin",  PIKA_GET_DOC(math_asin))
    .StaticMethod( ArcTan,       "atan",  PIKA_GET_DOC(math_atan))
    .StaticMethod( ArcTan2,      "atan2", PIKA_GET_DOC(math_atan2))
    .StaticMethod( Sqrt,         "sqrt",  PIKA_GET_DOC(math_sqrt))
    .StaticMethod( Hypot,        "hypot", PIKA_GET_DOC(math_hypot))
    .StaticMethod( math_round,   "round",   PIKA_GET_DOC(math_round))
    .StaticMethod( math_power,   "power",   PIKA_GET_DOC(math_power))
    .StaticMethod( Log,          "log",     PIKA_GET_DOC(math_log))
    .StaticMethod( Log10,        "log10",   PIKA_GET_DOC(math_log10))
    .StaticMethod( IntFloor,     "ifloor",   PIKA_GET_DOC(math_floor))
    .StaticMethod( IntCeil,      "iceiling", PIKA_GET_DOC(math_ceiling))
    .StaticMethod( Floor,        "floor",   PIKA_GET_DOC(math_floor))
    .StaticMethod( Ceil,         "ceiling", PIKA_GET_DOC(math_ceiling))
    .StaticMethod( Exp,          "exp",     PIKA_GET_DOC(math_exp))
    .Register    ( math_sum,     "sum", 0, true, false, PIKA_GET_DOC(math_sum))
    .Register    ( math_max,     "max", 0, true, false, PIKA_GET_DOC(math_max))
    .Register    ( math_min,     "min", 0, true, false, PIKA_GET_DOC(math_min))
    .Register    ( math_Abs,     "abs", 1, false, true, PIKA_GET_DOC(math_abs))
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

PIKA_DOC(os_getEnv, "/(var)"
"\n"
"Returns the environmental variable named |var|. If the variable does not exist '''null''' will be returned."
)

PIKA_DOC(os_setEnv, "/(var, val)"
"\n"
"Sets the environmental variable named |var| to value of [String] |val|. If |val| is '''null''' then the variable will be removed."
)

PIKA_DOC(os_unSetEnv, "/(var)"
"\n"
"Removes the environmental variable named |var|. This is equivalent to calling [setEnv] with a '''null''' value."
)

PIKA_DOC(os_exists, "/(path)\
\n\
Checks for the existence of a file or directory.\
")

PIKA_DOC(os_file, "/(path)\
\n\
Checks for the existence of a file. Will only return true if |path| points to a file not a directory.\
")

PIKA_DOC(os_dir, "/(dir)\
\n\
Checks for the existence of a directory. Will only return true if |path| points to a directory not a file.\
")

PIKA_DOC(os_mkdir, "/(path)\
\n\
Makes a new directory located at the |path| given.\
")

PIKA_DOC(os_rmdir, "/(path)\
\n\
Removes the directory located at the |path| given.\
")

PIKA_DOC(os_chdir, "/(path)\
\n\
Changes the current directory to the |path| given.\
")

int os_lib_load(Context* ctx, Value&)
{
    Engine* eng = ctx->GetEngine();
    GCPAUSE(eng);
    
    Package* world_Package = eng->GetWorld();
    String*  os_String     = eng->AllocString("os");
    Package* os_Package    = Package::Create(eng, os_String, world_Package);
    
	SlotBinder<Object>(eng, os_Package, os_Package)
    .StaticMethod( os_clock,                     "clock",    PIKA_GET_DOC(os_clock))
    .StaticMethod( os_time,                      "time",     PIKA_GET_DOC(os_time))
    .StaticMethod( getenv,                       "getenv",   PIKA_GET_DOC(os_getEnv))
    .StaticMethod( setenv,                       "setenv",   PIKA_GET_DOC(os_setEnv))
    .StaticMethod( unsetenv,                     "unsetenv", PIKA_GET_DOC(os_unSetEnv))
    .StaticMethod( Pika_FileExists,              "exists?",  PIKA_GET_DOC(os_exists))  
    .StaticMethod( Pika_IsFile,                  "file?",    PIKA_GET_DOC(os_file))     
    .StaticMethod( Pika_IsDirectory,             "dir?",     PIKA_GET_DOC(os_dir))       
    .StaticMethod( Pika_CreateDirectory,         "mkdir",    PIKA_GET_DOC(os_mkdir))
    .StaticMethod( Pika_RemoveDirectory,         "rmdir",    PIKA_GET_DOC(os_rmdir))
    .StaticMethod( Pika_SetCurrentDirectory,     "chdir",    PIKA_GET_DOC(os_chdir))
    .Register    ( os_system,                    "system",   0, true,  false, PIKA_GET_DOC(os_system))
    .Register    ( os_sleep,                     "sleep",    1, false, true,  PIKA_GET_DOC(os_sleep))
    .Register    ( os_readdir,                   "readdir",  0, true,  false, PIKA_GET_DOC(os_readdir))
    .Register    ( os_moveFile,                  "move",     0, true,  false, PIKA_GET_DOC(os_moveFile))
    .Register    ( os_copyFile,                  "copy",     0, true,  false, PIKA_GET_DOC(os_copyFile))
    .Register    ( os_removeFile,                "remove",   0, true,  false, PIKA_GET_DOC(os_removeFile))
    .Register    ( os_getcwd,                    "getcwd",   0, false, true, PIKA_GET_DOC(os_getcwd))
    .Register    ( os_fullpath,                  "fullpath", 1, false, true, PIKA_GET_DOC(os_fullpath))
    .Register    ( os_extname,                   "extname",  1, false, true, PIKA_GET_DOC(os_extname))
    .Register    ( os_filename,                  "filename", 1, false, true, PIKA_GET_DOC(os_filename))
    .Register    ( os_basename,                  "basename", 1, false, true, PIKA_GET_DOC(os_basename))
    .Constant    (eng->Paths(),                  "paths")
    .Constant    (eng->GetString(PIKA_PATH_SEP), "sep")
    ;
    
    Date::StaticInitType(os_Package, eng);
    
    eng->PutImport(os_String, os_Package);
    ctx->Push(os_Package);
    return 1;
}

int date_lib_load(Context* ctx, Value&)
{
    Engine* eng = ctx->GetEngine();
    GCPAUSE(eng);
    
    Package* world_Package = eng->GetWorld();
    String*  date_String     = eng->AllocString("date");
    Package* date_Package    = Package::Create(eng, date_String, world_Package);
    Date::StaticInitType(date_Package, eng);
    eng->PutImport(date_String, date_Package);
    ctx->Push(date_Package);
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
    { 
        // Setup 'date' package as an import.
        String*  date_String = eng->AllocString("date");
        static RegisterFunction date_FuncDef = { "date", date_lib_load, 0, 0, 0 };    
        Value date_fn(Function::Create(eng, &date_FuncDef, World_Package));
        eng->PutImport(date_String, date_fn);
    }
}

}// pika

