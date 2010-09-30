/*
 *  PGenCode.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PAst.h"
#include "PInstruction.h"
#include "PError.h"
#include "PCompiler.h"
#include "PParser.h"

namespace pika {

// Start a new block at the Instr x.

#define PIKA_BLOCKSTART(cs, x)              \
    Instr* oldEndOfBlock = cs->endOfBlock;  \
    cs->endOfBlock = x

// End a block.

#define PIKA_BLOCKEND(cs)                   \
    cs->endOfBlock = oldEndOfBlock

namespace {

INLINE const char* GetSourceOf(CompileState* cs)
{
    return cs->parser->tstream.tokenizer->GetBuffer();
}

// Certain blocks like try blocks, with blocks and package blocks need to cleanup before you exit the block.
// This function finds all returns, breaks and continues that jump out of the block and inserts an opcode
// that will handle the cleanup.

void HandleBlockBreaks(
    Instr*  start,                // start of the block
    Instr*  end,                  // end of the block
    Opcode  breakcode,            // opcode to insert
    bool    handleReturns = true) // true if we need handle returns
{
    // handle jumps out of the try block
    
    // if we exit a try block early we need to pop the handler off the
    // exception-handler stack first
    
    for (Instr* curr = start; curr && curr != end; curr = curr->next)
    {
        bool insert_here = false;
        
        Opcode oc = curr->opcode;
        
        if (handleReturns)
        {
            insert_here = ((oc == BREAK_LOOP) || (oc == CONTINUE_LOOP) ||
                           (oc == OP_ret)     || (oc == OP_retacc)     ||
                           (oc == OP_retv)    || (oc == OP_tailcall));
        }
        else
        {
            insert_here = ((oc == BREAK_LOOP) || (oc == CONTINUE_LOOP));
        }
        
        if (insert_here)
        {
            Instr* ipoptryBeforeJmp = Instr::Create(breakcode);
            Instr* iprev = curr->prev;
            
            curr->prev = ipoptryBeforeJmp;
            ipoptryBeforeJmp->next = curr;
            ipoptryBeforeJmp->prev = iprev;
            
            if (iprev)
                iprev->next = ipoptryBeforeJmp;
        }
    }
}

// TODO: overload operator new for Instr
// TODO: use auto_ptr so we do not have to have try-catch blocks.
// TODO: make sure all Instr instances are released with operator delete.

void CompileFunction(int line, CompileState* state, Stmt* body, Def* def)
{
    Instr* ibody = 0;
    try
    {
        Def* old = state->currDef;
        
        // Generate code for function body.
        Instr* iretnull = Instr::Create(OP_retacc);
        PIKA_BLOCKSTART(state, iretnull);
        
        ibody = body->GenerateCode();
        
        PIKA_BLOCKEND(state);
        
        // Add an explicit return.
        
        ibody->Attach(iretnull);
        
        // Compile the function body.
        
        Compiler cc(state, ibody, def, def->literals);
        cc.DoCompile();
        
        ASSERT(def->numArgs <= def->numLocals);
        
        for (size_t i = 0; i < def->numArgs; ++i)
        {
            def->SetLocalRange(i, 0, iretnull->pos);
        }
        
        int maxstack = cc.GetStackLimit();
        if (maxstack < 0 || maxstack > PIKA_MAX_STACKLIMIT)
            state->SyntaxError(line, "Function's max stack size is bogus: %d\n", maxstack);
        def->stackLimit = (u2)Clamp<int>(maxstack, 0, PIKA_MAX_STACKLIMIT);
        def->SetBytecode(cc.GetBytecode(), cc.GetBytecodeLength());
        
        // Calculate debug line information.
        
        Instr* last = ibody;
        code_t* byte_code_start = def->GetBytecode();
        u2 currline = 0;
        
        while (last)
        {   
            /* If the instruction has a valid line number and it is not the same 
             * as the previous line number then add debug line info to the 
             * function definition. 
             */
            if (last->line != 0 && last->line != currline)
            {
                LineInfo li;
                li.line  = last->line;
                li.pos   = byte_code_start + last->pos;
                currline = li.line;
                def->lineInfo.Push(li);
            }
            last = last->next;
        }
        
        // Cleanup.
        
        Pika_delete(ibody);
        ibody = 0;
        state->currDef = old;
    }
    catch(...) 
    {
        // Cleanup in case of exception.
        if (ibody)
            Pika_delete(ibody);
        ibody = 0;
        throw; // re-throw the exception.
    }
}

/* Generate code that will short curcuit if the first operand evaluates to a 
 * certain boolean value. This is used for both the "and" and "or" boolean
 * operators.
 */
Instr* GenerateShortCircuitOp(Expr* exprA, Expr* exprB, bool isand)
{
    if (exprA->kind == Expr::EXPR_load)
    {
        LoadExpr* le = (LoadExpr*)exprA;
        LoadExpr::LoadKind lkind = le->loadkind;
        if (isand && ((lkind == LoadExpr::LK_false) || (lkind == LoadExpr::LK_null)))
            return exprA->GenerateCode();
        if (!isand && (lkind == LoadExpr::LK_true))
            return exprA->GenerateCode();
    }
    Instr* ia    = exprA->GenerateCode();
    Instr* ib    = exprB->GenerateCode();
    Instr* idup  = Instr::Create(OP_dup);
    Instr* icond = Instr::Create(isand ? OP_jumpiffalse : OP_jumpiftrue);
    Instr* itgt  = Instr::Create(JMP_TARGET);
    Instr* ipop  = Instr::Create(OP_pop);
    
    ia->
    Attach(idup)->
    Attach(icond)->
    Attach(ipop)->
    Attach(ib)->
    Attach(itgt);
    
    icond->SetTarget(itgt);
    
    return ia;
}

void FinallyBlockBreaks(Instr* start, Instr* end, Instr* target)
{
    for (Instr* curr = start; curr && (curr != end); curr = curr->next)
    {
        Opcode oc = curr->opcode;
        
        switch (oc)
        {
        case OP_retacc:
        case OP_ret:
        case OP_retv:
        case BREAK_LOOP:
        case CONTINUE_LOOP:
        {
            Instr* icallfinally = Instr::Create(OP_callfinally);
            Instr* ipopfinally  = Instr::Create(OP_pophandler);
            
            icallfinally->next = ipopfinally;
            ipopfinally->prev  = icallfinally;
            Instr* iprev       = curr->prev;
            ipopfinally->next  = curr;
            icallfinally->prev = iprev;
            curr->prev         = ipopfinally;
            
            icallfinally->SetTarget(target);
            
            if (iprev)
            {
                iprev->next = icallfinally;
            }
        }
        //  Fall through
        //      |
        //      V
        default: continue;
        }
    }
}
#if defined(ENABLE_SYNTAX_WARNINGS)
void WarnNonLocalJumps(CompileState* state, Instr* start, Instr* end, int line)
{
    for (Instr* curr = start; curr && (curr != end); curr = curr->next)
    {
        Opcode oc = curr->opcode;
        
        if (curr->line > line)
        {
            line = curr->line;
        }
        switch (oc)
        {
        case OP_retacc:
        case OP_ret:
        case OP_retv:
        case BREAK_LOOP:
        case CONTINUE_LOOP:
        {
            state->SyntaxWarning(WARN_severe, line, "jumping out of an finally block is not recommended.");
        }
        default: continue;
        }
    }
}
#endif
}// anonymous namespace

Instr* TreeNode::GenerateCode()
{
    SHOULD_NEVER_HAPPEN();
    return 0;
}

Instr* EmptyStmt::DoStmtCodeGen()
{
    Instr* inop = Instr::Create(OP_nop);
    return inop;
}

Instr* Program::GenerateCode()
{
    CompileFunction(line, state, stmts, def);
    
    if (scriptEnd >= scriptBeg)
    {
        def->SetSource(state->engine, GetSourceOf(state) + scriptBeg, scriptEnd - scriptBeg);
    }
    return 0;
}

Instr* NamedTarget::GenerateCodeSet()
{
    return name->GenerateCodeSet();
}

Instr* AnnotationDecl::GenerateCode()
{
    SHOULD_NEVER_HAPPEN();
    return 0;   
}

Instr* AnnotationDecl::GenerateCodeWith(Instr* subj)
{
    Instr* iname = name->GenerateCode();    
    Instr* iargs = next ? next->GenerateCodeWith(subj) : subj;
    Instr* iself = Instr::Create(OP_pushnull);
    // TODO: Check argc and keyword argc < MAX_ARG_COUNT
    
    // Start at 1 since we *always* have 1 extra argument, either the next annotation or the declaration.
    u2 argc = 1; 
    u2 kwargc = 0;
    ExprList* curr_arg = args;
    while (curr_arg)
    {
        Expr* expr = curr_arg->expr;
        if (expr->kind == Expr::EXPR_kwarg)
            ++kwargc;
        else
            ++argc;
        Instr* icurr_arg = expr->GenerateCode();        
        iargs->Attach(icurr_arg);
        curr_arg = curr_arg->next;
    }
    
    Instr* icall = Instr::Create(OP_call);
    icall->operand   = argc;
    icall->operandu1 = kwargc;
    icall->operandu2 = 1;
       
    
    iargs->
    Attach(iself)->
    Attach(iname)->
    Attach(icall);
    
    return iargs;
}

Instr* FunctionDecl::GenerateCode()
{
    Instr* fun = GenerateFunctionCode();
    
    Instr* annotations = GenerateAnnotationCode(fun);
    Instr* set = NamedTarget::GenerateCodeSet();
    
    annotations->
    Attach(set);
    return annotations;
}

Instr* FunctionDecl::GenerateFunctionCode()
{
    Instr* iinst = 0;
    Instr* idefs = 0;
    
    ParamDecl* arg = args;
    u2 numdefs = 0;
    
    while (arg)
    {
        if (arg->val)
        {
            if (!idefs)
                idefs = arg->val->GenerateCode();
            else
                idefs->Attach(arg->val->GenerateCode());
            ++numdefs;
        }
        arg = arg->next;
    }
    
    // push the function
    Instr* ifun = Instr::Create(OP_pushliteral);
    ifun->operand = index;
    
    iinst = Instr::Create(OP_method);
    iinst->operand = numdefs;
    
    // Generate the self object if its a dot expr
    bool isdotexpr = name->dotexpr ? true : false;
    
    if (isdotexpr)
    {
        Instr* iprep = name->dotexpr->left->GenerateCode(); // self
        Instr* idup = Instr::Create(OP_dup);
        iprep->Attach(idup);
        iprep->Attach(ifun);
        
        ifun = iprep;
    }
    else
    {
        Instr* ipushnull = Instr::Create(OP_pushnull);
        
        ipushnull->Attach(ifun);
        ifun = ipushnull;
    }
    
    if (idefs)
    {
        idefs->Attach(iinst);
        iinst = idefs;
    }
        
    
    
    ifun->
    Attach(iinst)
    ;
    try
    {
        CompileFunction(line, state, body, def);
        
        if (scriptEnd >= scriptBeg)
        {
            def->SetSource(state->engine,
                           GetSourceOf(state) + scriptBeg,
                           scriptEnd - scriptBeg);
        }
    }
    catch (Exception& e)
    {
        if (e.kind == Exception::ERROR_syntax)
        {
            // only handle compile errors
            state->SyntaxError(line, "compiling function: %s", name->GetName());
            return ifun;
        }
        else
        {
            // rethrow
            Pika_delete(ifun);
            throw;
        }
    }
       
    return ifun;
}

Instr* VarDecl::GenerateCode()
{
    return Instr::Create(OP_nop);
}

Instr* LocalDecl::GenerateCode()
{
    Instr* irep = 0;
    
    if (!newLocal)
    {
        irep = Instr::Create(OP_nop);
    }
    else
    {
        ASSERT(!symbol->isGlobal && !symbol->isWith);
        
        Instr* ipushnull = Instr::Create(OP_pushnull);
        Instr* iset      = Instr::Create(OP_setlocal);
        
        iset->operand   = symbol->offset;
        iset->operandu1 = 1;
        iset->target    = state->endOfBlock;
        
        ipushnull->Attach(iset);
        irep = ipushnull;
    }
    
    if (next)
    {
        Instr* inext = next->GenerateCode();
        irep->Attach(inext);
    }
    return irep;
}

Instr* CallExpr::GenerateCode()
{
    Expr::Kind k = left->kind;
    Instr* ilvalue = 0;
    Instr* iargs = 0;
    
    if (k == Expr::EXPR_dot)
    {
        DotExpr* de = static_cast<DotExpr*>(left);
        Instr* idup = Instr::Create(OP_dup); // duplicate 'self'
        Instr* idotget = Instr::Create(de->GetOpcode());
        Instr* dotleft = de->left->GenerateCode();
        Instr* dotright = de->right->GenerateCode();
        
        if (redirectedcall)
        {
            Instr* ipushthis = Instr::Create(OP_pushself);
            ipushthis->
            Attach(dotleft)->
            Attach(dotright)->
            Attach(idotget)
            ;
            ilvalue = ipushthis;
        }
        else
        {
            dotleft->
            Attach(idup)->
            Attach(dotright)->
            Attach(idotget)
            ;
            ilvalue = dotleft;
        }
    }
    else
    {
        ilvalue = left->GenerateCode();
        Instr* ipushthis = Instr::Create(redirectedcall ? OP_pushself : OP_pushnull);        
        ipushthis->Attach(ilvalue);
        ilvalue = ipushthis;
    }
    
    ExprList* curr = args;
        
    while (curr)
    {
        if (curr->expr->kind == Expr::EXPR_kwarg    ||
            curr->expr->kind == Expr::EXPR_apply_va ||
            curr->expr->kind == Expr::EXPR_apply_kw )
            break;
    
        Expr* expr = curr->expr;
        Instr* iexpr = expr->GenerateCode();
        
        if (iargs)
            iargs->Attach(iexpr);
        else
            iargs = iexpr;
        
        curr = curr->next;
    }
        
    while (curr)
    {       
        Expr* expr = curr->expr;
        Instr* iexpr = expr->GenerateCode();
        
        if (iargs)
            iargs->Attach(iexpr);
        else
            iargs = iexpr;
        
        curr = curr->next;
    }
        
    Instr* icall = Instr::Create(is_apply ? OP_apply : OP_call);
    icall->operand   = argc;
    icall->operandu1 = kwargc;
    icall->operandu2 = retc ? retc : 1;
    if (iargs)
    {
        iargs->Attach(ilvalue);
        ilvalue->Attach(icall);
        return iargs;
    }
    
    ilvalue->Attach(icall);
    return ilvalue;
}

Instr* BinaryExpr::GenerateCode()
{
    if (kind == EXPR_and)
    {
        return GenerateShortCircuitOp(left, right, true);
    }
    else if (kind == EXPR_or)
    {
        return GenerateShortCircuitOp(left, right, false);
    }
    else
    {
        Opcode op = GetOpcode();
        Instr* iop = Instr::Create(op);
        Instr* ileft = left->GenerateCode();
        Instr* iright = right->GenerateCode();
        
        ileft->Attach(iright);
        iright->Attach(iop);
        
        return ileft;
    }
}

Instr* UnaryExpr::GenerateCode()
{
    Opcode op    = GetOpcode();
    Instr* iexpr = expr->GenerateCode();
    Instr* iop   = Instr::Create(op);
    
    if (kind == EXPR_preincr || kind == EXPR_postincr || 
        kind == EXPR_predecr || kind == EXPR_postdecr)
    {
        Expr::Kind k = expr->kind;
        Expr* exprres = expr;
        Instr* idup = Instr::Create(OP_dup);
        
        if (kind == EXPR_preincr || kind == EXPR_predecr)
        {
            // prefix
            iexpr->Attach(iop)->Attach(idup);
        }
        else
        {
            // postfix
            iexpr->Attach(idup)->Attach(iop);
        }
        
        if (k == EXPR_dot)
        {
            DotExpr *leftdot = (DotExpr*)exprres;
            Instr* dotset = leftdot->GenerateCodeSet();
            
            iexpr->Attach(dotset);
            return iexpr;
        }
        else if (k == EXPR_identifier)
        {
            IdExpr *idexpr = (IdExpr *)exprres;
            Instr* setleft = idexpr->GenerateCodeSet();
            
            iexpr->Attach(setleft);
            return iexpr;
        }
        else
        {
            SHOULD_NEVER_HAPPEN();
        }
    }
    
    iexpr->Attach(iop);
    return iexpr;
}

Instr* IdExpr::GenerateCode()
{
    Opcode oc = OP_nop;
    Instr* irep = 0;
    
    if (IsOuter())
    {
        oc = OP_pushlexical;
    }
    else if (IsLocal())
    {
        index = symbol->offset;
        
        if (index < PIKA_NUM_SPECIALIZED_OPCODES)
        {
            oc = (Opcode)(OP_pushlocal0 + index);
        }
        else
        {
            oc = OP_pushlocal;
        }
    }
    else if (IsWithGet())
    {
        //  We only treat this as a member if the symbol was declared in the
        //  same block as the identifier.
        
        if (!outerwith)
        {
            index = state->AddConstant(id->name);
            oc = OP_pushmember;
        }
        else
        {
            index = state->AddConstant(id->name);
            oc = OP_pushglobal;
        }
    }
    else if (IsGlobal())
    {
        index = state->AddConstant(id->name);
        oc = OP_pushglobal;
    }
    
    irep = Instr::Create(oc);
    irep->operand = index;
    irep->operandu1 = depthindex;
    return irep;
}

Instr* IdExpr::GenerateCodeSet()
{
    Opcode oc = OP_nop;
    Instr* irep = 0;
    
    if (IsOuter())
    {
        oc = OP_setlexical;
    }
    else if (IsLocal())
    {
        index = symbol->offset;
        
        if (index < PIKA_NUM_SPECIALIZED_OPCODES)
        {
            oc = (Opcode)(OP_setlocal0 + index);
        }
        else
        {
            oc = OP_setlocal;
        }
    }
    else if (IsWithSet())
    {
        if (!outerwith)
        {
            index = state->AddConstant(id->name);
            oc = OP_setmember;
        }
        else
        {
            index = state->AddConstant(id->name);
            oc = OP_setglobal;
        }
    }
    else if (IsGlobal())
    {
        index = state->AddConstant(id->name);
        oc = OP_setglobal;
    }
    
    PIKA_NEW(Instr, irep, (oc));
    
    irep->operand = index;
    irep->operandu1 = depthindex;
    
    if (newLocal && (irep->opcode >= OP_setlocal0 && irep->opcode <= OP_setlocal))
    {
        irep->operandu1 = 1;
        irep->target = state->endOfBlock;
    }
    return irep;
}

Instr* ConstantExpr::GenerateCode()
{
    Instr* iloadc = Instr::Create(OP_pushliteral);
    
    if (index < PIKA_NUM_SPECIALIZED_OPCODES)
    {
        iloadc->opcode = (Opcode)(OP_pushliteral0 + index);
    }
    iloadc->operand = index;
    return iloadc;
}

Instr* Stmt::GenerateCode()
{
    Instr* ir = DoStmtCodeGen();
    
    if (newline && ir)
    {
        ir->line = line;
    }
    return ir;
}

Instr* TryStmt::DoStmtCodeGen()
{
    /*      [ OP_pushtry    ]
     *      [ try block     ]
     *      [ OP_pophandler ]
     *      [ OP_jump       ]-----.
     *  .---[ catch block + ]     |
     *  |   [ JMP_TARGET    ] <---'    
     *  |   [ else block  ? ]
     *  '-> [ JMP_TARGET    ]  
     */
    Instr* ipushtry   = Instr::Create(OP_pushtry);
    Instr* ipoptry    = Instr::Create(OP_pophandler);
    Instr* ifinishtry = Instr::Create(OP_jump);
    Instr* ijmptarget = Instr::Create(JMP_TARGET);
    Instr* ijmptarget2 = Instr::Create(JMP_TARGET);
    PIKA_BLOCKSTART(state, ifinishtry);
    Instr* itry = tryBlock->GenerateCode();
    PIKA_BLOCKEND(state);
    
    ipushtry->
    Attach(itry)->
    Attach(ipoptry)->
    Attach(ifinishtry);
    
    ifinishtry->SetTarget(ijmptarget2);
    
    ipushtry->operand = 0;
    ipoptry->operand  = 0;
    bool settarget    = false;
    Instr* icurr      = ipoptry;
    Instr* prevtarget = 0;
    
    // catch ex is type
    // ...
    //
    // 'catch is' blocks must come first, before the 'catch all' and 'else' blocks.
    if (catchis)
    {
        CatchIsBlock* curr = catchis;
        while (curr)
        {
            Instr* icatchtarget = Instr::Create(JMP_TARGET);
            PIKA_BLOCKSTART(state, icatchtarget);
            
            Instr* idup        = Instr::Create(OP_dup);
            Instr* ijump       = Instr::Create(OP_jump);
            Instr* ijumpf      = Instr::Create(OP_jumpiffalse);
            Instr* iopis       = Instr::Create(OP_is);
            Instr* icatch      = curr->block->GenerateCode();
            Instr* iis         = curr->isexpr->GenerateCode();
            Instr* iassign     = Instr::Create(OP_setlocal); // set catch var
            iassign->operandu1 = 1;
            iassign->target    = ijmptarget;
            Instr* ireraise    = (curr->next) ? Instr::Create(OP_nop) : Instr::Create(OP_raise);
            
            PIKA_BLOCKEND(state);
            
            iassign->operand = curr->symbol->offset;
            
            icurr->
            Attach(idup)->
            Attach(iis)->
            Attach(iopis)->
            Attach(ijumpf)->
            Attach(iassign)->
            Attach(icatch)->
            Attach(ijump)->
            Attach(ireraise)->
            Attach(icatchtarget)
            ;
            ijumpf->target = icatchtarget;
            ijump->target  = ijmptarget;
            
            if (!curr->next && ijmptarget)
            {
                if (catchBlock)
                    prevtarget = ijumpf;
                else
                    ijumpf->target = ireraise;
            }
            
            if (!settarget)
            {
                settarget = true;
                ipushtry->SetTarget(idup);
            }
            
            curr = curr->next;
            
            HandleBlockBreaks(idup, ijump, OP_pop);
        }
    }
    
    // generic catch all block: catch ex
    if (catchBlock)
    {
        PIKA_BLOCKSTART(state, ijmptarget); // Start a local block
        
        Instr* icatch = catchBlock->GenerateCode();
        
        // Set the catch variable.
        Instr* iassign     = Instr::Create(OP_setlocal);
        iassign->operandu1 = 1;          // Tell the compiler that we know our range.
        iassign->target    = ijmptarget; // Set the last instruction of the block.
        
        PIKA_BLOCKEND(state);               // End a local block
        
        icurr->
        Attach(iassign)->
        Attach(icatch)
        ;
        if (!settarget)
        {
            settarget = true;
            ipushtry->SetTarget(iassign);
        }
        if (prevtarget)
            prevtarget->target = iassign;
            
        iassign->operand = symbol->offset;
    }
    
    // Handle the else block that will executed only if no exception is raised/caught.
    if (elseblock)
    {
        Instr* iejump = Instr::Create(OP_jump);
        Instr* ielse = elseblock->GenerateCode();
        
        ipushtry->Attach(iejump)->
        Attach(ijmptarget2)->
        Attach(ielse)->
        Attach(ijmptarget)
        ;
        iejump->SetTarget(ijmptarget);
    }
    else
    {
        ipushtry->Attach(ijmptarget2)->Attach( ijmptarget );
    }
    
    HandleBlockBreaks(ipushtry, ipoptry, OP_pophandler);
    
    return ipushtry;
}

Instr* RaiseStmt::DoStmtCodeGen()
{
    Instr* iexpr  = 0;
    Instr* ithrow = Instr::Create(OP_raise);
    
    if (!expr)
    {
        // no expression given so we reraise the current exception.
        iexpr = Instr::Create(OP_pushlocal);
        iexpr->operand = reraiseOffset;
    }
    else
    {
        iexpr = expr->GenerateCode();
    }
    
    iexpr->Attach(ithrow);
    return iexpr;
}

Instr* StmtList::DoStmtCodeGen()
{
    Instr* ifirst  = first->GenerateCode();
    Instr* isecond = second->GenerateCode();
    
    ifirst->Attach(isecond);
    return ifirst;
}

Instr* LabeledStmt::DoStmtCodeGen()
{
    return stmt->GenerateCode();
}

static bool MakeTailcall(bool intry, Instr* icurr)
{
    if (intry)
        return false;
    // Find the last instruction in this list.
    while (icurr && icurr->next)
        icurr = icurr->next;
        
    // If its a valid call opcode
    if (icurr && icurr->opcode == OP_call)
    {
        // Make it a tail call.
        icurr->opcode = OP_tailcall;
        return true;
    }
    return false;
}

Instr* RetStmt::DoStmtCodeGen()
{
    if (!exprs || count == 0)
    {
        Instr* iretnull = Instr::Create(OP_retacc);
        return iretnull;
    }
    
    Instr* iexpr = 0;
    Instr* last  = 0;
    for (ExprList* curr = exprs; curr != 0; curr = curr->next)
    {
        ASSERT(curr->expr);
        
        Instr* inext = curr->expr->GenerateCode();
        if (!iexpr)
            iexpr = inext;
        else
            iexpr->Attach(inext);
            
        last = iexpr;
    }
    
    if (count == 1)
    {
        ASSERT(iexpr);
        
        if (MakeTailcall(isInTry, last))
            return iexpr;
            
        Instr* iret  = Instr::Create(OP_ret);
        iexpr->Attach(iret);
        return iexpr;
    }
    else
    {
        Instr* iret  = Instr::Create(OP_retv);
        iret->operand = count;
        iexpr->Attach(iret);
        return iexpr;
    }
}

Instr* YieldStmt::DoStmtCodeGen()
{
    if (!exprs || count == 0)
    {
        Instr* iyieldnull = Instr::Create(OP_yieldnull);
        return iyieldnull;
    }
    
    Instr* iexpr = 0;
    Instr* last  = 0;
    for (ExprList* curr = exprs; curr != 0; curr = curr->next)
    {
        ASSERT(curr->expr);
        
        Instr* inext = curr->expr->GenerateCode();
        if (!iexpr)
            iexpr = inext;
        else
            iexpr->Attach(inext);
            
        last = iexpr;
    }
    
    if (count == 1)
    {
        Instr* iyield  = Instr::Create(OP_yield);
        iexpr->Attach(iyield);
        return iexpr;
    }
    else
    {
        Instr* iyield  = Instr::Create(OP_yieldv);
        iyield->operand = count;
        iexpr->Attach(iyield);
        return iexpr;
    }
}

Instr* ExprStmt::DoStmtCodeGen()
{
    ExprList* curr = exprList;
    Instr* ifirst = 0;
    
    while (curr)
    {
        Expr*   expr = curr->expr;
        Instr* iexpr = expr->GenerateCode();
        if (autopop)
        {
            Instr* ipop  = Instr::Create(OP_acc);
            iexpr->Attach(ipop);
        }
        if (ifirst)
        {
            ifirst->Attach(iexpr);
        }
        else
        {
            ifirst = iexpr;
        }
        curr = curr->next;
    }
    return ifirst;
}

Instr* DeclStmt::DoStmtCodeGen()
{
    return decl->GenerateCode();
}

Instr* BlockStmt::DoStmtCodeGen()
{
    Instr* iend = Instr::Create(JMP_TARGET);
    
    PIKA_BLOCKSTART(state, iend);
    
    Instr* iblock = stmts->GenerateCode();
    
    PIKA_BLOCKEND(state);
    
    iblock->Attach(iend);
    return  iblock;
}

Instr* IfStmt::DoStmtCodeGen()
{
    Instr* icond      = cond->GenerateCode();
    Instr* ijmpFalse  = (is_unless) ? Instr::Create(OP_jumpiftrue) : Instr::Create(OP_jumpiffalse) ;
    Instr* ithen      = then_part->GenerateCode();
    Instr* ijmpTarget = Instr::Create(JMP_TARGET);
    
    icond->Attach(ijmpFalse);
    ijmpFalse->Attach(ithen);
    ithen->Attach(ijmpTarget);
    
    ijmpFalse->SetTarget(ijmpTarget);
    
    return icond;
}

Instr* LoopStmt::DoStmtCodeGen()
{
    Instr* ibody      = body->GenerateCode();
    Instr* ijmpTarget = Instr::Create(JMP_TARGET);
    Instr* ijmpBack   = Instr::Create(OP_jump);
    
    ibody->
    Attach(ijmpBack)->
    Attach(ijmpTarget);
    
    ijmpBack->SetTarget(ibody);
    
    ibody->DoLoopPatch(ijmpTarget, ibody, label);
    
    return ibody;
}

Instr* ForToStmt::DoStmtCodeGen()
{
    Instr* ijmpTarget = Instr::Create(JMP_TARGET);
    PIKA_BLOCKSTART(state, ijmpTarget);
    
    Instr* ifrom      = from->GenerateCode();
    Instr* ito        = to->GenerateCode();
    Instr* istep      = step->GenerateCode();
    Instr* iforto     = Instr::Create(OP_forto);
    Instr* ibody      = body->GenerateCode();
    Instr* ijmp       = Instr::Create(OP_jump);
    Instr* ijmpFalse  = Instr::Create(OP_jumpiffalse);
    Instr* ipushlvar1 = Instr::Create(OP_pushlocal);
    Instr* ipushlvar2 = Instr::Create(OP_pushlocal);
    Instr* ipushto    = Instr::Create(OP_pushlocal);
    Instr* ipushstep  = Instr::Create(OP_pushlocal);
    Instr* iless      = Instr::Create(down ? OP_gt : OP_lt);
    Instr* iadd       = Instr::Create(OP_add);
    Instr* iset       = Instr::Create(OP_setlocal);
    
    PIKA_BLOCKEND(state);
    
    iforto->target = ijmpTarget;
    
    // Any changes to the order and the value of PIKA_FORTO_COMP_OFFSET (defined in "gOpcode.h")
    // needs to be changed as well.
    
    ifrom ->
    Attach(ito) ->
    Attach(istep) ->
    Attach(iforto) ->
    Attach(ipushlvar1)->    // 0
    Attach(ipushto) ->      // 1
    Attach(iless) ->        // 2 = PIKA_FORTO_COMP_OFFSET
    Attach(ijmpFalse) ->
    Attach(ibody) ->
    Attach(ipushlvar2)->
    Attach(ipushstep) ->
    Attach(iadd) ->
    Attach(iset) ->
    Attach(ijmp) ->
    Attach(ijmpTarget);
    
    ijmp->SetTarget(ipushlvar1);
    ijmpFalse->SetTarget(ijmpTarget);
    
    ibody->DoLoopPatch(ijmpTarget,
                       ipushlvar2,
                       this->label);
                       
    iset->operand       = symbol->offset;
    ipushlvar1->operand = symbol->offset;
    ipushlvar2->operand = symbol->offset;
    ipushto->operand    = symbol->offset + 1;
    ipushstep->operand  = symbol->offset + 2;
    iforto->operand     = symbol->offset;
    
    return ifrom;
}

Instr* ForeachStmt::DoStmtCodeGen()
{
    Instr* ijmpTarget = Instr::Create(JMP_TARGET);
    PIKA_BLOCKSTART(state, ijmpTarget);
    
    Instr* ivar       = iterVar->GenerateCode();
    Instr* iin        = in->GenerateCode();
    Instr* ibody      = body->GenerateCode();
    Instr* itype      = type_expr->GenerateCode();
    Instr* inewenum   = Instr::Create(OP_foreach);
    Instr* ienumvalid = Instr::Create(OP_enumisvalid);
    Instr* ijmpfalse  = Instr::Create(OP_jumpiffalse);
    Instr* ienumadv   = Instr::Create(OP_enumadvance);
    Instr* iset       = idexpr->GenerateCodeSet();
    Instr* ijmpback   = Instr::Create(OP_jumpiftrue);
    
    PIKA_BLOCKEND(state);
    
    ivar->
    Attach(iin)->
    Attach(itype)->
    Attach(inewenum)->      // create a new enumerator
    Attach(ijmpfalse)->
    Attach(ienumadv)->      // getcurrent
    Attach(iset)->
    Attach(ibody)->
    Attach(ienumvalid)->
    Attach(ijmpback)->
    Attach(ijmpTarget);
    
    ibody->DoLoopPatch(ijmpTarget,
                       ienumvalid,
                       this->label);
                       
    ijmpback->SetTarget(ienumadv);
    ijmpfalse->SetTarget(ijmpTarget);
    
    inewenum->operand = enum_offset;
    ienumvalid->operand = enum_offset;
    ienumadv->operand = enum_offset;
    return ivar;
}

Instr* CondLoopStmt::DoStmtCodeGen()
{
    Instr* icond    = cond->GenerateCode();
    Instr* ibody    = body->GenerateCode();
    Instr* ijmpCond = (until) ? Instr::Create(OP_jumpiftrue) : Instr::Create(OP_jumpiffalse);
    Instr* itarget  = Instr::Create(JMP_TARGET);
    Instr* ijmpBack = Instr::Create(OP_jump);
    
    if (repeat)
    {
        /* The body is executed at least once before the condition (while/until) is checked.
         *
         *       body <-----------.
         *       cond             |
         * .---- jmpCond          |
         * |     jmpBack ---------'
         * '---> target
         */
        ibody->Attach(icond);
        icond->Attach(ijmpCond);
        ijmpCond->Attach(ijmpBack);
        ijmpBack->Attach(itarget);
        
        ijmpCond->SetTarget(itarget);
        ijmpBack->SetTarget(ibody);
        
        ibody->DoLoopPatch(itarget, ibody, label);
        return ibody;
    }
    else
    {
        /* The body is executed while/until the condition is true.
         *
         *       cond <-----------.
         * .---- jmpCond          |
         * |     body             |
         * |     jmpBack ---------'
         * '---> target
         */
        icond->Attach(ijmpCond);
        ijmpCond->Attach(ibody);
        ibody->Attach(ijmpBack);
        ijmpBack->Attach(itarget);
        
        ijmpCond->SetTarget(itarget);
        ijmpBack->SetTarget(icond);
        
        ibody->DoLoopPatch(itarget, icond, label);
        return icond;
    }
}

Instr* ConditionalStmt::DoStmtCodeGen()
{
    bool bElse = elsePart != 0;
    
    Instr*  itarget  = Instr::Create(JMP_TARGET);
    Instr*  istart   = 0;
    IfStmt* currIf   = ifPart;
    Instr*  iprev    = 0;
    Instr*  ijmpprev = 0;
    
    while (currIf)
    {
        Instr* icond = currIf->cond->GenerateCode();
        icond->line = currIf->cond->line;
        
        Instr* ijmpfalse = (currIf->is_unless) ? Instr::Create(OP_jumpiftrue) : Instr::Create(OP_jumpiffalse) ;
        Instr* ithen     = currIf->then_part->GenerateCode();
        Instr* ijmp      = (bElse || currIf->next) ? Instr::Create(OP_jump) : 0;
        
        if (iprev)
        {
            iprev->Attach(icond);
        }
        
        icond->Attach(ijmpfalse);
        ijmpfalse->Attach(ithen);
        
        if (ijmp)
        {
            ithen->Attach(ijmp);
            ijmp->SetTarget(itarget);
        }
        
        if (ijmpprev)
        {
            ijmpprev->SetTarget(icond);
        }
        
        ijmpprev = ijmpfalse;
        iprev = icond;
        
        if (currIf == ifPart)
        {
            istart = icond;
        }
        
        currIf = (IfStmt*)currIf->next;
    }
    
    if (bElse)
    {
        Instr* ielse = elsePart->GenerateCode();
        ielse->line = elsePart->line;
        iprev->Attach(ielse);
        
        if (ijmpprev)
        {
            ijmpprev->SetTarget(ielse);
        }
    }
    else if (ijmpprev)
    {
        ijmpprev->SetTarget(itarget);
    }
    
    istart->Attach(itarget);
    return istart;
}

Instr* LoadExpr::GenerateCode()
{
    switch (loadkind)
    {
    case LK_super:      return Instr::Create(OP_super);
    case LK_self:       return Instr::Create(OP_pushself);
    case LK_null:       return Instr::Create(OP_pushnull);
    case LK_true:       return Instr::Create(OP_pushtrue);
    case LK_false:      return Instr::Create(OP_pushfalse);
    case LK_locals:     return Instr::Create(OP_locals);

    }
    return Instr::Create(OP_nop);
}

Instr* DotExpr::GenerateCode()
{
    if (left->kind  == Expr::EXPR_load && (((LoadExpr*)left)->loadkind == LoadExpr::LK_self) &&
            right->kind == Expr::EXPR_member)
    {
        // We can optimize a self.field[] expression so that we do not have to perform
        // a OP_pushself and OP_pushliteral
        
        MemberExpr* m = (MemberExpr*)right;
        Instr* igetmember = Instr::Create(OP_pushmember);
        igetmember->operand = m->index;
        return igetmember;
    }
    else
    {
        Instr* iop    = Instr::Create(OP_dotget);
        Instr* ileft  = left->GenerateCode();
        Instr* iright = right->GenerateCode();
        
        ileft->
        Attach(iright)->
        Attach(iop);
        
        return ileft;
    }
}

Instr* DotExpr::GenerateCodeSet()
{
    if (left->kind  == Expr::EXPR_load && (((LoadExpr*)left)->loadkind == LoadExpr::LK_self) &&
            right->kind == Expr::EXPR_member)
    {
        // We can optimize a self.field[.field] = expr, expression so that we do not have to perform
        // a OP_pushself and OP_pushliteral
        
        MemberExpr* m = (MemberExpr*)right;
        Instr* isetmember = Instr::Create(OP_setmember);
        isetmember->operand = m->index;
        return isetmember;
    }
    else
    {
        Instr* iop = Instr::Create(OP_dotset);
        Instr* ileft = left->GenerateCode();
        Instr* iright = right->GenerateCode();
        
        ileft->Attach(iright);
        iright->Attach(iop);
        
        return ileft;
    }
}

Instr* IndexExpr::GenerateCode()
{ 
    Instr* iop    = Instr::Create(OP_subget);
    Instr* ileft  = left->GenerateCode();
    Instr* iright = right->GenerateCode();
    
    ileft->
    Attach(iright)->
    Attach(iop);
    
    return ileft;
}

Instr* IndexExpr::GenerateCodeSet()
{ 
        Instr* iop = Instr::Create(OP_subset);
        Instr* ileft = left->GenerateCode();
        Instr* iright = right->GenerateCode();
        
        ileft->Attach(iright);
        iright->Attach(iop);
        
        return ileft; 
}

Instr* DotBindExpr::GenerateCode()
{
    Instr* iop    = Instr::Create(GetOpcode()); // TODO: OP_subget
    Instr* ileft  = left->GenerateCode();
    Instr* idup   = Instr::Create(OP_dup);
    Instr* iright = right->GenerateCode();
    Instr* ibind  = Instr::Create(OP_bind);
    Instr* iswap  = Instr::Create(OP_swap);
    
    ileft->
    Attach(idup)->
    Attach(iright)->
    Attach(iop)->
    Attach(iswap)->
    Attach(ibind);
    
    return ileft;
}

Instr* DotBindExpr::GenerateCodeSet()
{
    Instr* iop    = Instr::Create(SetOpcode()); // TODO: OP_subset
    Instr* ileft  = left->GenerateCode();
    Instr* idup   = Instr::Create(OP_dup);
    Instr* iright = right->GenerateCode();
    Instr* ibind  = Instr::Create(OP_bind);
    Instr* iswap  = Instr::Create(OP_swap);
    
    ileft->
    Attach(idup)->
    Attach(iright)->
    Attach(iop)->
    Attach(iswap)->
    Attach(ibind);
    
    return ileft;
}

Instr* PropExpr::GenerateCode()
{
    Instr* iprop = Instr::Create(OP_property);
    
    Instr* iname   = (nameexpr) ? nameexpr->GenerateCode() : Instr::Create(OP_pushnull);
    Instr* igetter = (getter)   ? getter->GenerateCode()   : Instr::Create(OP_pushnull);
    Instr* isetter = (setter)   ? setter->GenerateCode()   : Instr::Create(OP_pushnull);
    
    iname->line   = (nameexpr) ? nameexpr->line : line;
    isetter->line = (setter)   ? setter->line    : line;
    igetter->line = (getter)   ? getter->line    : line;
    
    if (isetter->line >= igetter->line)
    {
        iname->Attach(igetter);
        igetter->Attach(isetter);
        isetter->Attach(iprop);
    }
    else
    {
        //
        // we output in the order specified by the user
        // then swap the getter and setter before creating the
        // property.
        //
        // This ensures that LineInfo order is preserved, expressions are evaluated inorder and
        // OP_property recieves its name | getter | setter in the correct order.
        //
        Instr* iswap = Instr::Create(OP_swap);
        iname->Attach(isetter);
        isetter->Attach(igetter);
        igetter->Attach(iswap);
        iswap->Attach(iprop);
    }
    return iname;
}

Instr* FunExpr::GenerateCode()
{
    u2 numdefs = 0;
    ParamDecl* arg = args;
    Instr* idefs = 0;
    
    while (arg)
    {
        if (arg->val)
        {
            if (!idefs) idefs = arg->val->GenerateCode();
            else idefs->Attach(arg->val->GenerateCode());
            
            ++numdefs;
        }
        arg = arg->next;
    }
    
    Instr* iinst = Instr::Create(OP_method);
    iinst->operand = numdefs;
    
    if (idefs)
    {
        idefs->Attach(iinst);
        iinst = idefs;
    }
    
    Instr* ifun = ConstantExpr::GenerateCode(); // push the function
    ifun->Attach(iinst);
    Instr* ipushnull = Instr::Create(OP_pushnull);
    ipushnull->Attach(ifun);
    ifun = ipushnull;
    try
    {
        CompileFunction(line, state, body, def);
        
        if (this->scriptEnd >= this->scriptBeg)
        {
            def->SetSource(state->engine, GetSourceOf(state) + this->scriptBeg, this->scriptEnd - this->scriptBeg);
        }
    }
    catch (Exception& e)
    {
        if (e.kind == Exception::ERROR_syntax)
        {
            // we only handle Compile errors
            state->SyntaxError(line, "compiling anonymous function");
            return ifun;
        }
        else
        {
            Pika_delete(ifun);
            throw;
        }
    }
    return ifun;
}

Instr* BreakStmt::DoStmtCodeGen()
{
    Instr* ir = Instr::Create(BREAK_LOOP);
    ir->symbol = label;
    return ir;
}

Instr* ContinueStmt::DoStmtCodeGen()
{
    Instr* ir = Instr::Create(CONTINUE_LOOP);
    ir->symbol = label;
    return ir;
}

Instr* DictionaryExpr::GenerateCode()
{
    Instr*     inewObj = Instr::Create(OP_objectliteral);
    FieldList* curr    = fields;
    Instr*     icurr   = 0;
    Instr*     ifirst  = 0;
    u2         count   = 0;
    Instr*     itype   = type_expr ? type_expr->GenerateCode() : Instr::Create(OP_pushnull);
    itype->Attach(inewObj);
    
    while (curr)
    {
        if (curr->value == type_expr)
        {
            curr = curr->next;
            continue;
        }
        Instr* ival  = curr->value->GenerateCode();
        Instr* iprop = (curr->name) ? curr->name->GenerateCode() : Instr::Create(OP_pushnull);
        
        ival->Attach(iprop);
        
        if (icurr)
        {
            icurr->Attach(ival);
        }
        icurr = iprop;
        
        if (!ifirst)
        {
            ifirst = ival;
        }
        ++count;
        curr = curr->next;
    }
    
    if (icurr && icurr != itype)
    {
        icurr->Attach(itype);
    }
    
    inewObj->operand = count;
    
    return ifirst ? ifirst : itype;
}

Instr* ArrayExpr::GenerateCode()
{
    Instr* ibeg      = 0;
    Instr* inewArray = Instr::Create(OP_array);
    ExprList*    curr      = elements;
    u2           count     = 0;
    
    while (curr)
    {
        Expr *expr = curr->expr;
        Instr* iexpr = expr->GenerateCode();
        
        if (ibeg)
        {
            ibeg->Attach(iexpr);
        }
        else
        {
            ibeg = iexpr;
        }
        ++count;
        curr = curr->next;
    }
    
    inewArray->operand = count;
    
    if (ibeg)
    {
        ibeg->Attach(inewArray);
    }
    else
    {
        ibeg = inewArray;
    }
    return ibeg;
}

Instr* KeywordExpr::GenerateCode()
{
    Instr* iname = name->GenerateCode();
    Instr* ivalue = value->GenerateCode();

    iname->
    Attach(ivalue)
    ;
    
    return iname;
}

Instr* CondExpr::GenerateCode()
{
    Instr* icond      = cond->GenerateCode();
    Instr* ia         = exprA->GenerateCode();
    Instr* ib         = exprB->GenerateCode();
    Instr* ijmpfalse  = Instr::Create(unless ? OP_jumpiftrue : OP_jumpiffalse);
    Instr* ijmp       = Instr::Create(OP_jump);
    Instr* ijmptarget = Instr::Create(JMP_TARGET);
    
    icond->
    Attach(ijmpfalse)->
    Attach(ia)->
    Attach(ijmp)->
    Attach(ib)->
    Attach(ijmptarget);
    
    ijmpfalse->SetTarget(ib);
    ijmp->SetTarget(ijmptarget);
    
    return icond;
}

Instr* NullSelectExpr::GenerateCode()
{
    /*
     *  a ?? b:
     *
     *  If a is null we pop it and evaluate/push expression b, otherwise
     *  if a is not null we skip over evaluating b and keep a.
     *
     *  [   ]            eval expression a
     *  [ a ]            duplicate top of stack
     *  [ a a ]          push null
     *  [ a a null ]     compare a and null
     *
     *  [ a false ] if false [ a true ] if true
     *                 |     [ a ]      pop a
     *                 |     [   ]      eval expression b
     *  [ a ]      <---'     [ b ]
     */
    Instr* ia         = exprA->GenerateCode();
    Instr* ib         = exprB->GenerateCode();
    Instr* idup       = Instr::Create(OP_dup);
    Instr* ijmpfalse  = Instr::Create(OP_jumpiffalse);
    Instr* ijmptarget = Instr::Create(JMP_TARGET);
    Instr* ipop       = Instr::Create(OP_pop);
    Instr* ipushnull  = Instr::Create(OP_pushnull);
    Instr* ieq        = Instr::Create(OP_eq);
    
    ia->
    Attach(idup)->
    Attach(ipushnull)->
    Attach(ieq)->
    Attach(ijmpfalse)->
    Attach(ipop)->
    Attach(ib)->
    Attach(ijmptarget);
    
    ijmpfalse->SetTarget(ijmptarget);
    
    return ia;
}

Instr* SliceExpr::GenerateCode()
{
    /*
     * The slice operator (expr[from..to]), grabs the function 'opSlice' from
     * expr and calls it with arguments (from, to).
     *
     * [ ]                              eval expression from
     * [ from ]                         eval expression to
     * [ from to ]                      eval expression expr
     * [ from to expr ]                 duplicate top of stack
     * [ from to expr expr ]            push PIKA_SLICE_CSTRING
     * [ from to expr expr 'opSlice' ]  get expr.opSlice
     * [ from to expr expr.opSlice ]    call expr.opSlice( from, to )
     * [ result ]
     */
    Instr* iexpr   = expr->GenerateCode();
    Instr* ifrom   = from->GenerateCode();
    Instr* ito     = to->GenerateCode();
    Instr* icall   = Instr::Create(OP_call);
    Instr* idup    = Instr::Create(OP_dup); // duplicate iexpr
    Instr* islice  = slicefun->GenerateCode();
    Instr* idotget = Instr::Create(OP_dotget);
    
    ifrom->
    Attach(ito)->
    Attach(iexpr)->
    Attach(idup)->
    Attach(islice)->
    Attach(idotget)->
    Attach(icall);
    
    icall->operand = 2;
    return ifrom;
}

Instr* DeclarationTarget::GenerateCodeSet()
{
    if (with)
    {
        Instr* isetMember = Instr::Create(OP_setmember); // member
        isetMember->operand = nameindex;
        return isetMember;        
    }
    else if (symbol->isGlobal)
    {
        Instr* isetGlobal = Instr::Create(OP_setglobal); // global
        isetGlobal->operand = nameindex;
        return isetGlobal;
    }
    else
    {
        Instr* isetLocal = Instr::Create(OP_setlocal); // local
        isetLocal->operand = symbol->offset;
        isetLocal->operandu1 = 1;
        isetLocal->target = state->endOfBlock;
        return isetLocal;
    }
    return 0;
}

Instr* NamedTarget::GenerateAnnotationCode(Instr* subj)
{
    if (annotations)
        return annotations->GenerateCodeWith(subj);    
    return subj;
}

Instr* NamedTarget::GenerateCodeWith(Instr* body)
{
    Instr* get = name->GenerateCode();
    Instr* enterWith = Instr::Create(OP_pushwith);
    Instr* exitWith  = Instr::Create(OP_popwith);
    
    get->
    Attach(enterWith)->
    Attach(body)->
    Attach(exitWith);
    
    HandleBlockBreaks(body,
                      exitWith,
                      OP_popwith,
                      true);
    return get;
}

Instr* PropertyDecl::GeneratePropertyCode()
{
    Instr* iprop = Instr::Create(OP_property);
    
    Instr* iname   = (name_expr) ? name_expr->GenerateCode() : Instr::Create(OP_pushnull);
    Instr* igetter = (getter)    ? getter->GenerateCode()    : Instr::Create(OP_pushnull);
    Instr* isetter = (setter)    ? setter->GenerateCode()    : Instr::Create(OP_pushnull);
    
    iname->line   = (name_expr) ? name_expr->line : line;
    isetter->line = (setter)    ? setter->line    : line;
    igetter->line = (getter)    ? getter->line    : line;
    
    if (isetter->line >= igetter->line)
    {
        iname->
        Attach(igetter)->
        Attach(isetter)->
        Attach(iprop)
        ;
    }
    else
    {
        /* we output in the order specified by the user
         * then swap the getter and setter before creating the
         * property.
         *
         * This ensures that LineInfo order is preserved, expressions are evaluated inorder and
         * OP_property recieves its name | getter | setter in the correct order.
         */
        Instr* iswap = Instr::Create(OP_swap);
        iname->
        Attach(isetter)->
        Attach(igetter)->
        Attach(iswap)->
        Attach(iprop)
        ;
    }    
    return iname;
}

Instr* PropertyDecl::GenerateCode()
{
    Instr* prop = GeneratePropertyCode();
    Instr* annotations = GenerateAnnotationCode(prop);
    Instr* set = NamedTarget::GenerateCodeSet();    
    
    annotations->
    Attach(set);
    return annotations;
}

Instr* AssignmentStmt::DoStmtCodeGen()
{
    Instr*    assgn = Instr::Create(OP_nop);
    ExprList* curr  = right;
    if (isUnpack && !isCall)
    {
        /* Multiple left hand operands and only one right hand operand.
         *
         * a, b, c = right
         *
         * We want to unpack the right hand operand and then assign those values to the
         * left hand operands.
         */
        Instr* iright    = right->expr->GenerateCode();
        Instr* iunpack   = Instr::Create(OP_unpack);
        iunpack->operand = unpackCount;
        
        assgn->Attach(iright)->Attach(iunpack);
    }
    else
    {
        if (isBinaryOp)
        {
            ReverseExprList(&left);
            
            ExprList* lcurr = left;
            Opcode oc = GetOpcode();
            
            while (curr && lcurr)
            {
                Instr* irep  = curr->expr->GenerateCode();
                Instr* ilrep = lcurr->expr->GenerateCode();
                Instr* iop   = Instr::Create(oc);
                
                assgn->Attach(ilrep);
                ilrep->Attach(irep);
                irep->Attach(iop);
                
                curr  = curr->next;
                lcurr = lcurr->next;
            }
            ReverseExprList(&left);
        }
        else
        {
            while (curr)
            {
                Instr* irep = curr->expr->GenerateCode();
                assgn->Attach(irep);
                curr = curr->next;
            }
        }
    }
    
    if (isUnpack || isCall)
    {
        ReverseExprList(&left);
    }
    curr = left;
    
    if (isUnpack && isBinaryOp)
    {
        Opcode oc = GetOpcode();
        while (curr)
        {
            Expr::Kind k = curr->expr->kind;
            if (k == Expr::EXPR_identifier)
            {
                IdExpr* ide  = (IdExpr*)curr->expr;
                Instr*  iget = ide->GenerateCode();
                Instr*  iop  = Instr::Create(oc);
                Instr*  irep = ide->GenerateCodeSet();
                
                iget->
                Attach(iop)->
                Attach(irep)
                ;
                
                assgn->Attach(iget);
            }
            else if (k == Expr::EXPR_dot)
            {
                DotExpr* ide  = (DotExpr*)curr->expr;
                Instr*   iget = ide->GenerateCode();
                Instr*   iop  = Instr::Create(oc);
                Instr*   irep = ide->GenerateCodeSet();
                
                iget->Attach(iop)->Attach(irep);
                assgn->Attach(iget);
            }
            else if (k == Expr::EXPR_load)
            {
                Instr* ipop = Instr::Create(OP_pop);
                assgn->Attach(ipop);
            }
            else
            {
                SHOULD_NEVER_HAPPEN();
            }
            curr = curr->next;
        }
    }
    else
    {
        while (curr)
        {
            Expr::Kind k = curr->expr->kind;
            if (k == Expr::EXPR_identifier)
            {
                IdExpr* ide = (IdExpr*)curr->expr;
                Instr* irep = ide->GenerateCodeSet();
                assgn->Attach(irep);
            }
            else if (k == Expr::EXPR_dot)
            {
                DotExpr* ide = (DotExpr*)curr->expr;
                Instr* irep = ide->GenerateCodeSet();
                assgn->Attach(irep);
            }
            else if (k == Expr::EXPR_load)
            {
                Instr* ipop = Instr::Create(OP_pop);
                assgn->Attach(ipop);
            }
            else
            {
                SHOULD_NEVER_HAPPEN();
            }
            curr = curr->next;
        }
    }
    return assgn;
}

Instr* FinallyStmt::DoStmtCodeGen()
{
    /*
     * There are 3 situations we want to deal with.
     * 
     * 1.) Normal run
     *     The statment block runs to completion
     * 
     * 2.) Non-local-jump
     *     There is a non-local-jump (return, break or continue) and the block exits prematurely
     * 
     * 3.) Exception
     *     An exception is raised in the block and we have to unwind the stack and call the enclosing
     *     handler (if present).
     * 
     * For all 3 we want to ensure that the finally block will always be called.
     * 
     * ---------------------------------------------------------------------------------------------
     *
     * 1.) Normal
     *     A. Insert OP_callfinally at the end of the block with the finally block as the target.
     *     B. When the finally block returns we unconditionally jump to the finished label.
     *   
     * 2.) Non-local-jumps (return and continue, break loop in scope outside the block)
     *     A. Insert OP_callfinally before each jump with the finally block as the target.
     *     B. When the finally block returns the non-local-jump is performed, exiting the block.
     *  
     * 3.) Exception
     *     A.  Jump to invoke_finally.
     *         (Note that the entire block is actually a try-block, so invoke_finally is our
     *         'catch' handler.)
     *     B. Call finally block
     *     C. Reraise the exception
     *
     * ---------------------------------------------------------------------------------------------
     *  
     * start:
     *      pushfinally  [ target: invoke_finally ]
     *      <block>
     *      callfinally  [ target: finalize       ]
     *      jump         [ target: finished       ]
     * finalize:
     *      <finally block>
     *      jump to address on top of address stack
     * invoke_finally:
     *      callfinally  [ target: finalize       ]
     *      raise
     * finished:
     */
    Instr* ibegin     = Instr::Create(OP_nop);
    Instr* iblock     = block ? block->GenerateCode() : Instr::Create(OP_nop);
    Instr* iretfinally = Instr::Create(OP_retfinally);
    
    PIKA_BLOCKSTART(state, iretfinally);
    // START BLOCK----------------------------------------------------------------------------------
    
    Instr* ifinalize_block = finalize_block ? finalize_block->GenerateCode() : Instr::Create(OP_nop);
    
    if (block->kind == Stmt::STMT_with)
    {
        Instr* ientry = ((UsingStmt*)block)->DoHeader();
        ientry->Attach(ibegin);
        ibegin = ientry;
        
        Instr* ipopwith = Instr::Create(OP_popwith);
        ipopwith->Attach(ifinalize_block);
        ifinalize_block = ipopwith;
    }
    
    // END BLOCK------------------------------------------------------------------------------------
    PIKA_BLOCKEND(state);
    
    Instr* ipushfinally   = Instr::Create(OP_pushfinally);
    Instr* ipopfinally    = Instr::Create(OP_pophandler);
    Instr* icallfinally   = Instr::Create(OP_callfinally);
    Instr* ijmptoend     = Instr::Create(OP_jump);
    Instr* ifinished     = Instr::Create(JMP_TARGET);
    Instr* invoke_finally = Instr::Create(OP_callfinally);
    Instr* reraise       = Instr::Create(OP_raise);
    
    ibegin->
    Attach(ipushfinally)->
    Attach(iblock)->
    Attach(ipopfinally)->
    Attach(icallfinally)->       // stack call -> pop finally
    Attach(ijmptoend)->
    Attach(ifinalize_block)->
    Attach(iretfinally)->        // jsr
    Attach(invoke_finally)->
    Attach(reraise)->
    Attach(ifinished);
    
    ipushfinally->SetTarget(invoke_finally);
    invoke_finally->SetTarget(ifinalize_block);
    icallfinally->SetTarget(ifinalize_block);
    ijmptoend->SetTarget(ifinished);
    
    FinallyBlockBreaks(ibegin,          // Start of the block.
                       ifinalize_block,  // End of the block.
                       ifinalize_block); // Target for OP_callfinally, the beginning of the finally block.
#if defined(ENABLE_SYNTAX_WARNINGS)                      
    WarnNonLocalJumps(state,
                      ifinalize_block,
                      iretfinally,
                      line);
#endif
    return ibegin;
}

INLINE VarDecl* ReverseDeclarations(VarDecl** l)
{
    VarDecl* temp1 = *l;
    VarDecl* temp2 = 0;
    VarDecl* temp3 = 0;
    
    while (temp1)
    {
        *l = temp1; // Set the head to last node
        temp2 = temp1->next; // Save the next ptr in temp2
        temp1->next = temp3; // Change next to previous
        temp3 = temp1;
        temp1 = temp2;
    }
    return *l;
}

Instr* VariableTarget::GenerateCode()
{
    Instr* assgn = Instr::Create(OP_nop);
    ExprList* curr = exprs;
    
    if (isUnpack && !isCall)
    {
        Instr* iright = curr->expr->GenerateCode();
        Instr* iunpack = Instr::Create(OP_unpack);
        iunpack->operand = unpackCount;
        
        assgn->Attach(iright)->Attach(iunpack);
    }
    else
    {
        while (curr)
        {
            Instr* irep = curr->expr->GenerateCode();
            assgn->Attach(irep);
            curr = curr->next;
        }
    }
    
    ReverseDeclarations(&decls);
    curr_decl = decls;
    
    while (curr_decl)
    {
        symbol    = curr_decl->symbol;
        with      = curr_decl->symbol->isWith;
        nameindex = curr_decl->nameIndex;
        
        Instr* iset = GenerateCodeSet();
        assgn->Attach(iset);
        curr_decl = curr_decl->next;
    }
    ReverseDeclarations(&decls);
    
    return assgn;
}

Instr* UsingStmt::DoHeader()
{
    Instr* iwith = with->GenerateCode();
    Instr* ipushwith = Instr::Create(OP_pushwith);
    iwith->
    Attach(ipushwith);
    return iwith;
}

Instr* UsingStmt::DoStmtCodeGen()
{
    // A <using> statement is always wrapped inside of an finally block.
    // The finally-block will handle any cleanup required even from a return / break / or continue.
    
    //Instr* iwith = with->GenerateCode();
    //Instr* ipushwith = Instr::Create(OP_pushwith);
    Instr* ipopwith = Instr::Create(OP_nop);
    
    PIKA_BLOCKSTART(state, ipopwith);
    Instr* iblock = block->GenerateCode();
    PIKA_BLOCKEND(state);
    
    //iwith->
    //Attach(ipushwith)->
    iblock->
    Attach(ipopwith);
    
    /*HandleBlockBreaks(iblock,
                      ipopwith,
                      OP_popwith);*/
                      
    return iblock;
}

Instr* PkgDecl::GenerateCode()
{
    Instr* insideof  = name->GetSelfExpr() ? name->GetSelfExpr()->GenerateCode() : Instr::Create(OP_pushnull);
    Instr* ipkgname  = id->GenerateCode();
    Instr* ipush_pkg = Instr::Create(OP_pushpkg);
    Instr* inew_pkg  = Instr::Create(OP_newenv);
    Instr* ipop_pkg  = Instr::Create(OP_poppkg);
    
    PIKA_BLOCKSTART(state, ipop_pkg);
    Instr* ibody = body->GenerateCode();
    PIKA_BLOCKEND(state);
    
    Instr* iassign = NamedTarget::GenerateCodeSet();
    Instr* idup    = Instr::Create(OP_dup);
    //Instr* annotations   = GenerateAnnotationCode();
    
    ipkgname         ->
    Attach(insideof) ->
    Attach(inew_pkg) 
    ;
    
    ipkgname = GenerateAnnotationCode(ipkgname);
    
    ipkgname->
    Attach(idup)     -> // duplicate it     
    Attach(iassign)  -> // set the variable
    Attach(ipush_pkg)-> // enter package scope
    Attach(ibody)    -> // execute body
    Attach(ipop_pkg)    // exit package scope
    ;
    
    HandleBlockBreaks(ibody,
                      ipop_pkg,
                      OP_poppkg,
                      true);
                      
    return ipkgname;
}

// NameNode ////////////////////////////////////////////////////////////////////////////////////////

Instr* NameNode::GenerateCode()
{
    if (idexpr)
    {
        return idexpr->GenerateCode();
    }
    else if (dotexpr)
    {
        return dotexpr->GenerateCode();
    }
    SHOULD_NEVER_HAPPEN();
    return 0;
}

Instr* NameNode::GenerateCodeSet()
{
    if (idexpr)
    {
        return idexpr->GenerateCodeSet();
    }
    else if (dotexpr)
    {
        return dotexpr->GenerateCodeSet();
    }
    SHOULD_NEVER_HAPPEN();
    return 0;
}

Instr* ClassDecl::GenerateCode()
{
    Opcode const popcode = OP_poppkg;
    Opcode const pushcode = OP_pushpkg;
    Instr* insideof  = name->GetSelfExpr() ? name->GetSelfExpr()->GenerateCode() : Instr::Create(OP_pushnull);
    Instr* exitWith  = Instr::Create(popcode);
    Instr* typeName  = stringid->GenerateCode();
    Instr* enterWith = Instr::Create(pushcode);
    Instr* newType   = Instr::Create(OP_subclass);
    Instr* superType = super ? super->GenerateCode() : Instr::Create(OP_pushnull);    
    Instr* dup = Instr::Create(OP_dup);
    
    PIKA_BLOCKSTART(state, exitWith);
    
    Instr* doStmts = stmts->GenerateCode();
    
    PIKA_BLOCKEND(state);
    
    Instr* iassign = NamedTarget::GenerateCodeSet();
    
    insideof         ->
    Attach(typeName) -> // Push the literal contains the typename.
    Attach(superType)-> // Push the result of the super expression.
    Attach(newType)     // Create a new Type from the super and typename given.
   ;
    // Generate the code for annotations, which will use the Type we create
    // as a argument.
    insideof = GenerateAnnotationCode(insideof);
    
    
    insideof         ->
    Attach(dup)      -> // Need two copies one to assign the variable, the other to set up a pkg scope.
    Attach(iassign)  -> // Assign: We need to do this encase there are any
                        //         references to it in the class body.
    Attach(enterWith)-> // Enter a with frame using the Type.
    Attach(doStmts)  -> // Execute the class body.
    Attach(exitWith);   // Exit the with block.
    
    // Handle any breaks out of the with block.
    HandleBlockBreaks(doStmts,
                      exitWith,
                      popcode,
                      false);
                      
    return insideof;
}

Instr* ParenExpr::GenerateCode()
{
    return expr->GenerateCode();
}

}// pika
