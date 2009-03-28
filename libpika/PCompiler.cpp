/*
 *  PCompiler.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PAst.h"
#include "PCompiler.h"
#include "PContext_Ops.inl"

#define BINARY_FOLD(op, fun)                                                                         \
case op:                                                                                             \
{                                                                                                    \
            if ((res1.tag == TAG_integer || res1.tag == TAG_real) &&                                 \
                (res2.tag == TAG_integer || res2.tag == TAG_real))                                   \
            {                                                                                        \
                                                                                                     \
                prevprev->opcode = OP_pushliteral;                                                   \
                prev->Unattach(); curr->Unattach();                                                  \
                                                                                                     \
                if (res1.tag == TAG_integer && res2.tag == TAG_integer)                              \
                {                                                                                    \
                    if (op == OP_div && (res1.val.integer % res2.val.integer != 0))                  \
                    {                                                                                \
                        preal_t const ra = (preal_t)res1.val.integer;                                    \
                        preal_t const rb = (preal_t)res2.val.integer;                                    \
                        res1.Set((preal_t)(ra/rb));                                                    \
                        prevprev->operand = lp->Add(res1.val.real);                                  \
                    }                                                                                \
                    else                                                                             \
                    {                                                                                \
                        fun(res1.val.integer, res2.val.integer);                                     \
                        prevprev->operand = lp->Add(res1.val.integer);                               \
                    }                                                                                \
                }                                                                                    \
                else                                                                                 \
                {                                                                                    \
                    if (res1.tag == TAG_integer)                                                     \
                        res1.Set((float)res1.val.integer);                                           \
                    if (res2.tag == TAG_integer)                                                     \
                        res2.Set((float)res2.val.integer);                                           \
                                                                                                     \
                    fun(res1.val.real, res2.val.real);                                               \
                    if (op == OP_idiv)                                                               \
                    {                                                                                \
                        prevprev->operand = lp->Add(RealToInteger(res1.val.real));                   \
                    }                                                                                \
                    else                                                                             \
                    {                                                                                \
                        prevprev->operand = lp->Add(res1.val.real);                                  \
                    }                                                                                \
                }                                                                                    \
                                                                                                     \
                                                                                                     \
                Pika_delete(prev);Pika_delete(curr);                                               \
                curr = prevprev;                                                                     \
                                                                                                     \
            }                                                                                        \
    }break;


#define CONTINUE_FOLD_LOOP()                                                                \
    curr = curr->next; continue;

// Folds constant arithmetic|bit operations.
//
// We don't want to do any non-trivial conversions or report Type-Errors on operations.
// Basically do the most we can do without changing the result of the program.
namespace pika
{

void DoConstantFold(Instr* ir, LiteralPool* lp)
{
    Instr* curr = ir;

    ir->label = true; /* not really needed. */

    while (curr)
    {
        bool prevIsConst = false;
        bool prevprevIsConst = false;
        Instr* prev = curr->prev;
        Instr* prevprev = 0;

        if (prev)
        {
            prevIsConst = (prev->opcode == OP_pushliteral);
            prevprev = prev->prev;

            if (prevprev)
            {
                prevprevIsConst = (prevprev->opcode == OP_pushliteral);
            }
        }

        // A conditional jump.
        // If the condition is always met (ie is a literal), we turn it into a unconditional jump.

        if (((curr->opcode == OP_jumpiftrue) || (curr->opcode == OP_jumpiffalse))
            && (prevIsConst || (prev && (prev->opcode == OP_pushtrue || prev->opcode == OP_pushfalse || prev->opcode == OP_pushnull)))
            && !curr->label)
        {
            bool jmp = (curr->opcode == OP_jumpiftrue);

            switch (prev->opcode)
            {
            case OP_pushnull:  jmp = !jmp; break;
            case OP_pushtrue:  jmp =  jmp; break;
            case OP_pushfalse: jmp = !jmp; break;

            case OP_pushliteral:
            {
                u2 index = prev->operand;
                Value res = lp->Get(index);

                if (res.tag == TAG_integer)
                {
                    jmp = (res.val.integer != 0) == jmp;
                    break;
                }
                else if (res.tag == TAG_real)
                {
                    jmp = (Pika_RealToBoolean(res.val.real) == jmp);
                    break;
                }
            }

            //  Fall Through
            //  |
            //  V

            default:
                CONTINUE_FOLD_LOOP();
            }

            prev->operand = prev->operandu1 = 0;
            prev->target  = 0;

            if (jmp)
            {
                prev->opcode = OP_jump;
                prev->target = curr->target;
            }
            else
            {
                prev->opcode = OP_nop;
            }

            curr->Unattach();
            Pika_delete(curr);
            curr = prev;
        }
        else if (curr->opcode == OP_neg && prevIsConst && !curr->label)
        {
            u2 index = prev->operand;
            Value res = lp->Get(index);

            if (res.tag == TAG_integer)
            {
                prev->operand = lp->Add(-res.val.integer);
                curr->Unattach();
                Pika_delete(curr);
                curr = prev;
            }
            else if (res.tag == TAG_real)
            {
                prev->operand = lp->Add(-res.val.real);
                curr->Unattach();
                Pika_delete(curr);
                curr = prev;
            }
        }
        else if ((curr->opcode >= OP_add && curr->opcode <= OP_mod) &&
                 prevIsConst     &&
                 prevprevIsConst &&
                 !curr->label    &&
                 !prev->label)
        {
            u2 idx2 = prev->operand;
            u2 idx1 = prevprev->operand;
            Value res1 = lp->Get(idx1);
            Value res2 = lp->Get(idx2);


            if (curr->opcode == OP_div || curr->opcode == OP_idiv || curr->opcode == OP_mod)
            {
                if (res2.tag == TAG_integer && res2.val.integer == 0)
                {
                    /* division/modulo by zero is a RuntimeError so we skip this operator and let the interpreter
                     * raise an exception.
                     *
                     * TODO: provide a compile time error or warning for division by zero.
                     */
                    CONTINUE_FOLD_LOOP();
                }
            }
            
            switch (curr->opcode)
            {
                BINARY_FOLD(OP_add,  add_num)
                BINARY_FOLD(OP_sub,  sub_num)
                BINARY_FOLD(OP_mul,  mul_num)
                BINARY_FOLD(OP_div,  div_num)
                BINARY_FOLD(OP_idiv, div_num)
                BINARY_FOLD(OP_mod,  mod_num)
                default: break;
            }
        }
        CONTINUE_FOLD_LOOP();
    }
}

Compiler::Compiler(CompileState* state, Instr *ir, Def* d, LiteralPool* lp)
        : max_stack(0),
        def(d),
        start(ir),
        state(state)
{
    DoConstantFold(ir, lp);
}

Compiler::~Compiler() {}

void Compiler::DoCompile()
{
    if (!start)
        return;

    max_stack    = 0;
    Instr* icurr = start;
    int    space = 0;

    while (icurr)
    {
        space += OpcodeStackChange(icurr);        
        if (space > max_stack)
            max_stack =  space;        
        icurr = icurr->next;
    }

    Emit();
}

void Compiler::AddWord(code_t w)
{
    bytecode.Push(w);
}

void Compiler::Emit()
{
    start->CalcPos();
    Instr* curr = start;
    Instr* next = 0;

    while (curr)
    {
        next = curr->next;

        Opcode       oc  = curr->opcode;
        code_t       bcn = 0;
        OpcodeFormat fmt = OpcodeFormats[oc];
                
        switch (oc)
        {
        case OP_setlocal0:
        case OP_setlocal1:
        case OP_setlocal2:
        case OP_setlocal3:
        case OP_setlocal4:
        case OP_setlocal:
        case OP_forto:
            {
                // Set the range for local var info.
                if ((curr->operandu1 && curr->target) || (curr->opcode == OP_forto))
                {
                    def->SetLocalRange(curr->operand, curr->pos, curr->target->pos);
                }
            }
            break;
        default: break;
        }
        
        switch (fmt)
        {
        case OF_none:
        {
            bcn = PIKA_MAKE_B(oc);
            AddWord(bcn);
        }
        break;
        case OF_target:
        {
            bcn = PIKA_MAKE_W(oc, curr->target->pos);
            AddWord(bcn);
        }
        break;
        case OF_w:
        {
            bcn = PIKA_MAKE_W(oc, curr->operand);
            AddWord(bcn);
        }
        break;
        case OF_bw:
        {
            bcn = PIKA_MAKE_BW(oc, curr->operandu1, curr->operand);
            AddWord(bcn);
        }
        break;
        case OF_zero:
        default:
        {
            if (oc == BREAK_LOOP || oc == CONTINUE_LOOP)
            {
                if (oc == BREAK_LOOP)
                    state->SyntaxError(curr->line, "break statement found outside of a loop.");
                else
                    state->SyntaxError(curr->line, "continue statement found outside of a loop.");

                RaiseException(Exception::ERROR_syntax, "unhandled continue/break statement");
            }
        }
        break;
        }
        curr = next;
    }
}

void PrintBytecode(const u1* bc, size_t len)// OPCODE-CHANGE
{
    const u1* curr = bc;
    const u1* end  = curr + len;

    while (curr < end)
    {
        OpcodeLayout layout;
        layout.instruction = *curr++;
        Opcode oc = (Opcode)layout.opcode;

        const char* name = OpcodeNames[oc];
        OpcodeFormat fmt = OpcodeFormats[oc];

        switch (fmt)
        {
        case OF_none:
        {
            printf("%s\n", name);
        }
        break;
        case OF_target:
        case OF_w:
        {
            printf("%s %d\n", name, layout.w);
        }
        break;
        case OF_bw:
        {
            printf("%s %d %d\n", name, layout.b, layout.w);
        }
        break;
        case OF_zero:
        default:
        {
            RaiseException(Exception::ERROR_syntax, "ill-formed bytecode");
        }
        }
    }
}

}// pika
