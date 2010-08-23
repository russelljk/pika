/*
 *  PContext_Ops_Arith.inl
 *  See Copyright Notice in Pika.h
 */

PIKA_OPCODE(OP_add)  OpArithBinary(OP_add,  OVR_add,  OVR_add_r,  numcalls); PIKA_NEXT()
PIKA_OPCODE(OP_sub)  OpArithBinary(OP_sub,  OVR_sub,  OVR_sub_r,  numcalls); PIKA_NEXT()
PIKA_OPCODE(OP_mul)  OpArithBinary(OP_mul,  OVR_mul,  OVR_mul_r,  numcalls); PIKA_NEXT()
PIKA_OPCODE(OP_div)  OpArithBinary(OP_div,  OVR_div,  OVR_div_r,  numcalls); PIKA_NEXT()
PIKA_OPCODE(OP_idiv) OpArithBinary(OP_idiv, OVR_idiv, OVR_idiv_r, numcalls); PIKA_NEXT()
PIKA_OPCODE(OP_mod)  OpArithBinary(OP_mod,  OVR_mod,  OVR_mod_r,  numcalls); PIKA_NEXT()
PIKA_OPCODE(OP_pow)  OpArithBinary(OP_pow,  OVR_pow,  OVR_pow_r,  numcalls); PIKA_NEXT()

PIKA_OPCODE(OP_eq)
{
    Value& b = Top();
    Value& a = Top1();

    bool res = false;
    if (a.tag == TAG_object)
    {
        Object* obj = a.val.object;
        bool ovr_called = false;
        if (SetupOverrideLhs(obj, OVR_eq, &ovr_called))
        {
            ++numcalls;
        }

        if (ovr_called)
        {
            PIKA_NEXT()
        }
    }
    if (b.tag == TAG_object)
    {
        Object* obj = b.val.object;
        bool ovr_called = false;
        if (SetupOverrideRhs(obj, OVR_eq_r, &ovr_called))
        {
            ++numcalls;
        }

        if (ovr_called)
        {
            PIKA_NEXT()
        }
    }

    if (a.tag == TAG_integer)
    {
        if (b.tag == TAG_integer)
        {
            res = (a.val.integer == b.val.integer);
        }
        else if (b.tag == TAG_real)
        {
            res = (((preal_t)a.val.integer) == b.val.real);
        }
    }
    else if (a.tag == TAG_real)
    {
        if (b.tag == TAG_real)
        {
            res = (a.val.real == b.val.real);
        }
        else if (b.tag == TAG_integer)
        {
            res = (a.val.real == ((preal_t)b.val.integer));
        }
    }
    else if (a.tag == b.tag)
    {
        res = (a.val.index == b.val.index);
    }

    a.SetBool(res);
    Pop();
}
PIKA_NEXT()

PIKA_OPCODE(OP_ne)
{
    Value& b = Top();
    Value& a = Top1();

    bool res = true;

    if (a.tag == TAG_object)
    {
        Object* obj = a.val.object;

        bool ovr_called = false;
        if (SetupOverrideLhs(obj, OVR_ne, &ovr_called))
        {
            ++numcalls;
        }

        if (ovr_called)
        {
            PIKA_NEXT()
        }
    }
    if (b.tag == TAG_object)
    {
        Object* obj = b.val.object;

        bool ovr_called = false;
        if (SetupOverrideRhs(obj, OVR_ne_r, &ovr_called)) 
        {
            ++numcalls;
        }

        if (ovr_called) 
        {
            PIKA_NEXT()
        }
    }

    if (a.tag == TAG_integer) 
    {
        if (b.tag == TAG_integer) 
        {
            res = (a.val.integer != b.val.integer);
        }
        else if (b.tag == TAG_real) {
            res = (((preal_t)a.val.integer) != b.val.real);
        }
    }
    else if (a.tag == TAG_real) 
    {
        if (b.tag == TAG_real) 
        {
            res = (a.val.real != b.val.real);
        }
        else if (b.tag == TAG_integer) 
        {
            res = (a.val.real != ((preal_t)b.val.integer));
        }
    }
    else if (a.tag == b.tag) 
    {
        res = (a.val.index != b.val.index);
    }
    a.SetBool(res);
    Pop();
}
PIKA_NEXT()

PIKA_OPCODE(OP_lt)  OpCompBinary(OP_lt,  OVR_lt,  OVR_lt_r,  numcalls); PIKA_NEXT()
PIKA_OPCODE(OP_gt)  OpCompBinary(OP_gt,  OVR_gt,  OVR_gt_r,  numcalls); PIKA_NEXT()
PIKA_OPCODE(OP_lte) OpCompBinary(OP_lte, OVR_lte, OVR_lte_r, numcalls); PIKA_NEXT()
PIKA_OPCODE(OP_gte) OpCompBinary(OP_gte, OVR_gte, OVR_gte_r, numcalls); PIKA_NEXT()

PIKA_OPCODE(OP_bitand)  OpBitBinary(OP_bitand, OVR_bitand,  OVR_bitand_r,   numcalls); PIKA_NEXT()
PIKA_OPCODE(OP_bitor)   OpBitBinary(OP_bitor,  OVR_bitor,   OVR_bitor_r,    numcalls); PIKA_NEXT()
PIKA_OPCODE(OP_bitxor)  OpBitBinary(OP_bitxor, OVR_bitxor,  OVR_bitxor_r,   numcalls); PIKA_NEXT()

PIKA_OPCODE(OP_xor) 
{
    Value& b = Top();
    Value& a = Top1();

    Pop();
    a.val.index = engine->ToBoolean(this, a) != engine->ToBoolean(this, b);
    a.tag = TAG_boolean;
}
PIKA_NEXT()

// Left shift //////////////////////////////////////////////////////////////////////////////////////

PIKA_OPCODE(OP_lsh)   
    OpBitBinary(OP_lsh,  OVR_lsh,   OVR_lsh_r,    numcalls); 
PIKA_NEXT()

// Right shift /////////////////////////////////////////////////////////////////////////////////////

PIKA_OPCODE(OP_rsh)
    OpBitBinary(OP_rsh,  OVR_rsh,   OVR_rsh_r,    numcalls);
PIKA_NEXT()

// Unsigned right shift ////////////////////////////////////////////////////////////////////////////

PIKA_OPCODE(OP_ursh)
    OpBitBinary(OP_ursh, OVR_ursh,  OVR_ursh_r,   numcalls);
PIKA_NEXT()

// Boolean not /////////////////////////////////////////////////////////////////////////////////////

PIKA_OPCODE(OP_not) 
{
    Value& a = Top();
    OpOverride ovr = OVR_not;

    if (a.tag >= TAG_basic) 
    {
        Basic* bobj = a.val.basic;
        bool handled = false;

        if (SetupOverrideUnary(bobj, ovr, &handled)) 
        {
            ++numcalls;
        }

        if (handled) 
        {
            PIKA_NEXT()
        }
    }

    bool avalue = engine->ToBoolean(this, a);
    a.SetBool(!avalue);
}
PIKA_NEXT()

// Bitwise not /////////////////////////////////////////////////////////////////////////////////////

PIKA_OPCODE(OP_bitnot)
{
    Value& a = Top();
    OpOverride ovr = OVR_bitnot;

    if (a.tag == TAG_integer) 
    {
        a.val.integer = ~a.val.integer;
    }
    else if (a.tag >= TAG_object) 
    {
        Basic* bobj = a.val.basic;
        if (SetupOverrideUnary(bobj, ovr)) 
        {
            ++numcalls;
        }
    }
    else 
    {
        ReportRuntimeError(Exception::ERROR_runtime, "Operator %s not defined for type %s", engine->GetOverrideString(ovr)->GetBuffer(), engine->GetTypenameOf(a)->GetBuffer());
    }
}
PIKA_NEXT()

// Increment (prefix AND postfix) //////////////////////////////////////////////////////////////////

PIKA_OPCODE(OP_inc)
    OpArithUnary(OP_inc,  OVR_inc, numcalls);
PIKA_NEXT()

// Decrement (prefix AND postfix) //////////////////////////////////////////////////////////////////

PIKA_OPCODE(OP_dec)
    OpArithUnary(OP_dec,  OVR_dec, numcalls);
PIKA_NEXT()

// Prefix positive (+) /////////////////////////////////////////////////////////////////////////////

PIKA_OPCODE(OP_pos)
    OpArithUnary(OP_pos,  OVR_pos, numcalls);
PIKA_NEXT()

// Prefix negative (-) /////////////////////////////////////////////////////////////////////////////

PIKA_OPCODE(OP_neg) 
    OpArithUnary(OP_neg,  OVR_neg, numcalls);
PIKA_NEXT()

// Concat with space ///////////////////////////////////////////////////////////////////////////////

PIKA_OPCODE(OP_catsp) 
{
    if (OpCat(true)) 
    {
        ++numcalls;
    }
}
PIKA_NEXT()

// Concat //////////////////////////////////////////////////////////////////////////////////////////

PIKA_OPCODE(OP_cat) 
{
    if (OpCat(false)) 
    {
        ++numcalls;
    }
}
PIKA_NEXT()

PIKA_OPCODE(OP_bind) 
{
    if (OpBind())
    {
        ++numcalls;
    }
}
PIKA_NEXT()

PIKA_OPCODE(OP_unpack) 
{
    u2 expected = GetShortOperand(instr);
    if (OpUnpack(expected))
    {
        ++numcalls;
    }
}
PIKA_NEXT()
