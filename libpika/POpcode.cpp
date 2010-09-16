/*
 *  POpcode.h
 *  See Copyright Notice in Pika.cpp
 */
#include "Pika.h"
#include "POpcode.h"

namespace pika {

// Grap the name part of the opcode declarations.
#ifdef DECL_OP
#undef DECL_OP
#endif
#define DECL_OP(XOP, XNAME, XLENGTH, XFORMAT, XDESCR) XNAME,

const char* OpcodeNames[OPCODE_MAX] =
    {
#   include "POpcodeDef.inl"
        "", "", "",
    };

// Grap the length part of the opcode declarations.
#ifdef DECL_OP
#undef DECL_OP
#endif
#define DECL_OP(XOP, XNAME, XLENGTH, XFORMAT, XDESCR) XLENGTH,

const int OpcodeLengths[OPCODE_MAX] =
{
#   include "POpcodeDef.inl"
    0,
    0,
    0,
};

// Grap the format part of the opcode declarations.
#ifdef DECL_OP
#undef DECL_OP
#endif
#define DECL_OP(XOP, XNAME, XLENGTH, XFORMAT, XDESCR) XFORMAT,

const OpcodeFormat OpcodeFormats[OPCODE_MAX] =
{
#   include "POpcodeDef.inl"
    OF_zero,
    OF_zero,
    OF_zero,
};

/* Print the opcode given to stdout.
 */
void Pika_PrintInstruction(code_t bc)
{
    OpcodeLayout instr;
    instr.instruction = bc;
    
    // Only Print valid opcodes.
    if (instr.opcode < BREAK_LOOP)
    {
        OpcodeFormat fmt = OpcodeFormats[instr.opcode];
        const char* name = OpcodeNames[instr.opcode];        
        switch (fmt) {
        case OF_target:
        case OF_w:
            printf("%s %d\n", name, instr.w);
            return;            
        case OF_bw:
            printf("%s %d %d\n", name, instr.b, instr.w);
            return;
        case OF_bb:
            printf("%s %d %d\n", name, instr.b, instr.b2);
            return;
        case OF_bbb:
            printf("%s %d %d %d\n", name, instr.b, instr.b2, instr.b3);
            return;
        default:
            printf("%s\n", name);
        }
    }
}

}// pika
