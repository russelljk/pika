    
    DECL_OP( OP_nop,            "nop",          0, OF_zero,     "Does nothing." )
    DECL_OP( OP_dup,            "dup",          1, OF_none,     "Duplicates the value currently on top of the stack." )
    DECL_OP( OP_swap,           "swap",         1, OF_none,     "Swaps the two top-most values on the stack." )
    
    DECL_OP( OP_jump,           "jump",         3, OF_target,   "Jumps to a location in the byte-code." )
    DECL_OP( OP_jumpiffalse,    "jumpiffalse",  3, OF_target,   "Jumps to a location in the byte-code if the value on top of the stack is evaluates to false." )
    DECL_OP( OP_jumpiftrue,     "jumpiftrue",   3, OF_target,   "Jumps to a location in the byte-code if the value on top of the stack is evaluates to true." )
    
    DECL_OP( OP_pushnull,       "pushnull",     1, OF_none,     "Pushes null onto the stack." )
    DECL_OP( OP_pushself,       "pushself",     1, OF_none,     "Pushes the current self object onto the stack." )
    
    DECL_OP( OP_pushtrue,       "pushtrue",     1, OF_none,     "Pushes the value true onto the stack." )
    DECL_OP( OP_pushfalse,      "pushfalse",    1, OF_none,     "Pushes the value false onto the stack." )
    
    DECL_OP( OP_pushliteral0,   "pushliteral0", 1, OF_none,     "Pushes the literal at index 0 onto the stack." )
    DECL_OP( OP_pushliteral1,   "pushliteral1", 1, OF_none,     "Pushes the literal at index 1 onto the stack." )
    DECL_OP( OP_pushliteral2,   "pushliteral2", 1, OF_none,     "Pushes the literal at index 2 onto the stack." )
    DECL_OP( OP_pushliteral3,   "pushliteral3", 1, OF_none,     "Pushes the literal at index 3 onto the stack." )
    DECL_OP( OP_pushliteral4,   "pushliteral4", 1, OF_none,     "Pushes the literal at index 4 onto the stack." )
    DECL_OP( OP_pushliteral,    "pushliteral",  3, OF_w,        "Pushes the literal at a given index onto the stack." )
    
    DECL_OP( OP_pushlocal0,     "pushlocal0",   1, OF_none,     "Pushes the local variable at index 0 onto the stack." )
    DECL_OP( OP_pushlocal1,     "pushlocal1",   1, OF_none,     "Pushes the local variable at index 1 onto the stack." )
    DECL_OP( OP_pushlocal2,     "pushlocal2",   1, OF_none,     "Pushes the local variable at index 2 onto the stack." )
    DECL_OP( OP_pushlocal3,     "pushlocal3",   1, OF_none,     "Pushes the local variable at index 3 onto the stack." )
    DECL_OP( OP_pushlocal4,     "pushlocal4",   1, OF_none,     "Pushes the local variable at index 4 onto the stack." )
    DECL_OP( OP_pushlocal,      "pushlocal",    3, OF_w,        "Pushes a local variable at a given index onto the stack." )
    
    DECL_OP( OP_pushglobal,     "pushglobal",   3, OF_w,        "Pushes a global variable with a given name onto the stack." )
    DECL_OP( OP_pushmember,     "pushmember",   3, OF_w,        "Pushes a member variable with a given name onto the stack." )
    DECL_OP( OP_pushlexical,    "pushlexical",  4, OF_bw,       "Pushes a lexical variable at a given (depth, index) onto the stack." )
    
    DECL_OP( OP_setlocal0,      "setlocal0",    1, OF_none,     "Set the local variable at index 0 with the value currently on top of the stack." )
    DECL_OP( OP_setlocal1,      "setlocal1",    1, OF_none,     "Set the local variable at index 1 with the value currently on top of the stack." )
    DECL_OP( OP_setlocal2,      "setlocal2",    1, OF_none,     "Set the local variable at index 2 with the value currently on top of the stack." )
    DECL_OP( OP_setlocal3,      "setlocal3",    1, OF_none,     "Set the local variable at index 3 with the value currently on top of the stack." )
    DECL_OP( OP_setlocal4,      "setlocal4",    1, OF_none,     "Set the local variable at index 4 with the value currently on top of the stack." )
    DECL_OP( OP_setlocal,       "setlocal",     3, OF_w,        "Sets the local at a given index with the value on top of the stack." )
    
    DECL_OP( OP_setglobal,      "setglobal",    3, OF_w,        "Sets a global variable to the value on the top of the stack." )
    DECL_OP( OP_setmember,      "setmember",    3, OF_w,        "Sets a member variable to the value on the top of the stack" )
    DECL_OP( OP_setlexical,     "setlexical",   4, OF_bw,       "Sets a lexical variable at a given (depth, index) to the value on the top of the stack." )
    
    DECL_OP( OP_pop,            "pop",          1, OF_none,     "Pop the top value off the stack." )
    DECL_OP( OP_acc,            "acc",          1, OF_none,     "Pop the top value off the stack and set the acc to that value." )
    
    DECL_OP( OP_add,            "add",          1, OF_none,     "Add the top two values of the stack, pushing the result onto the stack." )
    DECL_OP( OP_sub,            "sub",          1, OF_none,     "Subtract the top two values of the stack, pushing the result onto the stack." )
    DECL_OP( OP_mul,            "mul",          1, OF_none,     "Multiply the top two values of the stack, pushing the result onto the stack." )
    DECL_OP( OP_div,            "div",          1, OF_none,     "Divide the top two values of the stack, pushing the result onto the stack." )
    DECL_OP( OP_idiv,           "idiv",         1, OF_none,     "Divide the top two values of the stack, pushing the result onto the stack as an integer." )
    DECL_OP( OP_mod,            "mod",          1, OF_none,     "Find the remainder of the top two values of the stack, and push the result onto the stack." )
    DECL_OP( OP_pow,            "pow",          1, OF_none,     "" )
    
    DECL_OP( OP_eq,             "eq",           1, OF_none,     "Test the equality of the top two values on the stack, pushing the result onto the stack." )
    DECL_OP( OP_ne,             "ne",           1, OF_none,     "Test the unequality of the top two values on the stack, pushing the result onto the stack." )
    
    DECL_OP( OP_lt,             "lt",           1, OF_none,     "" )
    DECL_OP( OP_gt,             "gt",           1, OF_none,     "" )
    DECL_OP( OP_lte,            "lte",          1, OF_none,     "" )
    DECL_OP( OP_gte,            "gte",          1, OF_none,     "" )
    
    DECL_OP( OP_bitand,         "bitand",       1, OF_none,     "" )
    DECL_OP( OP_bitor,          "bitor",        1, OF_none,     "" )
    DECL_OP( OP_bitxor,         "bitxor",       1, OF_none,     "" )
    
    DECL_OP( OP_xor,            "xor",          1, OF_none,     "" )
    
    DECL_OP( OP_lsh,            "lsh",          1, OF_none,     "" )
    DECL_OP( OP_rsh,            "rsh",          1, OF_none,     "" )
    DECL_OP( OP_ursh,           "ursh",         1, OF_none,     "" )
    
    DECL_OP( OP_not,            "not",          1, OF_none,     "" )
    DECL_OP( OP_bitnot,         "bitnot",       1, OF_none,     "" )
    DECL_OP( OP_inc,            "inc",          1, OF_none,     "" )
    DECL_OP( OP_dec,            "dec",          1, OF_none,     "" )
    DECL_OP( OP_pos,            "pos",          1, OF_none,     "" )
    DECL_OP( OP_neg,            "neg",          1, OF_none,     "" )
    
    DECL_OP( OP_catsp,          "catsp",        1, OF_none,     "" )
    DECL_OP( OP_cat,            "cat",          1, OF_none,     "" )
    
    DECL_OP( OP_bind,           "bind",         1, OF_none ,    "")
    
    DECL_OP( OP_unpack,         "unpack",       3, OF_w,        "" )
    
    DECL_OP( OP_tailapply,      "tailapply",    3, OF_bb,       "" )
    DECL_OP( OP_apply,          "apply",        3, OF_bbb,      "" )
    DECL_OP( OP_tailcall,       "tailcall",     3, OF_bb,       "" )
    DECL_OP( OP_call,           "call",         4, OF_bbb,      "" )
    
    DECL_OP( OP_dotget,         "dotget",       1, OF_none,     "" )
    DECL_OP( OP_dotset,         "dotset",       1, OF_none,     "" )
    
    DECL_OP( OP_subget,         "subget",       1, OF_none,     "" )
    DECL_OP( OP_subset,         "subset",       1, OF_none,     "" )
    
    DECL_OP( OP_locals,         "locals",       1, OF_none,     "" )
    
    DECL_OP( OP_method,         "method",       3, OF_w,        "" )
    
    DECL_OP( OP_property,       "property",     1, OF_none,     "" )
    
    DECL_OP( OP_subclass,       "subclass",     1, OF_none,     "" )
    
    DECL_OP( OP_array,          "array",        3, OF_w,        "" )
    DECL_OP( OP_dictionary,     "dictionary",   3, OF_w,        "" )
    DECL_OP( OP_super,          "super",        1, OF_none,     "" )
    DECL_OP( OP_forto,          "forto",        3, OF_w,        "" )

    DECL_OP( OP_compr,          "compr",        3, OF_w,        "" )    
    DECL_OP( OP_retacc,         "retacc",       1, OF_none,     "" )
    DECL_OP( OP_ret,            "ret",          1, OF_none,     "" )
    DECL_OP( OP_retv,           "retv",         3, OF_w,        "" )
    
    DECL_OP( OP_gennull,        "gennull",      1, OF_none,     "" )
    DECL_OP( OP_gen,            "gen",          1, OF_none,     "" )
    DECL_OP( OP_genv,           "genv",         3, OF_w,        "" )
    
    DECL_OP( OP_itercall,       "iterate",      4, OF_bw,       "" )
    
    DECL_OP( OP_foreach,        "foreach",      3, OF_w,        "" )
    
    DECL_OP( OP_same,           "same",         1, OF_none,     "" )
    DECL_OP( OP_notsame,        "notsame",      1, OF_none,     "" )
    DECL_OP( OP_is,             "is",           1, OF_none,     "" )
    DECL_OP( OP_has,            "has",          1, OF_none,     "" )
    
    DECL_OP( OP_pushtry,        "pushtry",      3, OF_target,   "" )
    DECL_OP( OP_pophandler,     "pophandler",   1, OF_none,     "" )

    DECL_OP( OP_raise,          "raise",        1, OF_none,     "" )
    
    DECL_OP( OP_retfinally,     "retfinally",    1, OF_none,     "" )
    DECL_OP( OP_callfinally,    "callfinally",   3, OF_target,   "" )
    
    DECL_OP( OP_pushwith,       "pushwith",     1, OF_none,     "" )
    DECL_OP( OP_popwith,        "popwith",      1, OF_none,     "" )
    
    DECL_OP( OP_newpkg,         "newpkg",       1, OF_none,     "" )
    DECL_OP( OP_pushpkg,        "pushenv",      1, OF_none,     "" )
    DECL_OP( OP_poppkg,         "popenv",       1, OF_none,     "" )

