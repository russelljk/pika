/*
 *  POpcode.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_OPCODE_HEADER
#define PIKA_OPCODE_HEADER

namespace pika {
// grep @OPCODE-LENGTH@ to find places where the opcode length is assumed.

// offset of the comparison operator of a for loop

#define PIKA_FORTO_COMP_OFFSET 2

// OpcodeFormat ////////////////////////////////////////////////////////////////////////////////////

enum OpcodeFormat
{
    OF_none,   // no operands
    OF_target, // target
    OF_w,      // word
    OF_bw,     // byte word
    OF_bb,     // byte byte
    OF_bbb,    // byte byte byte
    OF_zero,   // not an opcode
};

#ifdef DECL_OP
#undef DECL_OP
#endif
#define DECL_OP(XOP, XNAME, XLENGTH, XFORMAT, XDESCR) XOP,

// Opcode //////////////////////////////////////////////////////////////////////////////////////////

#define PIKA_NUM_SPECIALIZED_OPCODES 5 // pushliteral, pushlocal, setlocal have specialized forms

enum Opcode
{
#include "POpcodeDef.inl"
    BREAK_LOOP,
    CONTINUE_LOOP,
    JMP_TARGET,
    OPCODE_MAX,
};

#define OP_pushfinally OP_pushtry

extern const char*          OpcodeNames[OPCODE_MAX];
extern const int            OpcodeLengths[OPCODE_MAX];
extern const OpcodeFormat   OpcodeFormats[OPCODE_MAX];

// OpOverride //////////////////////////////////////////////////////////////////////////////////////

enum OpOverride
{
    OVR_first = 0,
    OVR_add = OVR_first,
    OVR_sub,
    OVR_mul,
    OVR_div,
    OVR_idiv,
    OVR_mod,
    OVR_pow,
    OVR_eq,
    OVR_ne,
    OVR_lt,
    OVR_gt,
    OVR_lte,
    OVR_gte,
    OVR_bitand,
    OVR_bitor,
    OVR_bitxor,
    OVR_unused1,
    OVR_lsh,
    OVR_rsh,
    OVR_ursh,
    OVR_not,
    OVR_bitnot,
    OVR_inc,
    OVR_dec,
    OVR_pos,
    OVR_neg,
    OVR_catsp,
    OVR_cat,    
    OVR_bind,
    OVR_unpack,
    OVR_unused2,
    OVR_call,
    OVR_get,
    OVR_set,
    OVR_getat,
    OVR_setat,
    OVR_init,
    OVR_next,
    OVR_iterate,
    /* right hand side overrides */
    OVR_add_r,
    OVR_sub_r,
    OVR_mul_r,
    OVR_div_r,
    OVR_idiv_r,
    OVR_mod_r,
    OVR_pow_r,
    OVR_unused3,
    OVR_unused4,
    OVR_lt_r,
    OVR_gt_r,
    OVR_lte_r,
    OVR_gte_r,
    OVR_bitand_r,
    OVR_bitor_r,
    OVR_bitxor_r,
    OVR_lsh_r,
    OVR_rsh_r,
    OVR_ursh_r,
    OVR_catsp_r,
    OVR_cat_r,    
    OVR_bind_r,
    OVR_comp,
    OVR_max,
    OVR_last = OVR_max,
};

static const size_t NUM_OVERRIDES = OVR_max;

// OpcodeLength ////////////////////////////////////////////////////////////////////////////////////

// OPCODE-CHANGE
INLINE u2 OpcodeLength(Opcode oc)
{
    ASSERT(oc < OPCODE_MAX);
    const OpcodeFormat fmt = OpcodeFormats[oc];
    switch (fmt)
    {
    case OF_none:
    case OF_target:
    case OF_w:
    case OF_bw:
    case OF_bb:    
    case OF_bbb:
        return 1;
    case OF_zero:
    default:
        return 0;
    }
    return 0;
}

// OpcodeLayout ////////////////////////////////////////////////////////////////////////////////////

union OpcodeLayout 
{
    code_t instruction;
    struct
    {
#if defined(PIKA_BIG_ENDIAN)
        union {
        struct {
            u1 b3;
            u1 b2;
        };
        u2 w;
        };
        u1 b;
        u1 opcode;
#else
        u1 opcode;
        u1 b;
        union {
        struct {
            u1 b2;
            u1 b3;
        };
        u2 w;
        };
#endif
    };
};

extern PIKA_API void Pika_PrintInstruction(code_t bc);

#define CS_MAKEOPERAND(a, b)    ((u2)(((u1)((u4)(a) & 0xff)) | ((u2)((u1)((u4)(b) & 0xff))) << 8))
#define CS_LOBYTE(w)            ((u1)((u2)(w) & 0x00ff))
#define CS_HIBYTE(w)            ((u1)((u2)(w) >> 8))

// [??][??][??][op]
#define PIKA_MAKE_B(op) \
    (((code_t)(op)) & 0xFF)

// [   w  ][??][op]
#define PIKA_MAKE_W(op, w) \
    (PIKA_MAKE_B(op) | (((((code_t)(w)) & 0xFFFF)) << 16))

// [   w  ][b1][op]
#define PIKA_MAKE_BW(op, b, w) \
    (PIKA_MAKE_W(op, w) | (PIKA_MAKE_B(b) << 8))

// [??][b2][b1][op]
#define PIKA_MAKE_BB(op, b1, b2) \
    (PIKA_MAKE_BW(op, b1, CS_MAKEOPERAND(b2, 0x00)))
    
// [b3][b2][b1][op]
#define PIKA_MAKE_BBB(op, b1, b2, b3) \
    (PIKA_MAKE_BW(op, b1, CS_MAKEOPERAND(b2, b3)))
    
#define PIKA_GET_OPCODEOF(x) ((Opcode)((u1)x & 0xFF))
#define PIKA_GET_SHORTOF(x)  ((x >> 16)  & 0xFFFF)
#define PIKA_GET_BYTEOF(x)   ((x >> 8)   & 0xFF)
#define PIKA_GET_BYTE2OF(x)  ((x >> 16)  & 0xFF)
#define PIKA_GET_BYTE3OF(x)  ((x >> 24)  & 0xFF)

}// pika

#endif

