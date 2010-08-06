/*
 *  POpcode.h
 *  See Copyright Notice in Pika.cpp
 */
#include "Pika.h"
#include "POpcode.h"

namespace pika {

// OpcodeNames /////////////////////////////////////////////////////////////////////////////////////

#ifdef DECL_OP
#undef DECL_OP
#endif
#define DECL_OP(XOP, XNAME, XLENGTH, XFORMAT, XDESCR) XNAME,

const char* OpcodeNames[OPCODE_MAX] =
    {
#   include "POpcodeDef.inl"
        "", "", "",
    };
    
// OpcodeLengths ///////////////////////////////////////////////////////////////////////////////////

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
    
// OpcodeFormats ///////////////////////////////////////////////////////////////////////////////////

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
    
// Pika_PrintInstruction ///////////////////////////////////////////////////////////////////////////
// OPCODE-CHANGE
void Pika_PrintInstruction(code_t bc)
{
    OpcodeLayout instr;
    instr.instruction = bc;
    
    if (instr.opcode < BREAK_LOOP)
    { // valid opcode
        OpcodeFormat fmt  = OpcodeFormats[instr.opcode];
        const char*   name = OpcodeNames[instr.opcode];
        
        switch (fmt)
        {
        case OF_target:
        case OF_w:
            printf("%s %d\n", name, instr.w);
            return;
            
        case OF_bw:
            printf("%s %d %d\n", name, instr.b, instr.w);
            return;
        default:
            printf("%s\n", name);
        }
    }
}

}// pika
