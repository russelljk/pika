/*
 *  PInstruction.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_INSTRUCTION_HEADER
#define PIKA_INSTRUCTION_HEADER

#include "POpcode.h"

namespace pika
{

struct TreeNode;

struct Instr
{
    Instr(Opcode code)
            : opcode(code),
            operandu1(0),
            operand(0),
            pos(0),
            line(0),
            next(0),
            prev(0),
            visited(false),
            label(false),
            target(0) {}
            
    ~Instr() { Pika_delete(next); }
    
    // patches break and continue jump targets which are optional matched by the symbol
    // given.
    
    void DoLoopPatch(Instr* breakTarget, Instr* continueTarget, struct Symbol* sym)
    {
        Instr* n = this;
        do
        {
            if (n->opcode == BREAK_LOOP)
            {
                if ((n->symbol && (sym == n->symbol)) || !n->symbol)
                {
                    n->opcode = OP_jump;
                    n->target = breakTarget;
                }
            }
            else if (n->opcode == CONTINUE_LOOP)
            {
                if ((n->symbol && (sym == n->symbol)) || !n->symbol)
                {
                    n->opcode = OP_jump;
                    n->target = continueTarget;
                }
            }
        }
        while ( (n = n->next) );
    }
    
    // Attaches the given Instr to end of this Instr chain.
    
    Instr* Attach(Instr* nxt)
    {
        if (!next)
        {
            next = nxt;
            nxt->prev = this;
        }
        else
        {
            Instr* curr = next;
            while (curr->next)
                curr = curr->next;
            curr->next = nxt;
            nxt->prev = curr;
        }
        return nxt;
    }
    
    void Unattach()
    {
        if (prev) prev->next = next;
        if (next) next->prev = prev;
        
        prev = next = 0;
    }
    
    // Calculates the byte-code position of this Instr chain.
    
    void CalcPos()
    {
        pos = 0;
        Instr* curr = next;
        Instr* nxt = 0;
        u2 currpos = pos + OpcodeLength(opcode);
        
        while (curr)
        {
            nxt       = curr->next;
            curr->pos = currpos;
            currpos  += OpcodeLength(curr->opcode);
            curr      = nxt;
        }
    }
    
    // Sets a jump target, making the given Instr a label.
    
    void SetTarget(Instr* t)
    {
        target = t;
        
        if (target)
        {
            target->label = true;
        }
    }
    
    static Instr* Create(Opcode oc)
    {
        Instr* ir = 0;
        PIKA_NEW(Instr, ir, (oc));
        return ir;
    }
    
    Opcode  opcode;
    u1      operandu1;
    u2      operand;
    u2      pos;
    u2      line;
    Instr*  next;
    Instr*  prev;
    bool    visited;
    bool    label;
    union
    {
        Instr*  target;
        Symbol* symbol;
    };
};

INLINE int OpcodeStackChange(Instr *ir)
{
    switch (ir->opcode)
    {
    case OP_nop:            return  0;
    case OP_dup:            return  1;
    case OP_swap:           return  0;
    case OP_jump:           return  0;
    case OP_jumpiffalse:
    case OP_jumpiftrue:     return -1;
    case OP_pushnull:
    case OP_pushself:
    case OP_pushtrue:
    case OP_pushfalse:
    case OP_pushliteral0:
    case OP_pushliteral1:
    case OP_pushliteral2:
    case OP_pushliteral3:
    case OP_pushliteral4:
    case OP_pushliteral:
    case OP_pushlocal0:
    case OP_pushlocal1:
    case OP_pushlocal2:
    case OP_pushlocal3:
    case OP_pushlocal4:
    case OP_pushlocal:
    case OP_pushglobal:
    case OP_pushmember:
    case OP_pushlexical:      return  1;
    case OP_setlocal0:
    case OP_setlocal1:
    case OP_setlocal2:
    case OP_setlocal3:
    case OP_setlocal4:
    case OP_setlocal:
    case OP_setglobal:
    case OP_setmember:
    case OP_setouter:       return -1;
    case OP_pop:            return -1;
    case OP_acc:            return -1;
    
    case OP_add:
    case OP_sub:
    case OP_mul:
    case OP_div:
    case OP_idiv:
    case OP_mod:
    case OP_eq:
    case OP_ne:
    case OP_lt:
    case OP_gt:
    case OP_lte:
    case OP_gte:
    case OP_bitand:
    case OP_bitor:
    case OP_bitxor:
    case OP_xor:
    case OP_lsh:
    case OP_rsh:
    case OP_ursh:           return -1;
    case OP_not:
    case OP_bitnot:
    case OP_inc:
    case OP_dec:
    case OP_pos:
    case OP_neg:            return  0;
    case OP_catsp:          return -1;
    case OP_cat:            return -1;
    case OP_pow:            return -1;
    case OP_bind:           return -1;
    
    case OP_unpack:
    {
        int operand = ir->operand;
        --operand;
        return operand;
    }
    case OP_new:
    case OP_tailcall:
    {
        int operand = ir->operand;
        operand     = -operand - 1; // ????: tailcall will be whatever the previous call was!
        return operand;
    }
    case OP_call:
    {
        int retv    = ir->operandu1 ? ir->operandu1 : 1;
        int operand = ir->operand;
        operand     = -operand - retv;
        return operand;
    }
    case OP_dotget:         return -1;
    case OP_dotset:         return -3;
    case OP_delete:         return -2;
    
    case OP_locals:         return  1;
    
    case OP_method:
    {
        int operand = ir->operand;
        operand = -operand - 1;
        return  operand;
    }
    
    case OP_property:       return -2;

    case OP_subclass:       return -1;
    
    case OP_array:
    {
        int operand = ir->operand;
        operand = -operand + 1;
        return  operand;
    }
    case OP_objectliteral:
    {
        int operand = ir->operand * 2;
        operand = -operand;
        return  operand;
    }
    case OP_super:          return  1;
    case OP_forto:          return -3;
    
    case OP_retacc:         return  1;
    case OP_ret:            return  0;
    case OP_retv:           return  0; // TODO:
    
    case OP_yieldnull:      return  0;
    case OP_yield:          return -1;
    case OP_yieldv:
    {
        int operand = ir->operand;
        return -operand;
    }
    case OP_foreach:        return -1;
    case OP_enumisvalid:    return  1;
    case OP_enumadvance:    return  1;
    
    case OP_same:           return -1;
    case OP_notsame:        return -1;
    case OP_is:             return -1;
    case OP_has:            return -1;
    
    case OP_pushtry:
    case OP_pophandler:    
    case OP_raise:    
    case OP_retensure:
    case OP_callensure:     return  0;
        
    case OP_pushwith:       return -1;
    case OP_popwith:        return  0;
    
    case OP_newenv:         return -1;
    case OP_pushpkg:        return -1;
    case OP_poppkg:         return  0;

    
    case BREAK_LOOP:        return  0;
    case CONTINUE_LOOP:     return  0;
    case JMP_TARGET:        return  0;
    default:                return 0;
    };
    return 0;
};

}// pika

#endif
