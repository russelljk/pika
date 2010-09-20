/*
 *  PDebugger.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_DEBUGGER_HEADER
#define PIKA_DEBUGGER_HEADER

#include "PLineInfo.h"

namespace pika {
class Function;
class Package;
class String;
        
struct PIKA_API LineDebugData
{
    LineDebugData() : line(0), func(0), pc(0) {}
    
    void Reset();
    void DoMark(Collector*);
    
    operator bool() const { return func && pc; }
    
    u2          line;
    Function*   func;
    code_t*     pc;
};

/* A Debugger class that has hooks into the VM. 
 *
 * The debugger interrupts the flow of a script by breaking
 * when an instruction is executed. 
 *
 * You can ignore all activity inside a package by setting the ignore-package. This
 * is useful if you do not want to break on debugger calls or imported packages.
 * 
 * TODO {
 *       Currently the debugger only breaks on instructions, and only calls when
 *       a new line is encountered. 
 *
 *       The following changes need to be made before the 1.0 release:
 *       
 *       1. Allow programmer to specify break points corresponding to (package,line) or (function, line) pairs.
 *
 *       2. Allow programmer to "step into", which would break on the next "instruction" instead of the next "line".
 *
 *       3. Add support for all hook events (call, native-call, import, return, yield).
 *          a. This means the Hook api will need to be expanded.
 *          b. import is unique in that its the only hook used by default. We 
 *             might be better providing a importer class that would allow the
 *             user to extend the import system.
 *
 *       4. Debugger::Start should take a bitfield of options
 *          ie dbg.start( dbg.INSTRUCTION | dbg.CALL | dbg.RETURN )
 *
 *       Finally: Testing, Testing, Testing!!!
 * }
 *
 */
class PIKA_API Debugger : public Object
{
    PIKA_DECL(Debugger, Object)
public:
    Debugger(Engine*, Type*);

    virtual ~Debugger();

    virtual void MarkRefs(Collector*);

    void SetCallback(pint_t he, Nullable<Function*> fn);
        
    void OnInstr(Context* ctx, code_t* pc, Function* fn);

    String* GetInstruction();
    
    void   Start();
    void   Quit();
    pint_t GetPC();
    
    void     SetIgnore(Package*);
    Package* GetIgnore() { return ignorePkg; }
    
    static void Constructor(Engine* eng, Type* obj_type, Value& res);
    static void StaticInitType(Engine* eng);  
    
    LineDebugData lineData;
    hook_t        debugHook;
    hook_t        hooks[HE_max];
    Function*     callbacks[HE_max];
    Package*      ignorePkg; // Ignore debug hooks if this is the current package. Useful if you want to have one package debug the others.
};

}// pika

#endif
