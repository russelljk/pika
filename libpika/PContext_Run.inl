/*
 *  PContext_Run.cpp
 *  See Copyright Notice in Pika.h
 *
 *  This File contains the main interpreter loop.
 *  Since a disproportionate amount of time is spent working on this particular method it made
 *  sense to give it its own file. Related opcodes families are further grouped into their own source
 *  files.
 */

/** This macro handles returns from Context::Run. */

#define PIKA_RET(V)                                                                 \
    --numcalls;                                                                     \
    if (generator) generator->Return();                                             \
    if (closure == 0 || (pc == 0 && !closure->IsNative()))                          \
    {                                                                               \
        --numRuns;                                                                  \
        state = DEAD;                                                               \
        if (prev)                                                                   \
        {   callsCount = 1;                                                         \
            DoSuspend(GetStackPtr() - V, V);                                        \
        }                                                                           \
        return;                                                                     \
    }                                                                               \
    if (numcalls == 0)                                                              \
    {                                                                               \
        callsCount = 1;                                                             \
        --numRuns;                                                                  \
        return;                                                                     \
    }

/** This macro sets the bytcode position of a new closure with respect to its parent. */
#define SET_PARENTPC(FUNCIN)                                \
    ptrdiff_t pos_in_parent = 0;                            \
    if (closure)                                            \
    {                                                       \
        pos_in_parent = (pc - 1) - closure->GetBytecode();  \
    }                                                       \
    FUNCIN->bytecodePos = pos_in_parent;

////////////////////////////////////////////////////////////////////////////////////////////////////

/** This macro calls the per instruction debug hook if its enabled. */
#ifndef PIKA_NO_HOOKS
#   define PIKA_CHECK_INSTR_HOOK()                              \
    if (engine->HasHook(HE_instruction))                        \
    {                                                           \
        InstructionData instr_data;                             \
        instr_data.context  = this;                             \
        instr_data.pc       = pc;                               \
        instr_data.function = closure;                          \
        engine->CallHook(HE_instruction, (void*)&instr_data);   \
    }
#else
#   define PIKA_CHECK_INSTR_HOOK()
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: Check what version of GCC introduced the labels as values extension and check for that version.
#if defined(PIKA_GNUC)
#   define PIKA_LABELS_AS_VALUES
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Opcode dispatch.
 */
#if defined(PIKA_LABELS_AS_VALUES)
/* Labels as values extension (GCC only).
 * Removes the overhead of the switch statement dispatch by performing an array index
 * followed by a jump directly to the next opcode. Enabling this usually provides 
 * a performace increase so its enabled by default if the compiler is GCC.
 */
#   define PIKA_OPCODE(x)           lbl_##x:
#   define PIKA_NEXT()                                  \
    {                                                   \
        if (state != RUNNING)                           \
            break;                                      \
        instr = *pc++;                                  \
        oc = PIKA_GET_OPCODEOF(instr);                  \
        PIKA_CHECK_INSTR_HOOK()                         \
        goto *static_jmp_addresses[oc];  /*the jump */  \
    }
#   define PIKA_END_DISPATCH()
#   define PIKA_BEGIN_DISPATCH()    goto *static_jmp_addresses[oc];
#else
/* Switch dispatch. Will work with any compiler. */
#   define PIKA_OPCODE(x)           case x:
#   define PIKA_NEXT()              continue;
#   define PIKA_END_DISPATCH()      default: RaiseException("unknown opcode"); }
#   define PIKA_BEGIN_DISPATCH()    switch (oc) {
#endif
/*
 *  This is main workhorse of the interpreter. Opcode dispatch and execution is all handled here.
 *  GCC's labels as values extension is used if present. For other compilers a 
 *  standard switch based dispatch is used.
 *
 *  Any exception raised while executing the script is handled 'in house' if possible.
 *
 *  We try to make this as 'stackless' as possible and every attempt should be made to limit
 *  the number of times Run calls itself. 'numRuns' keeps track of the number of times
 *  Run has been called, while the local variable 'numcalls' keeps track of the number of times Run
 *  has 'inlined' the call. We cannot return from Run until numcalls == 0, an exception is thrown
 *  or we yield. When we yield we must exit Run and store numcalls so that we can resume properly.
 *  This means a context cannot yield more than once without resuming, (not a problem as long you 
 *  do not call a yielded context).
 */
void Context::Run()
{
// Here we are setting up a array of goto addressess for use with 
// GCC's 'labeled gotos' aka 'labels as values' extension.

#   if defined(PIKA_LABELS_AS_VALUES)
#       ifdef DECL_OP
#           undef DECL_OP
#       endif
#       define DECL_OP(XOP, XNAME, XLENGTH, XFORMAT, XDESCR) &&lbl_##XOP,    
    static const void* static_jmp_addresses[] =
        {
#       include "POpcodeDef.inl" // Initialize this once, the first time Run is called.
        };
#   endif
        
    if (++numRuns > PIKA_MAX_NATIVE_RECURSION)
        RaiseException("C function recursion limit reached.");
        
    bool inlineThrow = false;
    ASSERT(state != SUSPENDED || callsCount >= 1);
    
    int numcalls  = state == SUSPENDED ? callsCount : 1;    //  Number of calls that have been "inlined". 
                                                            // If we are suspended then we need the #calls from the previous run.                                                            
    state         = RUNNING; //  Set our state to running                                                            
    Opcode oc     = OP_nop;  //  Current opcode we are dispatching.
    code_t instr  = 0;       //  Current instruction (including operands). see the bytecode layout in gOpcode.h
    
    Activate();              //  We are now the active thread.
    
    while (state == RUNNING)
    {
        try
        {
            instr = *pc++;
            oc = PIKA_GET_OPCODEOF(instr);
            
            //  Pika_PrintInstruction(instr);
            
            //  We check for the existance of the instruction hook (typically used by debuggers).
            //  If you are concerned about speed and don't care about supporting debugging you
            //  can remove this check.
            
            PIKA_CHECK_INSTR_HOOK()
            
            PIKA_BEGIN_DISPATCH()
            
            //  Standard push|pop get|set operations
#           include "PContext_Ops_Std.inl"
            
            //  All arithmetic/logical operators that can be overriden.
#           include "PContext_Ops_Arith.inl"
            
            //  Call|new operations
#           include "PContext_Ops_Call.inl"
            
            //  dot get|set
            
            PIKA_OPCODE(OP_dotget)
            {
                OpDotGet(numcalls, oc, OVR_get);
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_dotset)
            {
                OpDotSet(oc, OVR_set);
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_subget)
            {
                OpDotGet(numcalls, oc, OVR_getat);
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_subset)
            {
                OpDotSet(oc, OVR_setat);
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_locals)
            {
                GCPAUSE_NORUN(engine);
                CreateEnv();                
                ptrdiff_t pc_index = ((pc - 1) - closure->GetBytecode());
                Object* obj = LocalsObject::Create(engine, engine->LocalsObject_Type, closure, env, pc_index);
                Push(obj);
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_method)
            {
            
                /* Creates a new Function.
                    
                    Format => opcode [defaults count: u2]
                */
                u2 defaultArgc = GetShortOperand(instr);
                
                /* Stack:
                        [ ...  ]
                        [ self ] < self object for the method
                        [ def  ] < function def
                        [ arg0 ] < default args (if any)
                        [ ...  ]
                        [ argN ]
                                 < top of the stack
                */
                Value& vdef  = TopN(defaultArgc);
                Value& vself = TopN(defaultArgc + 1);
                
                if (vdef.tag == TAG_def)
                {
                    // create a new function closure.
                    Def* fun = vdef.val.def;
                    
                    DoClosure(fun, vdef, &vself);
                    SET_PARENTPC(fun);
                    Function* function = vdef.val.function;
                    
                    if (defaultArgc)
                    {
                        u4 num = Min(defaultArgc, (u2)fun->numArgs);
                        function->defaults = Defaults::Create(engine, GetStackPtr() - num, num);
                    }
                    
                    Swap(vdef, vself);
                    Pop(defaultArgc + 1);
                    
                    // Stack:
                    //      [ ...     ]
                    //      [ closure ]
                    //                  < top of the stack
                }
                else
                {
                    ReportRuntimeError(Exception::ERROR_runtime,
                                       "Attempted to create a function closure from type %s.",
                                       engine->GetTypenameOf(vdef)->GetBuffer());
                }
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_property)
            {
                GCPAUSE(engine);
                
                Value& name   = Top2();
                Value& getter = Top1();
                Value& setter = Top();
                bool hasGet = false;
                bool hasSet = false;
                String* propname = 0;
                
                if (name.IsString())
                {
                    propname = name.val.str;
                }
                else if (name.IsNull())
                {
                    propname = engine->emptyString;
                }
                else
                {
                    ReportRuntimeError(Exception::ERROR_runtime,
                                       "invalid property name. expected string or null.");
                }
                
                if (getter.IsDerivedFrom(Function::StaticGetClass()))
                {
                    hasGet = true;
                }
                else if ( !getter.IsNull() )
                {
                    ReportRuntimeError(Exception::ERROR_runtime,
                                       "invalid getter for property %s.",
                                       propname->GetBuffer());
                }
                
                if (setter.IsDerivedFrom(Function::StaticGetClass()))
                {
                    hasSet = true;
                }
                else if  ( !setter.IsNull() )
                {
                    ReportRuntimeError(Exception::ERROR_runtime,
                                       "invalid setter for property %s.",
                                       propname->GetBuffer());
                }
                
                Property* p = 0;
                if (hasGet && hasSet)
                {
                    p = Property::CreateReadWrite(engine,
                                                  propname,
                                                  getter.val.function,
                                                  setter.val.function);
                }
                else if (hasGet)
                {
                    p = Property::CreateRead(engine,
                                             propname,
                                             getter.val.function);
                }
                else if (hasSet)
                {
                    p = Property::CreateWrite(engine,
                                              propname,
                                              setter.val.function);
                }
                else
                {
                    ReportRuntimeError(Exception::ERROR_runtime,
                                       "Attempt to create property %s without a getter or setter function.",
                                       propname->GetBuffer());
                }
                
                name.Set(p);
                Pop(2);
            }
            PIKA_NEXT()

            PIKA_OPCODE(OP_subclass)
            {
                GCPAUSE(engine);
                /*
                [ typename ]
                [ base     ]
                [ < sp >   ]
                */
                Value&   vbase  = Top(); // base type
                Value&   vname  = Top1();
                Value&   vpkg   = Top2();
                String*  name   = vname.val.str;
                Type*    newtype   = 0;
                Package* superPkg  = 0;
                bool     nullsuper = vbase.IsNull();
                bool     specified_pkg =  false;
                
                if (vpkg.IsDerivedFrom(Package::StaticGetClass()))
                {
                    superPkg = vpkg.val.package;specified_pkg=true;
                }
                else
                {
                    superPkg = this->package;
                }               
                                
                if (vbase.IsDerivedFrom(Type::StaticGetClass()) || nullsuper)
                {
                    // Super is a valid type object.
                    Type* super = nullsuper ? engine->Object_Type : vbase.val.type;
                    
                    if (super->IsFinal())
                    {
                        // Super cannot be derived from
                        ReportRuntimeError(Exception::ERROR_runtime,
                                           "class '%s' is final and cannot be subclassed.",
                                           super->GetName()->GetBuffer());
                    }          
                    else
                    {
                        /* TODO { Ideally we would call CreateInstance instead. 
                         *        If so how do we initialize the super package
                         *        and base type? }
                         */
                        newtype = super->NewType(name, superPkg);
                    }
                }
                else
                {
                    // We cannot extend a non-type object.
                    String* supername = engine->GetTypenameOf(vbase);
                    ReportRuntimeError(Exception::ERROR_runtime,
                                       "Attempt to extend non-type derived value: %s.",
                                       supername->GetBuffer());
                }
                Pop(2);
                vpkg.Set(newtype);
                
                /*
                   [ type   ]                  
                   [ < sp > ]
                */
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_array)
            {
                GCPAUSE(engine);
                
                u2 elemCount = GetShortOperand(instr);
                
                Value* elements = (elemCount) ? (GetStackPtr() - elemCount) : 0;
                
                Pop(elemCount);
                Object* array = Array::Create(engine, engine->Array_Type, elemCount, elements);
                Push(array);
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_objectliteral)
            {
                GCPAUSE(engine);
                
                Value vobj(NULL_VALUE);                
                
                Type* type_obj = 0;
                
                u2 elemCount = GetShortOperand(instr);
                u2 elemDepth = elemCount * 2;
                
                Value type_val = PopTop();
                
                Value* beg = GetStackPtr() - elemDepth;
                Value* end = GetStackPtr();
                
                /* If the value on top of the stack is a Type we are creating
                 * an object literal. 
                 *
                 * If the value is null we are creating a Dictionary.
                 *
                 * For any other value we need to raise an exception.
                 */
                if (type_val.IsDerivedFrom(Type::StaticGetClass()))
                {
                    type_obj = type_val.val.type;
                }
                else if (type_val.IsNull())
                {
                    type_obj = engine->Dictionary_Type;
                }
                else
                {
                    ReportRuntimeError(Exception::ERROR_runtime,
                                       "invalid type value for object literal.");
                }
                
                /* Some one might have specified the type as a Dictionary manually. */
                bool is_dict = type_obj->IsSubtype(engine->Dictionary_Type);

                type_obj->CreateInstance(vobj);
                Object* obj = vobj.val.object;
                
                if (!(vobj.IsObject() && obj))
                {
                    RaiseException("Attempt to create object literal failed.");
                }
                
                /* Loop through each {key,val} pair and add it to the new
                 * object/dictionary.
                 *
                 * TODO { If we can break this up by writing:
                 *        if (is_dict)
                 *          while ...
                 *              add to dict
                 *          end
                 *        else
                 *          while (...)
                 *              add to object
                 *          end
                 *        end
                 *       We would only need to test if(is_dict) once instead of
                 *       once per element. }
                 */
                while (beg < end)
                {
                    Value* val  = beg;
                    Value* key = beg + 1;
                    if (is_dict)
                    {
                        obj->BracketWrite(*key, *val);
                    }
                    else if (key->IsNull())
                    {
                        /* If the key is null then this is an inlined
                         * Property or Function.
                         */
                        if (val->IsDerivedFrom(Function::StaticGetClass()))
                        {
                            obj->AddFunction(val->val.function);
                        }
                        else if (val->IsProperty())
                        {
                            obj->AddProperty(val->val.property);
                        }
                    }
                    else
                    {
                        obj->SetSlot(*key, *val);
                    }
                    beg += 2;
                }
                
                Pop(elemDepth);
                Push(obj);
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_super)
            {
                OpSuper();
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_forto)
            {
                /* Grab the offset of the for loop's local variables and
                 * the top three values off the stack.
                 *
                 * [ .... ]
                 * [ from ]
                 * [ to   ]
                 * [ step ] < sp
                 */
                u2 localOffset = GetShortOperand(instr);
                
                Value& from = Top2();
                Value& to = Top1();
                Value& step = Top();
                
                
                /* We only deal with integers. So raise an exception if 
                 * thats not the case.
                 *
                 * TODO { Should we convert reals to integers here or not? }
                 */
                if ((from.tag != TAG_integer) || (to.tag   != TAG_integer) || (step.tag != TAG_integer))
                {
                    ReportRuntimeError(Exception::ERROR_runtime,
                                       "for loop type error: expecting integer.");
                }
                
                /* To avoid an infinite loop we make sure the step variable points in the same
                 * direction as the range. Then we adjust the step's sign accordingly.
                 */
                pint_t  diff  = to.val.integer - from.val.integer;
                pint_t& istep = step.val.integer;
                
                if (!istep)
                {
                    istep = (diff < 0) ? -1 : 1;
                }
                
                /* WARNING {
                 * We need to modify the code by spacing the correct opcode needed.
                 *
                 * We need to do this EVERY time because another run through the code might
                 * yield a different comparison test.
                 * }
                 */
                if (istep < 0)
                {
                    *(pc + PIKA_FORTO_COMP_OFFSET) = PIKA_MAKE_B(OP_gt); // Set the op to >
                }
                else
                {
                    *(pc + PIKA_FORTO_COMP_OFFSET) = PIKA_MAKE_B(OP_lt); // Set the op to <
                }
                
                /* Set the loop variables: 'from', 'to' and 'step'. 
                 * The variables are stored in order.
                 */
                SetLocal(from, localOffset);
                SetLocal(to,   localOffset + 1);
                SetLocal(step, localOffset + 2);
                
                Pop(3);
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_retacc)
            Push(acc);
            /*
                Fall Through
                |
                V
            */
            PIKA_OPCODE(OP_ret)
            {
                OpReturn(1);                
#   ifndef PIKA_NO_HOOKS
                /* Call the return Hook if its present. */
                if (engine->HasHook(HE_return))
                {
                    engine->CallHook(HE_return, (void*)this);
                }
#   endif
                PIKA_RET(retCount)
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_retv)
            {
                int retc = GetShortOperand(instr);
                OpReturn(retc);
#   ifndef PIKA_NO_HOOKS
                /* Call the return Hook if its present. */
                if (engine->HasHook(HE_return))
                {
                    engine->CallHook(HE_return, (void*)this);
                }
#   endif
                PIKA_RET(retCount)
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_gennull)
            PushNull();
            /*
                Fall Through
                |
                V
            */
            PIKA_OPCODE(OP_gen)
            {
                OpYield(1);                
                PIKA_RET(retCount)
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_genv)
            {
                int retc = GetShortOperand(instr);
                OpYield(retc);
                PIKA_RET(retCount)
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_yieldnull)
            PushNull();
            /*
                Fall Through
                |
                V
            */
            PIKA_OPCODE(OP_yield)
            {
                if (!prev)
                {
                    ReportRuntimeError(Exception::ERROR_runtime,
                                       "cannot yield from this context: nothing to yield to.");
                }                
                
                if (nativeCallDepth > 0)
                {
                    ReportRuntimeError(Exception::ERROR_runtime,
                                       "cannot yield across a native call.");
                }
                
#   ifndef PIKA_NO_HOOKS
                if (engine->HasHook(HE_yield))
                {
                    engine->CallHook(HE_yield, (void*)this);
                }
#   endif
                Pop();
                callsCount = numcalls;
                DoSuspend(GetStackPtr(), 1);
                --numRuns;
                return;
            }
            PIKA_NEXT()
            PIKA_OPCODE(OP_yieldv)
            {
                u2 index = GetShortOperand(instr);
                
                if (!prev)
                {
                    ReportRuntimeError(Exception::ERROR_runtime,
                                       "cannot yield from this context: nothing to yield to.");
                }
                
                if (nativeCallDepth > 0)
                {
                    ReportRuntimeError(Exception::ERROR_runtime,
                                       "cannot yield across a native call.");
                }
                
#   ifndef PIKA_NO_HOOKS
                if (engine->HasHook(HE_yield))
                {
                    engine->CallHook(HE_yield, (void*)this);
                }
#   endif
                Pop(index);
                callsCount = numcalls;
                DoSuspend(GetStackPtr(), (ptrdiff_t)index);
                --numRuns;
                return;                
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_foreach)
            {
                GCPAUSE(engine);
                
                u2 index = GetShortOperand(instr);
                Value& enumlocal = GetLocal(index);
                
                Value& enumType = PopTop();
                
                if (!enumType.IsString())
                {
                    ReportRuntimeError(Exception::ERROR_type,
                                       "invalid foreach enumerator type %s.",
                                       engine->GetTypenameOf(enumType)->GetBuffer());
                }
                
                Enumerator* e = 0;
                Value& t = PopTop();
                
                if (t.IsEnumerator())
                {
                    e = t.val.enumerator;
                }
                else if (t.tag >= TAG_basic)
                {
                    Basic* c = t.val.basic;
                    e = c->GetEnumerator(enumType.val.str);
                }
                else
                {
                    e = ValueEnumerator::Create(engine, t, enumType.val.str);
                }
                if (!e)
                {
                    ReportRuntimeError(Exception::ERROR_runtime,
                                       "could not create enumerator for type %s",
                                       engine->GetTypenameOf(t)->GetBuffer());
                }
                e->Rewind();
                enumlocal.Set(e);
                bool res = e->IsValid();
                PushBool(res);
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_enumisvalid)
            {
                u2 index = GetShortOperand(instr);
                Value& t = GetLocal(index);
                
                if (t.tag == TAG_enumerator)
                {
                    Enumerator* e = t.val.enumerator;
                    e->Advance();
                    bool res = e->IsValid();
                    PushBool(res);
                }
                else
                {
                    ReportRuntimeError(Exception::ERROR_runtime,
                                       "invalid enumerator: %s.",
                                       engine->ToString(this, t)->GetBuffer());
                }
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_enumadvance)
            {
                u2 index = GetShortOperand(instr);
                Value& t = GetLocal(index);
                
                if (t.tag == TAG_enumerator)
                {
                    Enumerator* e = t.val.enumerator;
                    Value curr(NULL_VALUE);                    
                    e->GetCurrent(curr);
                    Push(curr);
                }
                else
                {
                    ReportRuntimeError(Exception::ERROR_runtime,
                                       "invalid enumerator: %s.",
                                       engine->ToString(this, t)->GetBuffer());
                }
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_same)
            {
                Value& a = Top();
                Value& b = Top1();
                
                if (a.tag != b.tag)
                    b.SetFalse();
                else
                    b.SetBool(a.val.index == b.val.index);
                    
                Pop();
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_notsame)
            {
                Value& a = Top();
                Value& b = Top1();
                
                if (a.tag != b.tag)
                    b.SetTrue();
                else
                    b.SetBool(a.val.index != b.val.index);
                    
                Pop();
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_is)
            {
                bool res = OpIs();
                Pop();
                Top().SetBool(res);
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_has)
            {
                bool res = OpHas();
                Pop();
                Top().SetBool(res);
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_pushtry)
            {
                u2 catchpc = GetShortOperand(instr);
                
                ExceptionBlock eb;
                eb.scope    = scopes.IndexOf(scopesTop);
                eb.catchpc  = catchpc;
                eb.sp       = sp - stack;
                eb.package  = this->package;
                eb.self     = self;
                handlers.Push(eb);
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_pophandler)
            {
                ASSERT(!handlers.IsEmpty());
                handlers.Pop();
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_raise)
            {
                // The programmer wants to raise an exception.

                Value& thrown = Top();
                ScriptException exception(thrown);
                switch (OpException(exception))
                {
                case ER_throw:
                    inlineThrow = true; // The exception handler inside run should re-throw this exception.
                    throw exception;    // "pass" it to the exception handler ... avert your eyes **yuck**
                case ER_continue:
                { 
                    PIKA_NEXT()         // We can handle the exception here.
                }
                case ER_exit:
                    return;             // We are dead, nothing to do but exit.
                default:
                    inlineThrow = true;
                    throw exception;
                }
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_retfinally)
            {
                ASSERT(!addressStack.IsEmpty());
                
                size_t offset = addressStack.Back();
                pc = closure->GetBytecode() + offset;
                addressStack.Pop();
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_callfinally)
            {
                code_t* byte_code_start = closure->GetBytecode();
                u2 finally_offset = GetShortOperand(instr);
                size_t offset = pc - byte_code_start;
                
                addressStack.Push(offset);
                pc = byte_code_start + finally_offset;
            }
            PIKA_NEXT()
                        
            PIKA_OPCODE(OP_pushwith)
            {
                // TODO: If OnUse raises an exception what about OnDispose.
                
                Value& v = Top();
                PushWithScope();
                self = v;                
                Pop();
                
                if (v.tag >= TAG_basic)
                {
                    Value res(NULL_VALUE);
                    if (v.val.basic->GetSlot(engine->OpUse_String, res))
                    {
                        Push(v);
                        Push(res);
                        
                        if (SetupCall(0))
                        {
                            Run();
                        }
                        Value& res = PopTop(); // result & exitwith string
                        if (!res.IsNull())
                            self = res;
                    }
                }
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_popwith)
            {                
                Value oldself = self;
                PopWithScope();
                if (oldself.tag >= TAG_basic)
                {
                    Value res(NULL_VALUE);
                    if (oldself.val.basic->GetSlot(engine->OpDispose_String, res))
                    {
                        Push(oldself);
                        Push(res);
                        
                        if (SetupCall(0))
                        {
                            Run();
                        }
                        Pop(); // result & exitwith string
                    }
                }
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_newpkg)
            {
                /* TODO { Right now a package w/o a dot-name overwrites any previous package. 
                          Meaning below the last foo overwrites the first:
                          package foo
                            bar = 3
                          end
                          
                          package foo
                            bazz = 4
                          end
                          
                          ...This is not a bug, we just need to decide if it's a feature.
                }
                *  
                * 
                * The stack:
                * [ ..... ] 
                * [name:String]
                * [super:Package]
                *                < sp
                */
                Value    super_pkg = PopTop();
                Value&   pkg_name  = Top();
                Package* super     = 0;
                
                // Packages must have a string name.
                if (!pkg_name.IsString())
                {
                    ReportRuntimeError(Exception::ERROR_runtime, 
                                       "Unable to create package: invalid name of type %s given.",                                            
                                       engine->GetTypenameOf(pkg_name)->GetBuffer());
                }
                
                // Here we determine the super package for the new package we are 
                // creating. If the package was declared with a dot or index 
                // expression we will use the left-hand operand if its a package. 
                // Otherwise we use the current package in this scope.
                
                super = (engine->Package_Type->IsInstance(super_pkg)) ?
                         super_pkg.val.package :
                         this->package;
                                
                if (super)
                {                    
                    Object* newpkg = 0;
                    newpkg = Package::Create(engine, pkg_name.val.str, super);
                    
                    if (newpkg)
                    {
                        Top().Set(newpkg);
                    }
                    else
                    {
                        ReportRuntimeError(Exception::ERROR_runtime, 
                                           "Unable to create package %s as child of package %s",
                                           pkg_name.val.str->GetBuffer(),
                                           super->GetDotName()->GetBuffer());
                    }
                }
                else
                {
                    // This only happens if there is no package for this scope.
                    // TODO { Is this even possible? }
                    ReportRuntimeError(Exception::ERROR_runtime, 
                                       "Unable to create package: invalid super package of type %s specified.",                                            
                                       engine->GetTypenameOf(super_pkg)->GetBuffer());
                }
                /*
                 * The stack:
                 * [ ..... ]
                 * [package]
                 *          < sp
                 */
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_pushpkg)
            {
                Value& the_env = PopTop();
                
                if ( !engine->Package_Type->IsInstance(the_env) )
                {
                    String* typeEnv = engine->GetTypenameOf(the_env);
                    ReportRuntimeError(Exception::ERROR_type,
                                       "Invalid package scope of type %s. Expected type %s.",
                                       typeEnv->GetBuffer(),
                                       engine->Package_Type->GetDotName()->GetBuffer());
                }
                Package* pkg = the_env.val.package;
                PushPackageScope();
                this->package = pkg;
            }
            PIKA_NEXT()
            
            PIKA_OPCODE(OP_poppkg)
            {
                PopPackageScope();
            }
            PIKA_NEXT()
            
            PIKA_END_DISPATCH()
        }
        catch (Exception& e)
        {
            if (inlineThrow)
            {
                throw;
            }
            else
            {
                switch (OpException(e))
                {
                case ER_throw:    throw;
                case ER_continue: continue;
                case ER_exit:     return;
                default:          throw;
                }
            }
        }
    }
    if (state == SUSPENDED)
        callsCount = numcalls > 0 ? numcalls : 1;
    --numRuns;
}
