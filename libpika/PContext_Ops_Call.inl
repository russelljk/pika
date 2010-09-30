/*
 *  PContext_Ops_Call.inl
 *  See Copyright Notice in Pika.h
 */

/////////////////////////////////////////// OP_apply ////////////////////////////////////////////

PIKA_OPCODE(OP_apply)
{
    u1 argc = GetByteOperand(instr);
    u1 kwargc = GetByte2Operand(instr);
    u1 retc = GetByte3Operand(instr); 
        
    Value frame = PopTop();
    Value selfobj = PopTop();
    Value var_arg = PopTop();
    Value kw_arg = PopTop();
    
    Array* array = 0;
    Dictionary* dict = 0;
    size_t amt = 0;
    size_t array_size = 0;
    size_t dict_size = 0;
    
    if (!var_arg.IsNull()) {
        if (engine->Array_Type->IsInstance(var_arg)) {
            array = static_cast<Array*>(var_arg.val.object);            
            array_size = array->GetLength();
            
            if (array_size > PIKA_MAX_ARGS) {
                RaiseException(Exception::ERROR_type, "attempt to apply variable argument call with an Array with too many members.");
            }
            
            argc += (u1)array_size;
            amt += array_size;            
        } else {
            RaiseException(Exception::ERROR_type, "attempt to apply invalid variable argument to a function call.");
        }
    }
    
    if (!kw_arg.IsNull()) {
        if (engine->Dictionary_Type->IsInstance(kw_arg)) {
            dict = static_cast<Dictionary*>(kw_arg.val.object);
            dict_size = dict->Elements().count;
            
            if (dict_size > PIKA_MAX_KWARGS) {
                RaiseException(Exception::ERROR_type, "attempt to apply keyword argument call with a Dictionary with too many members.");
            }
            
            amt += dict_size * 2;
        } else {
            RaiseException(Exception::ERROR_type, "attempt to apply invalid keyword argument to a function call.");
        }
    }
    
    CheckStackSpace(amt + closure->def->stackLimit);
    
    Value* old_sp = sp;
    StackAlloc(amt);
    
    Value* start = sp - amt;
    
    if (array) {
        if (kwargc) {
            Value* copy_from = old_sp - kwargc*2;
            Value* copy_to = sp;
            start = copy_from;
            while (copy_from < old_sp) {
                *copy_to++ = *copy_from++;
            }            
        }
        
        Buffer<Value>::Iterator end_iter = array->GetElements().End();
        for (Buffer<Value>::Iterator iter = array->GetElements().Begin(); iter != end_iter; ++iter) {
            *start = *iter;
            start++;
        }
    }
    
    if (dict) {
        start += kwargc*2;
        for (Table::Iterator iter = dict->Elements().GetIterator(); iter; ++iter) {
            *start = iter->key;
            *start = iter->val;
            start += 2;
        }
    }
    
    ASSERT(start == sp);
    
    Push(frame);
    Push(selfobj);
    
    kwargc += (u1)dict_size;
    
    if (SetupCall(argc, retc, kwargc, false))
    {
        ++numcalls;
    }    
}

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
