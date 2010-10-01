/*
 *  PContext_Ops_Std.inl
 *  See Copyright Notice in Pika.h
 */
/*
 * Does nothing.
 */
PIKA_OPCODE(OP_nop)
PIKA_NEXT()
/*
 * Duplicates the top-most element of the stack.
 */
PIKA_OPCODE(OP_dup)
{
    Push(Top());
}
PIKA_NEXT()
/*
 * Swaps the two top-most elements on the stack.
 */
PIKA_OPCODE(OP_swap)
{
    Value& vTop0 = Top();
    Value& vTop1 = Top1();
    Swap(vTop0, vTop1);
}
PIKA_NEXT()
/*
 * Jumps to a certain position unconditionally.
 */
PIKA_OPCODE(OP_jump)
{
    u2 jmppos = GetShortOperand(instr);
    pc = closure->GetBytecode() + jmppos;
}
PIKA_NEXT()
/*
 * Jumps to a certain position if an expression evaluates to false.
 */
PIKA_OPCODE(OP_jumpiffalse)
{
    Value& t = PopTop();
    u2 jmppos = GetShortOperand(instr);
    
    if (t.tag == TAG_boolean)
    {
        if (!t.val.index)
        {
            pc = closure->GetBytecode() + jmppos;
        }
    }
    else if (!engine->ToBoolean(this, t))
    {
        pc = closure->GetBytecode() + jmppos;
    }
}
PIKA_NEXT()
/*
 * Jumps to a certain position if an expression evaluates to true.
 */
PIKA_OPCODE(OP_jumpiftrue)
{
    Value& t = PopTop();
    u2 jmppos = GetShortOperand(instr);
    
    if (t.tag == TAG_boolean)
    {
        if (t.val.index)
        {
            pc = closure->GetBytecode() + jmppos;
        }
    }
    else if (engine->ToBoolean(this, t))
    {
        pc = closure->GetBytecode() + jmppos;
    }
}
PIKA_NEXT()
/*
 * Pushes null onto the stack.
 */
PIKA_OPCODE(OP_pushnull)
{
    PushNull();
}
PIKA_NEXT()
/*
 * Pushes the current self object onto the stack.
 */
PIKA_OPCODE(OP_pushself)
{
    Push(self);
}
PIKA_NEXT()
/*
 * Pushes the Boolean value true onto the stack.
 */
PIKA_OPCODE(OP_pushtrue)
{
    PushTrue();
}
PIKA_NEXT()
/*
 * Pushes the Boolean value false onto the stack.
 */
PIKA_OPCODE(OP_pushfalse)
{
    PushFalse();
}
PIKA_NEXT()

PIKA_OPCODE(OP_pushliteral0) Push(closure->GetLiteral(0)); PIKA_NEXT() // The 0th indexed literal is pushed onto the stack.
PIKA_OPCODE(OP_pushliteral1) Push(closure->GetLiteral(1)); PIKA_NEXT() // ... 1st ...
PIKA_OPCODE(OP_pushliteral2) Push(closure->GetLiteral(2)); PIKA_NEXT() // ... 2nd ...
PIKA_OPCODE(OP_pushliteral3) Push(closure->GetLiteral(3)); PIKA_NEXT() // ... 3rd ...
PIKA_OPCODE(OP_pushliteral4) Push(closure->GetLiteral(4)); PIKA_NEXT() // ... 4th ...
/*
 * The Nth indexed literal is pushed onto the stack.
 */
PIKA_OPCODE(OP_pushliteral)
{
    u2 index = GetShortOperand(instr);
    Push(closure->GetLiteral(index));
}
PIKA_NEXT()

PIKA_OPCODE(OP_pushlocal0) Push(GetLocal(0)); PIKA_NEXT() // The 0th indexed local variable is pushed onto the stack.
PIKA_OPCODE(OP_pushlocal1) Push(GetLocal(1)); PIKA_NEXT() // ... 1st ...
PIKA_OPCODE(OP_pushlocal2) Push(GetLocal(2)); PIKA_NEXT() // ... 2nd ...
PIKA_OPCODE(OP_pushlocal3) Push(GetLocal(3)); PIKA_NEXT() // ... 3rd ...
PIKA_OPCODE(OP_pushlocal4) Push(GetLocal(4)); PIKA_NEXT() // ... 4th ...
/*
 * The Nth indexed local variable is pushed onto the stack.
 */
PIKA_OPCODE(OP_pushlocal)
{
    u2 index = GetShortOperand(instr);
    Push(GetLocal(index));
}
PIKA_NEXT()
/*
 * Gets a global variable from the current package and pushes it onto the stack.
 */
PIKA_OPCODE(OP_pushglobal)
{
    u2 index = GetShortOperand(instr);
    
    const Value& name = closure->GetLiteral(index);
    Value res(NULL_VALUE);
    if (package && package->GetGlobal(name, res))
    {
        if (res.tag == TAG_property)
        {
            Push(this->package);
            if (DoPropertyGet(numcalls, res.val.property))
            {
                PIKA_NEXT()
            }
            else
            {
                ReportRuntimeError(Exception::ERROR_runtime,
                                   "Attempt to read global property '%s'.",
                                   engine->ToString(this, name)->GetBuffer());
            }
        }
        else
        {
            Push(res);
        }
    }
    else
    {
#   if defined( PIKA_ALLOW_MISSING_GLOBALS )
        PushNull();
#   else
        ReportRuntimeError(Exception::ERROR_runtime,
                           "Attempt to read global variable '%s'.",
                           engine->ToString(this, name)->GetBuffer());
#   endif
    }
}
PIKA_NEXT()
/*
 * Gets a variable from the
 * current self object.
 */
PIKA_OPCODE(OP_pushmember)
{
    u2 index = GetShortOperand(instr);
    const Value& name = closure->GetLiteral(index);
    
    Push(self);
    Push(name);
    
    OpDotGet(numcalls, oc, OVR_get);
}
PIKA_NEXT()
/*
 * Gets a bound local variable from a parent function's environment.
 */
PIKA_OPCODE(OP_pushlexical)
{
    u1 depth = GetByteOperand(instr);
    u2 index = GetShortOperand(instr);
    
    Push(GetOuter(index, depth));
}
PIKA_NEXT()
/*
 * Sets a local variable from the
 * current function call.
 */
PIKA_OPCODE(OP_setlocal0) SetLocal(PopTop(), 0); PIKA_NEXT()
PIKA_OPCODE(OP_setlocal1) SetLocal(PopTop(), 1); PIKA_NEXT()
PIKA_OPCODE(OP_setlocal2) SetLocal(PopTop(), 2); PIKA_NEXT()
PIKA_OPCODE(OP_setlocal3) SetLocal(PopTop(), 3); PIKA_NEXT()
PIKA_OPCODE(OP_setlocal4) SetLocal(PopTop(), 4); PIKA_NEXT()

PIKA_OPCODE(OP_setlocal)
{
    Value& t = PopTop();
    u2 index = GetShortOperand(instr);
    
    SetLocal(t, index);
}
PIKA_NEXT()
/*
 * Sets a global variable from the
 * current package.
 */
PIKA_OPCODE(OP_setglobal)
{
    u2 index = GetShortOperand(instr);
    
    const Value& name = closure->GetLiteral(index);
    Value& val  = Top();
    if (!this->package->SetGlobal(name, val))
    {
        Value res(NULL_VALUE);
        if (this->package->GetGlobal(name, res) && res.tag == TAG_property)
        {
            Push(this->package);
            if (DoPropertySet(res.val.property))
            {
                PIKA_NEXT()
            }
            else
            {
                ReportRuntimeError(Exception::ERROR_runtime,
                                   "Attempt to set read-only property '%s'.",
                                   engine->ToString(this, name)->GetBuffer());
            }
        }
        else
        {
            ReportRuntimeError(Exception::ERROR_runtime,
                               "Attempt to set variable '%s'.",
                               engine->ToString(this, name)->GetBuffer());
        }
    }
    else
    {
        Pop();
    }
}
PIKA_NEXT()
/*
 * Sets a variable from the
 * current self object.
 */
PIKA_OPCODE(OP_setmember)
{
    u2 index = GetShortOperand(instr);
    const Value& name = closure->GetLiteral(index);
    
    Push( self );
    Push( name );
    
    OpDotSet(oc, OVR_set);
}
PIKA_NEXT()
/*
 * Sets a local variable from a
 * previous|outer function call.
 */
PIKA_OPCODE(OP_setlexical)
{
    Value& t = PopTop();
    u1 depth = GetByteOperand(instr);
    u2 index = GetShortOperand(instr);
    SetOuter(t, index, depth);
}
PIKA_NEXT()

PIKA_OPCODE(OP_pop)
{
    Pop();
}
PIKA_NEXT()
/* Store the top stack value in the accumulator (see OP_retacc). */
PIKA_OPCODE(OP_acc)
{
    acc = PopTop();
}
PIKA_NEXT()
