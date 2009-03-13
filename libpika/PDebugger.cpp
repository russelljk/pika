/*
 *  PDebugger.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"


namespace pika
{

PIKA_IMPL(Debugger)

Debugger::Debugger(Engine* eng, Type* obj_type)
        : ThisSuper(eng, obj_type),
        debugHook(0),
        lineFunc(0),
        callFunc(0),
        raiseFunc(0),
        returnFunc(0),
              ignorePkg(0)
      {}
      
      Debugger::~Debugger() {}
      
      void Debugger::MarkRefs(Collector* c)
      {
          ThisSuper::MarkRefs(c);
          
          breakpoints.DoMark(c);
          
          if (lineFunc) lineFunc->Mark(c);
          if (callFunc) callFunc->Mark(c);
          if (raiseFunc) raiseFunc->Mark(c);
          if (returnFunc) returnFunc->Mark(c);
          if (ignorePkg) ignorePkg->Mark(c);
          
          lineData.DoMark(c);
      }
      
      void Debugger::SetIgnore(Package* pkg) { ignorePkg = pkg; }
      
      void Debugger::OnInstruction(Context* ctx, code_t* xpc, Function* fn)
      {
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
              
              if ((pc == currpos) || (pc >= currpos && pc < nextpos) || (pc >= currpos && (i == def->lineInfo.GetSize() - 1)))
                  // ((pc == currpos) || ((pc >= currpos) && ((pc < nextpos) || (i == def->lineInfo.GetSize() - 1)))) TODO: TEST Simplified line <<=
              {
                  if (def->lineInfo[i].line != lineData.line)
                  {
                      lineData.line = def->lineInfo[i].line;
                      lineData.func = fn;
                      lineData.pc = pc;
                      
                      ctx->CheckStackSpace(4);
                      ctx->Push(fn);                   // arg 1
                      ctx->Push((pint_t)lineData.line); // arg 2
                      ctx->PushNull();                 // self
                      ctx->Push(lineFunc);             // function
                      
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
                      case OF_w:  return engine->AllocStringFmt("%s 0x%hx",     opcodename, instr.w);
                      case OF_bw: return engine->AllocStringFmt("%s 0x%x 0x%x", opcodename, instr.b, instr.w);
                      }
                      return engine->AllocString(opcodename);
                  }
              }
          }
          return engine->emptyString;
      }
      
      void Debugger::SetLineCallback(Nullable<Function*> fn)
      {
          lineFunc = fn;
          
          if (!lineFunc)
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
              dbg->OnInstruction(data->context, data->pc, data->function);
              return false;
          }
          
          virtual void Release(HookEvent) { Pika_delete(this); }
          
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

void Debugger::LineDebugData::DoMark(Collector* c)
{
    if (func) func->Mark(c);
    if (ctx)  ctx->Mark(c);
}

void Debugger::LineDebugData::Reset()
{
    pc   = 0;
    line = 0;
    func = 0;
    ctx  = 0;
}

}// pika

static void Debugger_newFn(Engine* eng, Type* type, Value& res)
{
    Debugger* dbg=0;
    GCNEW(eng, Debugger, dbg, (eng, type));
    res.Set(dbg);
}

void InitDebuggerAPI(Engine* eng)
{
    Package*  world    = eng->GetWorld();
    String*   dbg_str  = eng->AllocString("Debugger");
    Debugger* dbg      = 0;
    Type*     dbg_type = Type::Create(eng, dbg_str, eng->Object_Type, Debugger_newFn, world);
    
    dbg_type->SetAbstract(true);
    dbg_type->SetFinal(true);
    
    GCNEW(eng, Debugger, dbg, (eng, dbg_type));
    
    SlotBinder<Debugger>(eng, dbg_type)
    .Method(&Debugger::SetLineCallback, "setLineCallback")
    .Method(&Debugger::GetInstruction,  "getInstruction")
    .Method(&Debugger::GetPC,           "getPc")
    .Method(&Debugger::Quit,            "quit")
    .Method(&Debugger::Start,           "start")
    .PropertyRW("ignore", &Debugger::GetIgnore, "getIgnore",
                &Debugger::SetIgnore, "setIgnore");
                
    eng->SetDebugger(dbg);
    
    world->SetSlot(dbg_str, dbg);
}
