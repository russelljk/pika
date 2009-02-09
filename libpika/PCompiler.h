/*
 *  PCompiler.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_COMPILER_HEADER
#define PIKA_COMPILER_HEADER

#include "PBuffer.h"
#include "PInstruction.h"

namespace pika
{
struct CompileState;

/** Compiles the intermediate representation (aka the Instr class) into bytecode.
  */
class Compiler
{
public:
    Compiler(CompileState*, 
             Instr*, 
	         Def*,
             LiteralPool*);

    ~Compiler();

    void DoCompile();
	
    code_t* GetBytecode()       { return (bytecode.GetSize()) ? &bytecode[0] : 0; }    
	u2      GetBytecodeLength() { return (u2)bytecode.GetSize(); }    
	int     GetStackLimit()     { return max_stack; }

private:
    void Emit();
    void AddWord(code_t w);

    int     max_stack;       //!< max space the operand stack needs.
    Def*    def;             //!< def that this code belongs to.
    Instr*  start;           //!< def that this code belongs to.
    CompileState*  state;    //!< Compile state we are compiling from.
    Buffer<code_t> bytecode; //!< Byte code result of the compilation.
};

void PrintBytecode(const u1* bc, size_t len);

}// pika

#endif
