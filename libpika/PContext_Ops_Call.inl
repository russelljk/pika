/*
 *  PContext_Ops_Call.inl
 *  See Copyright Notice in Pika.h
 */

////////////////////////////////// OP_tailapply ////////////////////////////////

PIKA_OPCODE(OP_tailapply)
{
    if (env) {
        // Before we replace the current call stack we need to allow the LexicalEnv
        // a chance to copy the local variables.
        env->EndCall();
    }
    u1 argc = GetByteOperand(instr);
    u1 kwargc = GetByte2Operand(instr);
        
    if (!OpApply(argc, 1, kwargc, true))
    {
        PIKA_RET(1)
    }
}
PIKA_NEXT()

////////////////////////////////// OP_apply ////////////////////////////////////

PIKA_OPCODE(OP_apply)
{
    u1 argc = GetByteOperand(instr);
    u1 kwargc = GetByte2Operand(instr);
    u1 retc = GetByte3Operand(instr); 
    
    if (OpApply(argc, retc, kwargc, false))
    {
        ++numcalls;
    }
}
PIKA_NEXT()

////////////////////////////////// OP_tailcall /////////////////////////////////

PIKA_OPCODE(OP_tailcall)
{
    if (env) {
        // Before we replace the current call stack we need to allow the LexicalEnv
        // a chance to copy the local variables.
        env->EndCall();
    }
    const u1 argc = GetByteOperand(instr);
    const u1 kwargc = GetByte2Operand(instr);
    
    if (!SetupCall(argc, 1, kwargc, true))
    {
        PIKA_RET(1)
    }
}
PIKA_NEXT()

////////////////////////////////// OP_call /////////////////////////////////////

PIKA_OPCODE(OP_call)
{
    const u1 argc = GetByteOperand(instr);
    const u1 kwargc = GetByte2Operand(instr);
    const u1 retc = GetByte3Operand(instr);    
                
    if (SetupCall(argc, retc, kwargc, false))
    {
        ++numcalls;
    }
}
PIKA_NEXT()
