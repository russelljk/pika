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

#ifndef PIKA_TABEL_HEADER
#   include "PTable.h"
#endif

namespace pika {
#if defined(PIKA_DLL) && defined(_WIN32)
template class PIKA_API Buffer<LineInfo>;
template class PIKA_API Buffer<LocalVarInfo>;
#endif

class Context;
class Function;
class LiteralPool;

/** The proto-type for a native function callable by Pika.
  *
  * @param [in] The Context we are called from.
  * @param [in] The self object if a method.
  *
  * @result The number of arguments returned on the Context's stack. The Context
  *         will adjust the count to match whats expected.
  */
typedef int (*Nativecode_t)(Context* ctx, Value& self);

enum DefFlags {
    DEF_VAR_ARGS     = PIKA_BITFLAG(0), //!< Def takes a variable number of arguments.
    DEF_STRICT       = PIKA_BITFLAG(1), //!< Def takes a strict number of arguments.
    DEF_KEYWORD_ARGS = PIKA_BITFLAG(2)  //!< Create a keyword argument when called.
};

#define PIKA_DOC(X, DOC)\
    static const char* X##_Doc_String = DOC

#define PIKA_GET_DOC(X)\
    X##_Doc_String

struct RegisterFunction
{
    const char*  name;    //!< Name of the function.
    Nativecode_t code;    //!< Pointer to the function.
    u2           argc;    //!< Number of arguments expected.   
    u4           flags;   //!< Functions DefFlags, or'ed together.
    const char*  __doc;
};

struct RegisterProperty
{
    const char*  name;          //!< Name of the property.
    Nativecode_t getter;        //!< Getter function.
    const char*  getterName;    //!< Getter name.
    Nativecode_t setter;        //!< Setter funcion.
    const char*  setterName;    //!< Setter name.
    bool         unattached;    //!< Should the function's be attached to the Object/Type (ClassMethod or InstanceMethod).
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
            isKeyword(false),
            isStrict(false),
            isGenerator(false),
            line(-1) {}
    
    Def(String* declname, Nativecode_t fn, u2 argc, 
        bool varargs, bool strict, bool kwas, Def* parent_def) : name(declname),
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
            isKeyword(kwas),
            isStrict(strict),
            isGenerator(false),
            line(-1) {}
public:
    virtual ~Def();
    
    virtual void MarkRefs(Collector* c);
    
    static Def* CreateWith(Engine* eng, String* name, Nativecode_t fn, u2 argc, 
                           u4 flags, Def* parent);
    static Def* Create(Engine* eng);
    
    void           SetBytecode(code_t* bc, u2 len);
    INLINE code_t* GetBytecode() const { return(bytecode) ? bytecode->code : 0; }
    
    //TODO: AddLocalVar and SetLocalRange assume that the CompileState and Def assign the var the same offset.
    //      The offset of the var should be tracked by only one class.
    
    void AddLocalVar(Engine*, const char*, ELocalVarType lvt = LVT_variable);
    void SetLocalRange(size_t local, size_t start, size_t end);
    void SetSource(Engine* eng, const char* buff, size_t len);
    void SetGenerator();
    String*          name;        //!< Declared name.
    String*          source;      //!< Source code.
    Def*             parent;      //!< Parent function.
    Bytecode*        bytecode;    //!< Bytecode pointer.
    Nativecode_t     nativecode;  //!< Native function pointer.
    u2               numArgs;     //!< Number of parameters, including the 'rest' parameter.
    u2               numLocals;   //!< Number of lexEnv and parameters.
    u2               stackLimit;  //!< Stack space needed for execution.
    Buffer<LineInfo> lineInfo;    //!< Mapping of source code line numbers to bytecode position.    
    
    /** Ranges for all local variables (including arguments) in this function. It should be noted that this runs
      * from the local declaration to the end of the block it is declared in. Parameters will exist for the entire 
      * function's life time. No further calculations are performed.
      */
    Buffer<LocalVarInfo> localsInfo;    
    
    Table        kwargs;      //!< Table of (string, int) pairs.
    ptrdiff_t    bytecodePos; //!< Point defined in parent's bytecode.
    LiteralPool* literals;    //!< Literals used in the bytecode.
    bool         mustClose;   //!< True if this function's locals are accessed by a child function.
    bool         isVarArg;    //!< Bytecode function that has the rest parameter or Native function that takes a variable number of arguments.
    bool         isKeyword;   //!< Function has keyword argument parameter.
    bool         isStrict;    //!< Function must has the correct number of arguments.
    bool         isGenerator; //!< Function has yield statement.
    int          line;        //!< Line in the script this def is declared or -1 for native defs.
};

}// pika

#endif
