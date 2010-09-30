/*
 *  PAst.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PAst.h"
#include "PSymbolTable.h"

namespace pika {

namespace {

void Pika_FunctionResourceCalculation(
    int           funcline, /* in  */
    SymbolTable*  st,       /* in  */
    CompileState& cs,       /* in  */
    ParamDecl*    args,     /* in  */
    Stmt*         body,     /* in  */
    u2*           index,    /* out */
    Def**         def,      /* out */
    SymbolTable** symtab,   /* out */
    const char*   name      /* in  */)
{
    // name, parent, literals
    (*def) = Def::Create(cs.engine);
    (*def)->name = cs.engine->AllocString(name ? name : "");
    (*def)->line = funcline;
    
    // push offsets
    
    int oldOffset = cs.GetLocalOffset();
    int oldLocalCount = cs.localCount;
    CompileState::TryState oldTryState = cs.trystate;
    u2 prevLine = cs.currentLine;
    
    cs.SetLineInfo(prevLine);
    
    (*def)->parent = cs.currDef;
    
    cs.currDef = (*def);
    
    cs.SetLocalOffset(0);
    cs.localCount = 0;
    
    *index = cs.AddConstant((*def));
    (*def)->literals = cs.literals;
    
    *symtab = 0;
    PIKA_NEW(SymbolTable, (*symtab), (st, 0));
    
    if ((*symtab)->IncrementDepth() > PIKA_MAX_NESTED_FUNCTIONS)
    {
        cs.SyntaxException(Exception::ERROR_syntax, funcline, "max nested functions depth reached %d.", PIKA_MAX_NESTED_FUNCTIONS);
    }
    
    if (args)
    {
        args->CalculateDefaultResources(st);
        args->CalculateResources((*symtab));
        (*def)->numArgs = cs.localCount;
        if (cs.localCount > PIKA_MAX_ARGS)
        {
            cs.SyntaxException(Exception::ERROR_syntax, funcline, "max number of argument for function declaration reached %d.", PIKA_MAX_ARGS);    
        }
    }
    else
    {
        (*def)->numArgs = 0;
    }
    
    if (body)
    {
        body->CalculateResources((*symtab));
    }
    (*def)->numLocals = cs.localCount;
    
    // pop offsets
    cs.trystate = oldTryState;
    cs.SetLocalOffset(oldOffset);
    cs.currDef = (*def)->parent;
    cs.localCount = oldLocalCount;
    cs.SetLineInfo(prevLine);
}

}//anonymous namespace

void TreeNodeList::operator+=(TreeNode* t)
{
    *last = t;
    last = &t->astnext;
}

CompileState::CompileState(Engine* eng)
        : literals(0),
        engine(eng),
        currDef(0),
        localOffset(0),
        localCount(0),
        currentLine(0),
        errors(0),
        warnings(0),
        endOfBlock(0),
        parser(0),
        repl_mode(false)
{
    trystate.inCatch = false;
    trystate.inTry = false;
    trystate.catchVarOffset = (u2)(-1);
}

CompileState::~CompileState()
{
    TreeNode* t = nodes.first;
    
    while (t)
    {
        TreeNode* next = t->astnext;
        Pika_delete(t);
        t = next;
    }
}

int CompileState::GetLocalOffset() const
{
    return localOffset;
}

void CompileState::SetLocalOffset(int off)
{
    localOffset = off;
}

int CompileState::NextLocalOffset(const char* name, ELocalVarType lvt)
{
    int off = localOffset;
    
    if (++localOffset > localCount)
    {
        localCount = localOffset;
    }
    if (currDef)
    {
        currDef->AddLocalVar(engine, name, lvt);
    }
    return off;
}

void CompileState::SetLineInfo(u2 line)
{
    currentLine = line;
}

bool CompileState::UpdateLineInfo(int line)
{
    if (line <= 0 || line <= (int)currentLine)
    {
        return false;
    }
    currentLine = (u2)line;
    return true;
}

u2 CompileState::AddConstant(pint_t i)
{
    Value v, res;
    v.Set(i);
    
    if (literalLookup.Get(v, res))
    {
        return (u2)res.val.index;
    }
    
    res.tag = TAG_index;
    res.val.index = literals->Add(v);
    
    literalLookup.Set(v, res);
    
    return (u2)res.val.index;
}

u2 CompileState::AddConstant(preal_t f)
{
    Value v, res;
    v.Set(f);
    
    if (literalLookup.Get(v, res))
    {
        return (u2)res.val.index;
    }
    res.tag = TAG_index;
    res.val.index = literals->Add(v);
    
    literalLookup.Set(v, res);
    
    return (u2)res.val.index;
}

u2 CompileState::AddConstant(Def *fun)
{
    Value v(fun);
    return literals->Add(v);
}

u2 CompileState::AddConstant(const char *cstr)
{
    String *s = engine->AllocString(cstr);
    
    Value v(s), res;
    
    if (literalLookup.Get(v, res))
    {
        return (u2)res.val.index;
    }
    
    res.tag = TAG_index;
    res.val.index = literals->Add(v);
    
    literalLookup.Set(v, res);
    
    return (u2)res.val.index;
}

u2 CompileState::AddConstant(const char *cstr, size_t len)
{
    String *s = engine->AllocString(cstr, len);
    
    Value v(s), res;
    
    if (literalLookup.Get(v, res))
    {
        return (u2)res.val.index;
    }
    
    res.tag = TAG_index;
    res.val.index = literals->Add(v);
    
    literalLookup.Set(v, res);
    
    return (u2)res.val.index;
}

Symbol* CompileState::CreateLocalPlus(SymbolTable* st, const char* name, size_t extra)
{
    Symbol* symbol   = st->Put(name);
    symbol->isWith   = false;
    symbol->isGlobal = false;
    symbol->offset   = this->NextLocalOffset(name);
    
    for (size_t i = 0; i < extra; ++i)
    {
        this->NextLocalOffset(""); // generate a local
    }
    return symbol;
}

#if defined(ENABLE_SYNTAX_WARNINGS)

void CompileState::SyntaxWarning(WarningLevel level, int line, const char* format, ...)
{
    va_list args;
    
    this->warnings++;
    fprintf(stderr, "** Syntax Warning %d (line %d): ", level, line);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
    fflush(stderr);
}

#endif /* ENABLE_SYNTAX_WARNINGS */

void CompileState::SyntaxError(int line,  const char* format, ...)
{
    va_list args;
    
    this->errors++;
    fprintf(stderr, "** Syntax Error (line %d): ", line);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
    fflush(stderr);
    
    if (repl_mode)
        throw Exception(Exception::ERROR_syntax);        
}

void CompileState::SyntaxException(Exception::Kind k, int line, const char *msg, ...)
{
    va_list args;
    
    errors++;
    fprintf(stderr, "** Syntax Error (line %d): ", line);
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
    fprintf(stderr, "\n");
    
    throw Exception(k);
}

void CompileState::SyntaxException(Exception::Kind k, int line, int col, const char *msg, ...)
{
    va_list args;
    
    errors++;
    fprintf(stderr, "** Syntax Error at line %d col %d: ", line, col);
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
    fprintf(stderr, "\n");
    
    throw Exception(k);
}

void CompileState::SyntaxErrorSummary()
{
    fprintf(stderr, "%d error(s) found.\n",   this->errors);
    fprintf(stderr, "%d warning(s) found.\n", this->warnings);
    fflush(stderr);
}

Id::Id(CompileState* s, char *name) : TreeNode(s), name(name), next(0) {}

Id::~Id() { Pika_free(name); }

void FunctionProg::CalculateResources(SymbolTable* st)
{
    symtab = 0;
    PIKA_NEW(SymbolTable, symtab, (st, ST_function));
    
    state->literals = LiteralPool::Create(state->engine);
    
    Pika_FunctionResourceCalculation(line, st, *state,
                                      args, stmts,
                                      &index,
                                      &def,
                                      &symtab,
                                      "(main)");
}

void Program::CalculateResources(SymbolTable* st)
{
    symtab = 0;
    PIKA_NEW(SymbolTable, symtab, (st, state->repl_mode ? ST_package : ST_main));
    
    state->literals = LiteralPool::Create(state->engine);
    
    def = Def::Create(state->engine);
    
    def->numLocals = 0;
    def->mustClose = false;
    def->name = state->engine->AllocString("__main");
    def->literals = state->literals;
    
    index = state->AddConstant(def);
    state->currDef = def;
    
    stmts->CalculateResources(symtab);
    
    def->numLocals = state->localCount; // even though a program defaults to global it might still have lexEnv!
}

Program::~Program()
{
    Pika_delete(symtab);
}

const char* NamedTarget::GetIdentifierName()
{
    return name->GetName();
}

void NamedTarget::CalculateSymbols(SymbolTable* st)
{
    CreateSymbol(st);
    name->CalculateResources(st);
}

void NamedTarget::CalculateResources(SymbolTable* st)
{   
    if (annotations) 
        annotations->CalculateResources(st);
}

void AnnotationDecl::CalculateResources(SymbolTable* st)
{
    if (next) next->CalculateResources(st);
    if (name) name->CalculateResources(st);
    if (args) args->CalculateResources(st);
}

FunctionDecl::FunctionDecl(CompileState* s, NameNode* name, ParamDecl* args, Stmt* body,
                           size_t begtxt, size_t endtxt,
                           StorageKind sk)
        : NamedTarget(s, name, Decl::DECL_function, sk),
        def(0),
        index(0),
        args(args),
        body(body),
        symtab(0),
        varArgs(false),
        kwArgs(false),
        scriptBeg(begtxt),
        scriptEnd(endtxt)
{
    if (args) {
        varArgs = args->HasVarArgs();
        kwArgs = args->HasKeywordArgs();
    } else {
        varArgs = false;
        kwArgs = false;
    }    
}

const char* FunctionDecl::GetIdentifierName() { return name->GetName(); }

void FunctionDecl::CalculateResources(SymbolTable* st)
{    
    NamedTarget::CalculateResources(st);
    NamedTarget::CalculateSymbols(st);
    
    Pika_FunctionResourceCalculation(line, st, *state,
                                      args, body,
                                      &index,
                                      &def,
                                      &symtab,
                                      name->GetName());
                                      
    def->isVarArg = varArgs ? 1 : 0;
    def->isKeyword = kwArgs ? 1 : 0;
}

FunctionDecl::~FunctionDecl() { Pika_delete(symtab); }

ParamDecl::ParamDecl(CompileState* s, Id *name, bool rest, bool kw, Expr* val)
        : Decl(s, Decl::DECL_parameter),
        val(val),
        symbol(0),
        name(name),
        has_rest(rest),
        has_kwargs(kw),
        next(0) {}
        
void ParamDecl::CalculateDefaultResources(SymbolTable* st)
{
    if (val) val->CalculateResources(st);
    if (next) next->CalculateDefaultResources(st);
}

void ParamDecl::CalculateResources(SymbolTable* st)
{
    if (st->IsDefined(name->name))
    {
        symbol = st->Shadow(name->name);
    }
    else
    {
        symbol = st->Put(name->name);
    }
    
    symbol->offset = state->NextLocalOffset(name->name, has_rest ? LVT_rest : (has_kwargs ? LVT_keyword : LVT_parameter));
    
    if (next)
    {
        next->CalculateResources(st);
    }
}

void ParamDecl::Attach(ParamDecl* nxt)
{
    if (!next)
    {
        next = nxt;
    }
    else
    {
        ParamDecl *curr = next;
        
        while (curr->next)
        {
            curr = curr->next;
        }
        curr->next = nxt;
    }
}

bool ParamDecl::HasDefaultValue()
{
    if (val)
    {
        return true;
    }
    
    if (next)
    {
        return next->HasDefaultValue();
    }
    
    return false;
}

bool ParamDecl::HasVarArgs()
{
    if (next) {
        if (!next->next && has_rest)
            return has_rest;
        return next->HasVarArgs();
    }
    return has_rest;
}

bool ParamDecl::HasKeywordArgs()
{
    if (next)
        return next->HasKeywordArgs();
    return has_kwargs;
}

void KeywordExpr::CalculateResources(SymbolTable* st)
{
    if (!name || !value)
        state->SyntaxException(Exception::ERROR_syntax, line, "malformed keyword argument.");
    name->CalculateResources(st);
    value->CalculateResources(st);
}

void LocalDecl::CalculateResources(SymbolTable* st)
{
    if (st->IsDefined(name->name))
    {
        symbol = st->Get(name->name);
        if (symbol->table == st && !symbol->isGlobal && !symbol->isWith)
        {
            newLocal = false;
        }
        else
        {
            symbol    = st->Shadow(name->name);
            newLocal = true;
        }
    }
    else
    {
        symbol    = st->Put(name->name);
        newLocal = true;
    }
    symbol->isWith   = false;
    symbol->isGlobal = false;
    
    if (newLocal)
    {
        symbol->offset = state->NextLocalOffset(name->name);
    }
    
    if (next)
        next->CalculateResources(st);
}

void MemberDeclaration::CalculateResources(SymbolTable* st)
{
    if (st->IsDefined(name->name))
    {
        symbol = st->Shadow(name->name);
    }
    else
    {
        symbol = st->Put(name->name);
    }
    
    symbol->isGlobal = true;
    symbol->isWith   = true;
    symbol->offset = -1;
    nameIndex = state->AddConstant(name->name);
    
    if (next)
        next->CalculateResources(st);
}

void VarDecl::CalculateResources(SymbolTable* st)
{
    if (st->IsDefined(name->name))
    {
        symbol = st->Shadow(name->name);
    }
    else
    {
        symbol = st->Put(name->name);
    }
    
    symbol->isGlobal = true;
    symbol->isWith   = false;
    symbol->offset = -1;
    nameIndex = state->AddConstant(name->name);
    
    if (next)
        next->CalculateResources(st);
}

void BreakStmt::DoStmtResources(SymbolTable* st)
{
    if (id)
    {
        label = st->Get(id->name);
        if (!label)
        {
            state->SyntaxError(line, "break statement: no matching label '%s' found.", id->name);
            RaiseException(Exception::ERROR_syntax, "BreakStmt");
        }
    }
}

void ContinueStmt::DoStmtResources(SymbolTable* st)
{
    if (id)
    {
        label = st->Get(id->name);
        
        if (!label)
        {
            state->SyntaxError(line, "continue statement: no matching label '%s' found.", id->name);
            RaiseException(Exception::ERROR_syntax, "ContinueStmt");
        }
    }
}

void LabeledStmt::DoStmtResources(SymbolTable* st)
{
    ASSERT(id && stmt);
    
    if (st->IsDefined(id->name))
    {
        label = st->Shadow(id->name);
    }
    else
    {
        label = st->Put(id->name);
    }
    
    if (stmt)
    {
        if (stmt->IsLoop())
        {
            LoopingStmt* lstmt = (LoopingStmt*)stmt;
            lstmt->label = label;
        }
#if defined(ENABLE_SYNTAX_WARNINGS)
        else
        {
            state->SyntaxWarning(WARN_mild, line, "label \"%s\" has no meaning in this context.", id->name);
        }
#endif
        stmt->CalculateResources(st);
    }
}

void ExprStmt::DoStmtResources(SymbolTable* st)
{
    ExprList* curr = exprList;
    
    while (curr)
    {
        Expr* expr = curr->expr;
        if (false && autopop)
        {
            if (expr)
            {
                Expr::Kind k = expr->kind;
                
                switch (k)
                {
                case Expr::EXPR_preincr:
                case Expr::EXPR_predecr:
                case Expr::EXPR_postincr:
                case Expr::EXPR_postdecr:
                case Expr::EXPR_call:
                case Expr::EXPR_new:
                    expr->CalculateResources(st);
                    break;
                    
                default:
                    state->SyntaxException(Exception::ERROR_syntax, expr->line, "Only assignment, call, increment, decrement and new expressions can be used as a statement.");
                    break;
                };
            }
        }
        else
        {
            expr->CalculateResources(st);
        }
        curr = curr->next;
    }
}

void RetStmt::DoStmtResources(SymbolTable* st)
{
    count = 0;
    ExprList* curr = exprs;
    isInTry = state->trystate.inTry;
    while (curr)
    {
        ++count;
        curr->expr->CalculateResources(st);
        curr = curr->next;
        if (count >= PIKA_MAX_RETC)
        {
            state->SyntaxException(Exception::ERROR_syntax, line,
                                   "max number of return values exceeded (max = %d).",
                                   PIKA_MAX_RETC);
        }
    }
}

void YieldStmt::DoStmtResources(SymbolTable* st)
{
    count = 0;
    ExprList* curr = exprs;
    while (curr)
    {
        ++count;
        curr->expr->CalculateResources(st);
        curr = curr->next;
        if (count >= PIKA_MAX_RETC)
        {
            state->SyntaxException(Exception::ERROR_syntax, line,
                                   "max number of yield values exceeded (max = %d).",
                                   PIKA_MAX_RETC);
        }
    }
}

void LoopStmt::DoStmtResources(SymbolTable* st)
{
    if (body)
    {
        body->CalculateResources(st);
    }
}

ForToStmt::~ForToStmt() { Pika_delete(symtab); }

void ForToStmt::DoStmtResources(SymbolTable* st)
{
    PIKA_NEW(SymbolTable, symtab, (st, st->IsWithBlock() ? ST_using : 0));
    
    // Create the looping variable's symbol along to extra local variables
    // (for 'to' and 'by'.)
    symbol = state->CreateLocalPlus(symtab, id->name, 2);
    
    from->CalculateResources(symtab);
    to  ->CalculateResources(symtab);
    step->CalculateResources(symtab);
    body->CalculateResources(symtab);
}

ForeachStmt::~ForeachStmt()
{
    Pika_delete(symtab);
}

void ForeachStmt::DoStmtResources(SymbolTable* st)
{
    PIKA_NEW(SymbolTable, symtab, (st, st->IsWithBlock() ? ST_using : 0));
    PIKA_NEWNODE(LocalDecl, iterVar, (state, id));
    iterVar->line = id->line;
    PIKA_NEWNODE(IdExpr, idexpr, (state, id));
    
    
    idexpr->line = iterVar->name->line;
    enum_offset = state->NextLocalOffset("");
    
    type_expr->CalculateResources(symtab);
    iterVar->CalculateResources(symtab);
    idexpr->CalculateResources(symtab);
    in->CalculateResources(symtab);
    body->CalculateResources(symtab);
}

void CondLoopStmt::DoStmtResources(SymbolTable* st)
{
    if (cond) cond->CalculateResources(st);
    if (body) body->CalculateResources(st);
}

void IfStmt::DoStmtResources(SymbolTable* st)
{
    if (cond)       cond     ->CalculateResources(st);
    if (then_part)  then_part->CalculateResources(st);
    if (next)       next     ->CalculateResources(st);
}

void BlockStmt::DoStmtResources(SymbolTable* st)
{
    symtab = 0;
    PIKA_NEW(SymbolTable, symtab, (st, st->IsWithBlock() ? ST_using : 0));
    
    if (stmts)
    {
        stmts->CalculateResources(symtab);
    }
}

BlockStmt::~BlockStmt() { Pika_delete(symtab); }

DeclStmt::DeclStmt(CompileState* s, Decl *decl) : Stmt(s, Stmt::STMT_decl), decl(decl) {}

void DeclStmt::DoStmtResources(SymbolTable* st)
{
    if (decl)
    {
        decl->CalculateResources(st);
    }
}

ParenExpr::~ParenExpr() {}
    
void ParenExpr::CalculateResources(SymbolTable* st)
{
    expr->CalculateResources(st);
}

void CallExpr::CalculateResources(SymbolTable* st)
{
    if (left)
    {
        Expr::Kind left_kind = left->kind;
        if (left_kind == Expr::EXPR_load)
        {
            LoadExpr* le = (LoadExpr*)left;
            if (le->loadkind == LoadExpr::LK_super)
            {
                redirectedcall = true;
            }
        }
        left->CalculateResources(st);
    }
    if (args) {       
        ExprList* el = args;
        while (el)
        {
            Expr* arg_expr = el->expr;
            arg_expr->CalculateResources(st);
            if (arg_expr->kind == Expr::EXPR_apply_va ||
                arg_expr->kind == Expr::EXPR_apply_kw )
            {
                is_apply = true;
            }
            else if (arg_expr->kind == EXPR_kwarg) {
                if (++kwargc > PIKA_MAX_KWARGS)
                    state->SyntaxError(line, "Max number of keyword arguments reached"); 
            }
            else {            
                if (++argc > PIKA_MAX_ARGS)
                    state->SyntaxError(line, "Max number of arguments reached");
            }
            el = el->next;            
        }
    }
}

void BinaryExpr::CalculateResources(SymbolTable* st)
{
    left ->CalculateResources(st);
    right->CalculateResources(st);
}

void IdExpr::CalculateResources(SymbolTable* st)
{
    symbol = st->Get(id->name);
    
    inWithBlock = st->IsWithBlock();
    depthindex = 0;
    
    if (symbol && symbol->table != st && symbol->isWith && st->GetWith())
    {
        outerwith = true;
    }
    //
    // Tell an outer function that we will need to use its locals.
    //
    if (symbol && (symbol->table->GetDepth() < st->GetDepth()) && !symbol->isGlobal)
    {
    
        int depth = st->GetDepth() - symbol->table->GetDepth();
        depthindex = depth;
        Def* currDef = state->currDef;
        
        while (depth-- && (currDef = currDef->parent)); // XXX: If we ever need the entire chain to be marked this is the place to do it.
        
        currDef->mustClose = true;
        outer = true;
    }
    
    if (IsOuter())
    {
        index = symbol->offset;
    }
    else if (IsLocal())
    {
        index = symbol->offset;
    }
    else if (IsGlobal())
    {
        index = state->AddConstant(id->name);
    }
}

void MemberExpr::CalculateResources(SymbolTable* st)
{
    index = state->AddConstant(id->name);
}

StringExpr::~StringExpr() { Pika_free(string); }

void StringExpr::CalculateResources(SymbolTable* st)
{
    //if (!length)
    //    length = strlen(string);
    
    index = state->AddConstant(string, length);
}

void IntegerExpr::CalculateResources(SymbolTable* st)
{
    index = state->AddConstant(integer);
}

void RealExpr::CalculateResources(SymbolTable* st)
{
    index = state->AddConstant(real);
}

DotExpr::DotExpr(CompileState* s, Expr *l, Expr *r) : BinaryExpr(s, Expr::EXPR_dot, l, r) {}

void DotExpr::CalculateResources(SymbolTable* st)
{
    BinaryExpr::CalculateResources(st);
}

void IndexExpr::CalculateResources(SymbolTable* st)
{
    DotExpr::CalculateResources(st);
}

FunExpr::~FunExpr() { Pika_delete(symtab); }

void FunExpr::CalculateResources(SymbolTable* st)
{
    Pika_FunctionResourceCalculation(line, st, *state, args, body, &index, &def, &symtab, (name) ? name->name : 0);
    if (args) {
        def->isVarArg = args->HasVarArgs();
        def->isKeyword = args->HasKeywordArgs();
    } else {
        def->isVarArg = false;
        def->isKeyword = false;
    }
}

void PropExpr::CalculateResources(SymbolTable* st)
{
    if (nameexpr) nameexpr->CalculateResources(st);
    if (getter)   getter  ->CalculateResources(st);
    if (setter)   setter  ->CalculateResources(st);
}

void ExprList::CalculateResources(SymbolTable* st)
{
    if (expr) expr->CalculateResources(st);
    if (next) next->CalculateResources(st);
}

size_t ExprList::Count()
{
    size_t amt = 1;
    ExprList* e = next;
    while (e)
    {
        e = e->next;
        ++amt;     
    }
    return amt;
}

void UnaryExpr::CalculateResources(SymbolTable* st)
{
    if (expr)
    {
        expr->CalculateResources(st);
        
        if (kind == EXPR_preincr || kind == EXPR_postincr ||
            kind == EXPR_predecr || kind == EXPR_postdecr)
        {
            Expr::Kind k = expr->kind;
            
            if (k != EXPR_dot && k != EXPR_identifier)
            {
                const char* lhs = "lhs";
                const char* rhs = "rhs";
                const char* pp  = "++";
                const char* mm  = "--";
                
                state->SyntaxError(line, "illegal %s value for operator '%s'.",
                                   (kind == EXPR_preincr || kind == EXPR_predecr)  ? rhs : lhs,
                                   (kind == EXPR_preincr || kind == EXPR_postincr) ? pp  : mm);
                                   
                RaiseException(Exception::ERROR_syntax, "UnaryExpr");
            }
        }
    }
}

void DictionaryExpr::CalculateResources(SymbolTable* st)
{
    FieldList* curr = fields;
    
    while (curr)
    {
        /*
        if (curr->value->kind == Expr::EXPR_function)
        {
            FunExpr* fun = (FunExpr*)curr->value;
            if (curr->name->kind == Expr::EXPR_string)
            {
                StringExpr* strexpr = (StringExpr*)curr->name;
                PIKA_NEWNODE(Id, fun->name, (state, Pika_strdup( strexpr->string )));
            }
        }
        */
        if (curr->name)
        {
            curr->name->CalculateResources(st);
            if (curr->name->kind == Expr::EXPR_string)
            {
                StringExpr* str = (StringExpr*)curr->name;
                if (str->length == 4 && StrCmp(str->string, "type") == 0)
                {
                    type_expr = curr->value;
                }
            }
        }
        if (curr->value)
        {
            curr->value->CalculateResources(st);
        }
        
        curr = curr->next;
    }
}

void LoadExpr::CalculateResources(SymbolTable* st)
{
#if !defined(PIKA_LOCALS_CANNOT_CLOSE)
    if (loadkind == LK_locals)
    {
        for (Def* curr = state->currDef->parent; curr != 0; curr = curr->parent)
        {
           curr->mustClose = true;
        }
    }
#endif
}

void ArrayExpr::CalculateResources(SymbolTable* st)
{
    if (elements) elements->CalculateResources(st);
}

void CondExpr::CalculateResources(SymbolTable* st)
{
    if (cond)  cond ->CalculateResources(st);
    if (exprA) exprA->CalculateResources(st);
    if (exprB) exprB->CalculateResources(st);
}

void NullSelectExpr::CalculateResources(SymbolTable* st)
{
    if (exprA) exprA->CalculateResources(st);
    if (exprB) exprB->CalculateResources(st);
}

SliceExpr::~SliceExpr() {}

void SliceExpr::CalculateResources(SymbolTable* st)
{
    Id* id = 0;
    char *slice = Pika_strdup(OPSLICE_STR);
    
    PIKA_NEWNODE(Id, id, (state, slice));
    PIKA_NEWNODE(MemberExpr, slicefun, (state, id));
    
    
    slicefun->line = id->line = expr->line;
    
    slicefun->CalculateResources(st);
    
    if (expr) expr->CalculateResources(st);
    if (from) from->CalculateResources(st);
    if (to)   to  ->CalculateResources(st);
}

CatchIsBlock::~CatchIsBlock()
{
    Pika_delete(symtab);
}

void CatchIsBlock::CalculateResources(SymbolTable* st)
{
    CompileState::TryState trystate = state->trystate;
    
    PIKA_NEW(SymbolTable, symtab, (st, ST_noinherit));
    
    symbol = symtab->Put(catchvar->name);
    symbol->offset = state->NextLocalOffset(catchvar->name);
    
    state->trystate = trystate;
    state->trystate.inCatch = true;
    state->trystate.catchVarOffset = symbol->offset;
    
    isexpr->CalculateResources(symtab);
    block->CalculateResources(symtab);
    
    state->trystate = trystate;
    
    if (next) next->CalculateResources(st);
}

TryStmt::~TryStmt()
{
    Pika_delete(tryTab);
    Pika_delete(catchTab);
}

void TryStmt::DoStmtResources(SymbolTable* st)
{
    CompileState::TryState trystate = state->trystate;
    
    PIKA_NEW(SymbolTable, tryTab, (st, st->IsWithBlock() ? ST_using : 0));
    
    state->trystate.inTry = true;
    
    tryBlock->CalculateResources(tryTab);
    
    state->trystate = trystate;
    
    if (!catchis && !catchBlock)
    {
        state->SyntaxException(Exception::ERROR_syntax, line, "Attempt to create a try block without a catch block.");
    }
    if (catchis) catchis->CalculateResources(st);
    
    if (catchBlock)
    {
        PIKA_NEW(SymbolTable, catchTab, (st, ST_noinherit));
        symbol = catchTab->Put(caughtVar->name);
        symbol->offset = state->NextLocalOffset(caughtVar->name);
        
        state->trystate = trystate;
        state->trystate.inCatch = true;
        state->trystate.catchVarOffset = symbol->offset;
        
        catchBlock->CalculateResources(catchTab);
    }
    state->trystate = trystate;
    if (elseblock)
    {
        elseblock->CalculateResources(st);
    }
}

void RaiseStmt::DoStmtResources(SymbolTable* st)
{
    if (expr)
    {
        expr->CalculateResources(st);
    }
    else if (state->trystate.inCatch)
    {
        reraiseOffset = state->trystate.catchVarOffset;
    }
    else
    {
        state->SyntaxException(Exception::ERROR_syntax, line, "Attempt to re-raise outside of a catch block.");
    }
}

void DeclarationTarget::CreateSymbol(SymbolTable* st)
{
    const char* namestr = GetIdentifierName();
    if (st->IsDefined(namestr))
    {
        symbol = st->Shadow(namestr);
    }
    else
    {
        symbol = st->Put(namestr);
    }
    
    with = (st->IsWithBlock() && STO_none == storage) || STO_member == storage;
    
    if ((!st->DefaultsToGlobal() && STO_none == storage) || STO_local == storage)
    {
        symbol->isWith   = false;
        symbol->isGlobal = false;
        symbol->offset = state->NextLocalOffset(namestr);
    }
    else
    {
        symbol->isWith   = with;
        symbol->isGlobal = true;
        nameindex      = state->AddConstant(namestr);
    }
}

PropertyDecl::~PropertyDecl() {}

void PropertyDecl::CalculateResources(SymbolTable* st)
{
    NamedTarget::CalculateResources(st);
    
    PIKA_NEWNODE(StringExpr, name_expr, (state, Pika_strdup(name->GetName()), strlen(name->GetName())));
    
    name_expr->CalculateResources(st);
    
    if (getter) getter->CalculateResources(st);
    if (setter) setter->CalculateResources(st);
    
    NamedTarget::CalculateSymbols(st);
}

AssignmentStmt::AssignmentStmt(CompileState* s, ExprList* l, ExprList* r)
        : Stmt(s, Stmt::STMT_decl),
        left(l),
        right(r),
        isBinaryOp(false),
        isUnpack(false),
        isCall(false),
    unpackCount(0) {}
    
void AssignmentStmt::DoStmtResources(SymbolTable* st)
{
    ExprList* curr    = 0;
    size_t    num_lhs = 0;
    size_t    num_rhs = 0;
    
    // first do the rhs
    
    if (right)
    {
        ReverseExprList(&right);
        curr = right;
        
        while (curr)
        {
            ++num_rhs;
            curr->expr->CalculateResources(st);
            curr = curr->next;
        }
    }
    //
    // then do the lhs so that the rhs does not reference and lhs values.
    //
    curr = left;
    
    while (curr)
    {
        Expr::Kind k = curr->expr->kind;
        bool isskip = false;
        
        if (k == Expr::EXPR_load && (((LoadExpr*)curr->expr)->loadkind == LoadExpr::LK_null))
        {
            isskip = true;
        }
        
        //
        // We can only assign to identifiers and dot expressions (including brackets [].)
        //
        if (k != Expr::EXPR_identifier && k != Expr::EXPR_dot && !isskip)
        {
            state->SyntaxError(line, "illegal lhs value for assignment.");
            RaiseException(Exception::ERROR_syntax, "AssignmentStmt");
        }
        
        if (k == Expr::EXPR_identifier)
        {
            // Id Expression:
            //
            curr->expr->CalculateResources(st);
            IdExpr* idleft = (IdExpr*)curr->expr;
            //
            // We want to pick the right storage kind for an undefined identifier.
            //
            // If the symbol is already defined was it done in a symboltable other than the
            // current one ?
            bool parent_block_symbol = (idleft->symbol && idleft->symbol->table != st && st->IsWithBlock());
            
            if (!idleft->symbol || parent_block_symbol)
            {                
                // Create a symbol.
                // NOTE: We don't call shadow because the symbol does NOT exist in the current symboltable.
                idleft->symbol = st->Put(idleft->id->name);
                
                // Is this a with/class block ?
                idleft->symbol->isWith = st->IsWithBlock();
                
                if (!st->VarsAreGlobal())
                { 
                    // Create a new local variable.
                    idleft->symbol->isGlobal = false;
                    idleft->symbol->offset = state->NextLocalOffset(idleft->id->name);
                    idleft->index = idleft->symbol->offset;
                    idleft->newLocal = true;
                }
            }
        }
        else if (!isskip)
        {
            // Dot Expression.
            curr->expr->CalculateResources(st);
        }
        ++num_lhs;
        curr = curr->next;
    }
    if (num_lhs != num_rhs)
    {
        if (num_rhs == 1)
        {
            if (right->expr->kind == Expr::EXPR_call)
            {
                if (num_lhs >= PIKA_MAX_RETC)
                {
                    state->SyntaxError(line, "max number of return values (%d) reached .", PIKA_MAX_RETC);
                }
                isCall = true;
                ((CallExpr*)right->expr)->retc = num_lhs;
            }
            else
            {
                isUnpack    = true;
                unpackCount = static_cast<u2>(num_lhs);
            }
        }
        else
        {
            state->SyntaxError(line, "number of rhs expressions does not match number of lhs targets for assignment.");
            RaiseException(Exception::ERROR_syntax, "AssignmentStmt");
        }
    }
}

Opcode AssignmentStmt::GetOpcode() const
{
    switch (kind)
    {
    case ASSIGN_STMT:               return OP_nop;
    case BIT_OR_ASSIGN_STMT:        return OP_bitor;
    case BIT_XOR_ASSIGN_STMT:       return OP_bitxor;
    case BIT_AND_ASSIGN_STMT:       return OP_bitand;
    case LSH_ASSIGN_STMT:           return OP_lsh;
    case RSH_ASSIGN_STMT:           return OP_rsh;
    case URSH_ASSIGN_STMT:          return OP_ursh;
    case ADD_ASSIGN_STMT:           return OP_add;
    case SUB_ASSIGN_STMT:           return OP_sub;
    case MUL_ASSIGN_STMT:           return OP_mul;
    case DIV_ASSIGN_STMT:           return OP_div;
    case IDIV_ASSIGN_STMT:          return OP_idiv;
    case MOD_ASSIGN_STMT:           return OP_mod;
    case CONCAT_SPACE_ASSIGN_STMT:  return OP_catsp;
    case CONCAT_ASSIGN_STMT:        return OP_cat;
    };
    return OP_nop;
}

void VariableTarget::CalculateResources(SymbolTable* st)
{
    // First do the right-hand side expressions.
    
    size_t expr_count = 0;
    ExprList* ex = exprs;
    
    if (exprs)
        exprs->CalculateResources(st);
        
    while (ex)
    {
        ex = ex->next;
        ++expr_count;
    }
    
    // Last do the left-hand side declarations.
    // We do them last because the rhs cannot reference the lhs.
    
    size_t decl_count = 0;
    VarDecl* vd = decls;
    
    if (decls)
        decls->CalculateResources(st);
        
    while (vd)
    {
        vd = vd->next;
        ++decl_count;
    }
    
    if (decl_count != expr_count)
    {
        if (expr_count == 1)
        {
            if (exprs->expr->kind == Expr::EXPR_call)
            {
                if (decl_count >= PIKA_MAX_RETC)
                    state->SyntaxError(line, "Max number of return values reached");
                isCall = true;
                ((CallExpr*)exprs->expr)->retc = decl_count;
            }
            else
            {
                isUnpack = true;
                unpackCount = static_cast<u2>(decl_count);
            }
        }
        else
        {
            state->SyntaxError(line, "number of rhs expressions does not match number of lhs targets for variable assignment.");
            RaiseException(Exception::ERROR_syntax, "VariableTarget");
        }
    }
}

FinallyStmt::FinallyStmt(CompileState* s, Stmt* block, Stmt* finalize_block)
        : Stmt(s, Stmt::STMT_finallyblock),
        block(block),
        symtab(0),
        finalize_block(finalize_block) {}
        
FinallyStmt::~FinallyStmt() { Pika_delete(symtab); }

void FinallyStmt::DoStmtResources(SymbolTable* st)
{
    if (block)
        block->CalculateResources(st);
        
    PIKA_NEW(SymbolTable, symtab, (st, ST_noinherit));
    
    if (finalize_block)
        finalize_block->CalculateResources(symtab);
}

UsingStmt::UsingStmt(CompileState* s, Expr* e, Stmt* b)
        : Stmt(s, Stmt::STMT_with),
        with(e),
        block(b),
        symtab(0)
{}

UsingStmt::~UsingStmt() { Pika_delete(symtab); }

void UsingStmt::DoStmtResources(SymbolTable* st)
{
    PIKA_NEW(SymbolTable, symtab, (st, ST_using));
    
    if (with)
        with->CalculateResources(st);
        
    if (block)
        block->CalculateResources(symtab);
}

PkgDecl::PkgDecl(CompileState* s, NameNode* nnode, Stmt* body, StorageKind sto)
        : NamedTarget(s, nnode, DECL_package, sto),
        symtab(0),
        id(0),
    body(body) {}
    
PkgDecl::~PkgDecl() { Pika_delete(symtab); }

void PkgDecl::CalculateResources(SymbolTable* st)
{
    NamedTarget::CalculateResources(st);
    PIKA_NEWNODE(StringExpr, id, (state, Pika_strdup(name->GetName()), strlen(name->GetName())));
    
    id->CalculateResources(st);
    
    PIKA_NEW(SymbolTable, symtab, (st, ST_package));
    
    body->CalculateResources(symtab);
    
    NamedTarget::CalculateSymbols(st);
}

void NameNode::CalculateResources(SymbolTable* st)
{
    if (idexpr)
    {
        idexpr->newLocal = true;
        idexpr->CalculateResources(st);
    }
    else if (dotexpr)
    {
        dotexpr->CalculateResources(st);
    }
}

const char* NameNode::GetName()
{
    if (idexpr)
    {
        return idexpr->id->name;
    }
    else if (dotexpr)
    {
        if (dotexpr->right->kind == Expr::EXPR_member)
        {
            MemberExpr* me = (MemberExpr*)dotexpr->right;
            return me->id->name;
        }
        else
        {
            return "";
        }
    }
    SHOULD_NEVER_HAPPEN();
    return 0;
}

ClassDecl::~ClassDecl()
{
    Pika_delete(symtab);    
}

void ClassDecl::CalculateResources(SymbolTable* st)
{
    NamedTarget::CalculateResources(st);
    PIKA_NEWNODE(StringExpr, stringid, (state, Pika_strdup(name->GetName()), strlen(name->GetName())));
    
    stringid->CalculateResources(st);
    
    if (super) super->CalculateResources(st);
    
    NamedTarget::CalculateSymbols(st);
    
    if (stmts)
    {
        PIKA_NEW(SymbolTable, symtab, (st, ST_package));
        stmts->CalculateResources(symtab);
    }
}

}// pika

