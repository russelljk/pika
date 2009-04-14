/*
 *  PDebugger.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_DEBUGGER_HEADER
#define PIKA_DEBUGGER_HEADER

#include "PLineInfo.h"

namespace pika
{
class Function;
class Package;
class String;

/** A Debugger class that has hooks into the VM; allowing access to current script state. */
class PIKA_API Debugger : public Object
{
    PIKA_DECL(Debugger, Object)
public:
    Debugger(Engine*, Type*);

    virtual ~Debugger();

    virtual void MarkRefs(Collector*);

    void SetLineCB(Nullable<Function*> fn);
    void OnInstr(Context* ctx, code_t* pc, Function* fn);

    String* GetInstr();

    void   Start();
    void   Quit();
    pint_t GetPC();
    
    void     SetIgnore(Package*);
    Package* GetIgnore() { return ignorePkg; }
    
    struct PIKA_API LineDebugData
    {
        LineDebugData() : line(0), func(0), ctx(0), pc(0) {}
        
        void Reset();
        void DoMark(Collector*);
        
        operator bool() const { return func && pc && ctx; }
        
        u2          line;
        Function*   func;
        Context*    ctx;
        code_t*     pc;
    };

    Table         breakpoints;
    LineDebugData lineData;    
    IHook*        debugHook;        
    Function*     lineFunc;
    Function*     callFunc;
    Function*     raiseFunc;
    Function*     returnFunc;
    Package*      ignorePkg; // Ignore debug hooks if this is the current package. Useful if you want to have one package debug the others.
};

}

#endif
