/*
 * PDef.h
 * See Copyright Notice in Pika.h
 */
#ifndef PIKA_DEF_HEADER
#define PIKA_DEF_HEADER

#include "PLineInfo.h"

#ifndef PIKA_BUFFER_HEADER
#   include "PBuffer.h"
#endif

#ifndef PIKA_COLLECTOR_HEADER
#   include "PCollector.h"
#endif

namespace pika
{
#if defined(PIKA_DLL) && defined(_WIN32)
template class PIKA_API Buffer<LineInfo>;
template class PIKA_API Buffer<LocalVarInfo>;
#endif

class Context;
class Function;
class LiteralPool;
/*
    Native function.
    ctx:
        Context the function is called from.
    self:
        Self object

    return:
        number of return values (0 - PIKA_MAX_RETC).
*/
typedef int (*Nativecode_t)(Context* ctx, Value& self);

struct RegisterFunction
{
    const char*  name;
    Nativecode_t code;
    u2           argc;
    bool         varargs;
    bool         strict;
};

struct RegisterProperty
{
    const char*     name;
    Nativecode_t    getter;
    const char*     getterName;
    Nativecode_t    setter;
    const char*     setterName;
};

class Bytecode
{
public:
    Bytecode(code_t* code, u2 length);
    ~Bytecode();
    
    code_t* code;
    u2      length;
};

// Def /////////////////////////////////////////////////////////////////////////////////////

class PIKA_API Def : public GCObject
{
    Def() : name(0),
            source(0),
            parent(0),
            bytecode(0),
            nativecode(0),
            numArgs(0),
            numLocals(0),
            stackLimit(0),
            bytecodePos(0),
            literals(0),
            mustClose(false),
            isVarArg(false),
            isStrict(false),
            line(-1)
    {}
    
    /** Constructor that creates a native function Def. */
    Def(String*      declname,
        Nativecode_t fn,
        u2           argc,
        bool         varargs,
        bool         strict,
        Def*         parent_def) :
            name(declname),
            source(0),
            parent(parent_def),
            bytecode(0),
            nativecode(fn),
            numArgs(argc),
            numLocals(argc),
            stackLimit(0),
            bytecodePos(0),
            literals(0),
            mustClose(false),
            isVarArg(varargs),
            isStrict(strict),
            line(-1)
    {}
public:
    virtual ~Def();
    
    virtual void MarkRefs(Collector* c);
    
    static Def* CreateWith(Engine* eng, String* name, Nativecode_t fn, u2 argc, bool varargs, bool strict, Def* parent);
    static Def* Create(Engine* eng);
    
    void            SetBytecode(code_t* bc, u2 len);
    INLINE code_t*  GetBytecode() const { return(bytecode) ? bytecode->code : 0; }

    //TODO: AddLocalVar and SetLocalRange assume that the CompileState and this Def assign the var the same offset.
    void AddLocalVar(Engine*, const char*);
    void SetLocalRange(size_t local, size_t start, size_t end);
    void SetSource(Engine* eng, const char* buff, size_t len);
    
    // return < 0 on failure
    int OffsetOfLocal(const String* str) const;
    
    //TODO: Should a name actually matter or does it belong at closure creating time.
    
    String*          name;        //!< Declared name.
    String*          source;      //!< Source code.
    Def*             parent;      //!< Parent function.
    Bytecode*        bytecode;    //!< Bytecode pointer.
    Nativecode_t     nativecode;  //!< Native function pointer.
    u2               numArgs;     //!< Number of parameters, including the 'rest' parameter.
    u2               numLocals;   //!< Number of lexEnv and parameters.
    u2               stackLimit;  //!< Stack space needed for execution.
    Buffer<LineInfo> lineInfo;    //!< Mapping of source code line numbers to bytecode position.
    /**
     *  Ranges for all local variables (including arguments) in this def. It should be noted that this runs
     *  from the local declaration to the end of its block, no further calculations are performed.
     */
    Buffer<LocalVarInfo> localsInfo;
    
    ptrdiff_t    bytecodePos; //!< Point defined in parent's bytecode.
    LiteralPool* literals;    //!< Literals used in the bytecode.
    bool         mustClose;   //!< True if this function's locals are accessed by a child function.
    bool         isVarArg;    //!< Bytecode function that has the rest parameter or Native function that takes a variable number of arguments.
    bool         isStrict;    //!< Native function must has the correct number of arguments.
    int          line;        //!< Line in the script this def is declared or -1 for native defs.
};

}// pika

#endif
