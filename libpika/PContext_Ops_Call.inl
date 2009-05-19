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
    const u2 argc = GetShortOperand(instr);
    
    if (!SetupCall(argc, true, 1))
    {
        PIKA_RET(1)
    }
}
PIKA_NEXT()

////////////////////////////////////////////// OP_call /////////////////////////////////////////////

PIKA_OPCODE(OP_call)
{
    const u2 argc = GetShortOperand(instr);
    const u1 retc = GetByteOperand(instr);
    
    if (SetupCall(argc, false, retc))
    {
        ++numcalls;
    }
}
PIKA_NEXT()
