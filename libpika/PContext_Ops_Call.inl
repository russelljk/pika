/*
 *  PContext_Ops_Call.inl
 *  See Copyright Notice in Pika.h
 */

/////////////////////////////////////////// OP_tailcall ////////////////////////////////////////////

PIKA_OPCODE(OP_tailcall)
{
    if (env)
    {
        env->EndCall();
    }
    const u1 argc = GetByteOperand(instr);
    const u1 kwa  = GetByte2Operand(instr);
    
    if (!SetupCall(argc, 1, kwa, true))
    {
        PIKA_RET(1)
    }
}
PIKA_NEXT()

////////////////////////////////////////////// OP_call /////////////////////////////////////////////

PIKA_OPCODE(OP_call)
{
    const u1 argc = GetByteOperand(instr);
    const u1  kwa = GetByte2Operand(instr);
    const u1 retc = GetByte3Operand(instr);    
                
    if (SetupCall(argc, retc, kwa, false))
    {
        ++numcalls;
    }
}
PIKA_NEXT()
