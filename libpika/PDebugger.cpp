/*
 *  PDebugger.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"

namespace pika {

void LineDebugData::DoMark(Collector* c)
{
    if (func) func->Mark(c);

}

void LineDebugData::Reset()
{
    pc   = 0;
    line = 0;
    func = 0;

}

PIKA_IMPL(Debugger)

Debugger::Debugger(Engine* eng, Type* obj_type)
    : ThisSuper(eng, obj_type),
    debugHook(0),
    ignorePkg(0)
{
    Pika_memzero(hooks, sizeof(hook_t) * HE_max);
    Pika_memzero(callbacks, sizeof(Function*) * HE_max);    
}

Debugger::~Debugger() {}

void Debugger::MarkRefs(Collector* c)
{
    ThisSuper::MarkRefs(c);
    
    for (size_t i = 0; i < HE_max; ++i)
    {
        if (callbacks[i])
            callbacks[i]->Mark(c);
    }
    
    if (ignorePkg) ignorePkg->Mark(c);
    
    lineData.DoMark(c);
}

void Debugger::SetIgnore(Package* pkg) { ignorePkg = pkg; }

void Debugger::OnInstr(Context* ctx, code_t* xpc, Function* fn)
{
    Function* lineFunc = callbacks[HE_instruction];
    if (fn == lineFunc || !lineFunc)
        return;
    if (ignorePkg && fn->IsLocatedIn(ignorePkg))
        return;
    
    Def* def = fn->GetDef();
    code_t* pc = (xpc - 1); // @OPCODE-LENGTH@
    
    for (size_t i = 0; i < def->lineInfo.GetSize(); ++i)
    {
        code_t* currpos = def->lineInfo[i].pos;
        code_t* nextpos = (i == def->lineInfo.GetSize() - 1) ? currpos : def->lineInfo[i + 1].pos;
        
        if ((pc == currpos) || (pc >= currpos && (pc < nextpos || i == def->lineInfo.GetSize() - 1)))
        {
            if (def->lineInfo[i].line != lineData.line)
            {
                lineData.line = def->lineInfo[i].line;
                lineData.func = fn;
                lineData.pc = pc;
                
                ctx->CheckStackSpace(4);
                ctx->Push(fn);                    // arg 1
                ctx->Push((pint_t)lineData.line); // arg 2
                ctx->PushNull();                  // self
                ctx->Push(lineFunc);              // function
                
                if (ctx->SetupCall(2))
                {
                    ctx->Run();
                }
                ctx->PopTop(); // TODO: Should we pop off the return value?
                break;
            }
        }
    }
}

String* Debugger::GetInstruction()
{
    if (lineData)
    {
        code_t* beg = lineData.func->GetBytecode();
        code_t* end = lineData.func->GetDef()->bytecode->length + beg;
        
        if (lineData.pc >= beg && lineData.pc <= end) // valid pc
        {
            OpcodeLayout instr;
            instr.instruction = *lineData.pc;
            if (instr.opcode < BREAK_LOOP) // valid opcode
            {
                OpcodeFormat fmt = OpcodeFormats[instr.opcode];
                const char* opcodename = OpcodeNames[instr.opcode];
                // OPCODE-CHANGE
                switch (fmt)
                {
                    case OF_target:
                    case OF_w:  return engine->AllocStringFmt("%s 0x%hx",                  opcodename, instr.w);
                    case OF_bw: return engine->AllocStringFmt("%s 0x%hhx 0x%hx",           opcodename, instr.b, instr.w);
                    case OF_bb: return engine->AllocStringFmt("%s 0x%hhx 0x%hhx\n",        opcodename, instr.b, instr.b2);
                    case OF_bbb:return engine->AllocStringFmt("%s 0x%hhx 0x%hhx 0x%hhx\n", opcodename, instr.b, instr.b2, instr.b3);               
                    default: break;
                }
                return engine->AllocString(opcodename);
            }
        }
    }
    return engine->emptyString;
}

void Debugger::SetCallback(pint_t he, Nullable<Function*> fn)
{
    if (he < 0 || he >= HE_max)
        RaiseException("attempt to set invalid debugger callback.");
    
    callbacks[he] = fn;
    if (he == HE_instruction && !fn)
    {
        lineData.Reset();
    }
}

struct DebugHook : IHook
{
    DebugHook(Debugger* dbg) : dbg(dbg) { }
    
    virtual bool OnEvent(HookEvent, void* v)
    {
        InstructionData* data = (InstructionData*)v;
        dbg->OnInstr(data->context, data->pc, data->function);
        return false;
    }
    
    virtual void Release(HookEvent)
    { 
        Pika_delete(this);
    }
    
    Debugger* dbg;
};

void Debugger::Start()
{
    if (!debugHook)
    {
        PIKA_NEW(DebugHook, debugHook, (this));
        engine->AddHook(HE_instruction, debugHook);
    }
}

void Debugger::Quit()
{
    if (debugHook)
    {
        engine->RemoveHook(HE_instruction, debugHook);
        debugHook = 0;
    }
}

pint_t Debugger::GetPC()
{
    if (!lineData)
    {
        RaiseException("Attempt to call GetPC from invalid debugger");
        return 0;
    }
    
    code_t* start = lineData.func->GetBytecode();
    code_t* end   = start + lineData.func->def->bytecode->length;
    
    if (lineData.pc >= end || lineData.pc < start)
    {
        RaiseException("invalid program counter.");
        return 0;
    }
    
    ptrdiff_t pdiff = lineData.pc - start;
    return (pint_t)pdiff;
}

void Debugger::Constructor(Engine* eng, Type* type, Value& res)
{
    Debugger* dbg=0;
    GCNEW(eng, Debugger, dbg, (eng, type));
    res.Set(dbg);
}

void Debugger::StaticInitType(Engine* eng)
{
    Package*  world = eng->GetWorld();
    String*   Debugger_str = eng->AllocString("Debugger");
    String*   dbg_str = eng->AllocString("dbg");
    Debugger* dbg = 0;
    Type*     dbg_type = Type::Create(eng, Debugger_str, eng->Object_Type, Debugger::Constructor, world);
    
    dbg_type->SetAbstract(true);
    dbg_type->SetFinal(true);
    
    GCNEW(eng, Debugger, dbg, (eng, dbg_type));
    
    NamedConstant Hook_Constants[] =    {
        { "CALL",        HE_call        },
        { "RETURN",      HE_return      },
        { "YIELD",       HE_yield       },
        { "NATIVECALL",  HE_nativeCall  },
        { "INSTRUCTION", HE_instruction },
        { "EXCEPT",      HE_except      },
        { "IMPORT",      HE_import      },      
    };
    
    SlotBinder<Debugger>(eng, dbg_type)
    .Method(&Debugger::SetCallback, "setCallback")
    .PropertyR("instruction", &Debugger::GetInstruction,  "getInstruction")
    .Method(&Debugger::GetPC,       "getPC")
    .Method(&Debugger::Quit,        "quit")
    .Method(&Debugger::Start,       "start")
    .PropertyRW("ignore",
            &Debugger::GetIgnore,   "getIgnore",
            &Debugger::SetIgnore,   "setIgnore");
    
    Basic::EnterConstants(dbg_type, Hook_Constants, countof(Hook_Constants));
    
    eng->SetDebugger(dbg);
    
    world->SetSlot(dbg_str, dbg);
}

}// pika
