/*
 *  PContext_Ops_Call.inl
 *  See Copyright Notice in Pika.h
 */

////////////////////////////////////////////// OP_new //////////////////////////////////////////////

PIKA_OPCODE(OP_new)
{
    Value&   vtype = Top();
    Value&   vself = Top1();
    const u2 argc  = GetShortOperand(instr);
    
    if (!vtype.IsDerivedFrom(Type::StaticGetClass()))
    {
        ReportRuntimeError(Exception::ERROR_runtime, "operator new must be used on a Type instance: given %s", engine->GetTypenameOf(vtype)->GetBuffer());
    }    
    
    Type* typeobj = vtype.val.type;
    
    if (typeobj->GetField(Value(engine->GetOverrideString(OVR_new)), vtype)) // XXX: Get Override
    {
        vself.Set(typeobj);
        if (SetupCall(argc, false, 1))
        {
            ++numcalls;
        }
    }
    else
    {
        typeobj->CreateInstance(vself);
        if (typeobj->GetField(Value(engine->GetOverrideString(OVR_init)), vtype)) // XXX: Get Override
        {
            if (SetupCall(argc, false, 1))
            {
                ++numcalls;
                newCall = true;
            }
            else
            {
                Top() = vself;
            }
        }
        else
        {
            Pop();
            Top() = vself;
        }
    }
}
PIKA_NEXT()

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
