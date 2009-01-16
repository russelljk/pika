/*
 *  PContext_Ops_Std.inl
 *  See Copyright Notice in Pika.h
 */
PIKA_OPCODE(OP_nop)
PIKA_NEXT()

PIKA_OPCODE(OP_dup)
{
    Push(Top());
}
PIKA_NEXT()

PIKA_OPCODE(OP_swap)
{
    Value& vTop0 = Top();
    Value& vTop1 = Top1();
    Swap(vTop0, vTop1);
}
PIKA_NEXT()

PIKA_OPCODE(OP_jump)
{
    u2 jmppos = GetShortOperand(instr);
    pc = closure->GetBytecode() + jmppos;
}
PIKA_NEXT()

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

PIKA_OPCODE(OP_assert)
{
    Value& t  = PopTop();
    bool a_ok = false;
    
    if (t.tag == TAG_boolean)
    {
        a_ok = (t.val.index != 0);
    }
    else
    {
        a_ok = engine->ToBoolean(this, t);
    }
    if (!a_ok)
    {
        ReportRuntimeError(Exception::ERROR_assert, "Assertion failed.");
    }
}
PIKA_NEXT()

PIKA_OPCODE(OP_pushnull) 
{
    PushNull();
}
PIKA_NEXT()

PIKA_OPCODE(OP_pushself) 
{
    Push(self);
}
PIKA_NEXT()
/*
 * OP_pushtrue ------------------------------------------------------------------------------------
 */
PIKA_OPCODE(OP_pushtrue)
{
    PushTrue();
}
PIKA_NEXT()
/*
 * OP_pushfalse -----------------------------------------------------------------------------------
 */
PIKA_OPCODE(OP_pushfalse)
{
    PushFalse();
}
PIKA_NEXT()
/*
 * Gets a literal|constant variable.
 */
PIKA_OPCODE(OP_pushliteral0) Push(closure->GetLiteral(0)); PIKA_NEXT()
PIKA_OPCODE(OP_pushliteral1) Push(closure->GetLiteral(1)); PIKA_NEXT()
PIKA_OPCODE(OP_pushliteral2) Push(closure->GetLiteral(2)); PIKA_NEXT()
PIKA_OPCODE(OP_pushliteral3) Push(closure->GetLiteral(3)); PIKA_NEXT()
PIKA_OPCODE(OP_pushliteral4) Push(closure->GetLiteral(4)); PIKA_NEXT()

PIKA_OPCODE(OP_pushliteral)
{
    u2 index = GetShortOperand(instr);
    Push(closure->GetLiteral(index));
}
PIKA_NEXT()
/*
 * Gets a local variable from the
 * current function call.
 */
PIKA_OPCODE(OP_pushlocal0) Push(GetLocal(0)); PIKA_NEXT()
PIKA_OPCODE(OP_pushlocal1) Push(GetLocal(1)); PIKA_NEXT()
PIKA_OPCODE(OP_pushlocal2) Push(GetLocal(2)); PIKA_NEXT()
PIKA_OPCODE(OP_pushlocal3) Push(GetLocal(3)); PIKA_NEXT()
PIKA_OPCODE(OP_pushlocal4) Push(GetLocal(4)); PIKA_NEXT()

PIKA_OPCODE(OP_pushlocal)
{
    u2 index = GetShortOperand(instr);
    Push(GetLocal(index));
}
PIKA_NEXT()
/*
 * Gets a global variable from the
 * current package.
 */
PIKA_OPCODE(OP_pushglobal)
{
    u2 index = GetShortOperand(instr);

    const Value& name = closure->GetLiteral(index);
#   if 0
    Value res(NULL_VALUE);
    if (self.tag >= TAG_basic && self.val.basic->GetSlot(name, res))
    {
        Push(res);
    }
    else if (package && self.val.basic != (Basic*)package && package->GetSlot(name, res))
    {
        Push(res);
    }
    else
    {
        RaiseException("Attempt to read global.");
    }
#   endif
    Push(this->package);
    Push(name);

    OpDotGet(numcalls, oc);
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

    if (!self.IsObject())
        RaiseException("invalid self object.\n");

    Push(self);
    Push(name);

    OpDotGet(numcalls, oc);
}
PIKA_NEXT()
/*
 * Gets a local variable from a
 * previous|outer function call.
 */
PIKA_OPCODE(OP_pushouter)
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

    Push(this->package);
    Push(name);

    OpDotSet(numcalls, oc);
}
PIKA_NEXT()
/*
 * Sets a variable from the
 * current self object.
 */
PIKA_OPCODE(OP_setmember)
{
    u2 index = GetShortOperand(instr);

    if (!self.IsObject())
        RaiseException("Invalid self object.\n");

    const Value& name = closure->GetLiteral(index);

    Push( self );
    Push( name );

    OpDotSet(numcalls, oc);
}
PIKA_NEXT()
/*
 * Sets a local variable from a
 * previous|outer function call.
 */
PIKA_OPCODE(OP_setouter)
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

PIKA_OPCODE(OP_acc)
{
    acc = PopTop();
}
PIKA_NEXT()
