/*
 *  PAst.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PAst.h"
#include "PSymbolTable.h"

namespace pika
{

namespace
{

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
    PIKA_NEW(SymbolTable, (*symtab), (st, false, false, true));
    
    if ((*symtab)->IncrementDepth() > PIKA_MAX_NESTED_FUNCTIONS)
    {
        cs.SyntaxError(funcline, "max nested functions depth reached %d.", PIKA_MAX_NESTED_FUNCTIONS);
        RaiseException(Exception::ERROR_syntax, "DotExpr");
    }
    
    if (args)
    {
        args->CalculateDefaultResources(st, cs);
        args->CalculateResources((*symtab), cs);
        (*def)->numArgs = cs.localCount;
    }
    else
    {
        (*def)->numArgs = 0;
    }
    
    if (body)
    {
        body->CalculateResources((*symtab), cs);
    }
    (*def)->numLocals = cs.localCount;
    
    // pop offsets
    cs.trystate = oldTryState;
    cs.SetLocalOffset(oldOffset);
    cs.currDef = (*def)->parent;
    cs.localCount = oldLocalCount;
    cs.SetLineInfo(prevLine);
}

}//local namespace

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
        parser(0)
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

int CompileState::NextLocalOffset(const char* name)
{
    int off = localOffset;
    
    if (++localOffset > localCount)
    {
        localCount = localOffset;
    }
    if (currDef)
    {
        currDef->AddLocalVar(engine, name);
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
#endif
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

Id::Id(char *name): name(name), next(0) {}

Id::~Id() { Pika_free(name); }

void Program::CalculateResources(SymbolTable* st, CompileState& cs)
{
    symtab = 0;
    PIKA_NEW(SymbolTable, symtab, (st, true));
    
    cs.literals = LiteralPool::Create(cs.engine);
    
    def = Def::Create(cs.engine);
    
    def->numLocals = 0;
    def->mustClose = false;
    def->name = cs.engine->AllocString("__main");
    def->literals = cs.literals;
    
    index = cs.AddConstant(def);
    cs.currDef = def;
    
    stmts->CalculateResources(symtab, cs);
    
    def->numLocals = cs.localCount; // even though a program defaults to global it might still have lexEnv!
}

void FunctionProg::CalculateResources(SymbolTable* st, CompileState& cs)
{
    symtab = 0;
    PIKA_NEW(SymbolTable, symtab, (st, false));
    
    cs.literals = LiteralPool::Create(cs.engine);
    
    Pika_FunctionResourceCalculation(line, st, cs,
                                      args, stmts,
                                      &index,
                                      &def,
                                      &symtab,
                                      "(main)");
}

Program::~Program()
{
    Pika_delete(symtab);
}

const char* NamedTarget::GetIdentifierName()
{
    return name->GetName();
}

void NamedTarget::CalculateResources(SymbolTable* st, CompileState& cs)
{
    CreateSymbol(st, cs);
    name->CalculateResources(st, cs);
}

FunctionDecl::FunctionDecl(NameNode* name, ParamDecl* args, Stmt* body,
                           size_t begtxt, size_t endtxt,
                           StorageKind sk)
        : DeclarationTarget(Decl::DECL_function, sk),
        def(0),
        index(0),
        args(args),
        body(body),
        symtab(0),
        name(name),
        varArgs(false),
        scriptBeg(begtxt),
        scriptEnd(endtxt)
{
    varArgs = (args) ? args->HasVarArgs() : false;
}

const char* FunctionDecl::GetIdentifierName() { return name->GetName(); }

void FunctionDecl::CalculateResources(SymbolTable* st, CompileState& cs)
{
    CreateSymbol(st, cs);
    name->CalculateResources(st, cs);
    
    Pika_FunctionResourceCalculation(line, st, cs,
                                      args, body,
                                      &index,
                                      &def,
                                      &symtab,
                                      name->GetName());
                                      
    def->isVarArg = varArgs ? 1 : 0;
}

FunctionDecl::~FunctionDecl() { Pika_delete(symtab); }

ParamDecl::ParamDecl(Id *name, bool rest, Expr* val)
        : Decl(Decl::DECL_parameter),
        val(val),
        symbol(0),
        name(name),
        has_rest(rest),
        next(0) {}
        
void ParamDecl::CalculateDefaultResources(SymbolTable* st, CompileState& cs)
{
    if (val) val->CalculateResources(st, cs);
    if (next) next->CalculateDefaultResources(st, cs);
}

void ParamDecl::CalculateResources(SymbolTable* st, CompileState& cs)
{
    if (st->IsDefined(name->name))
    {
        symbol = st->Shadow(name->name);
    }
    else
    {
        symbol = st->Put(name->name);
    }
    
    symbol->offset = cs.NextLocalOffset(name->name);
    
    if (next)
    {
        next->CalculateResources(st, cs);
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
    if (next)
        return next->HasVarArgs();
    return has_rest;
}

void LocalDecl::CalculateResources(SymbolTable* st, CompileState& cs)
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
        symbol->offset = cs.NextLocalOffset(name->name);
    }
    
    if (next)
        next->CalculateResources(st, cs);
}

void MemberDeclaration::CalculateResources(SymbolTable* st, CompileState& cs)
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
    nameIndex = cs.AddConstant(name->name);
    
    if (next)
        next->CalculateResources(st, cs);
}

void VarDecl::CalculateResources(SymbolTable* st, CompileState& cs)
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
    nameIndex = cs.AddConstant(name->name);
    
    if (next)
        next->CalculateResources(st, cs);
}

void BreakStmt::DoStmtResources(SymbolTable* st, CompileState& cs)
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

void ContinueStmt::DoStmtResources(SymbolTable* st, CompileState& cs)
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

void LabeledStmt::DoStmtResources(SymbolTable* st, CompileState& cs)
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
        stmt->CalculateResources(st, cs);
    }
}

void ExprStmt::DoStmtResources(SymbolTable* st, CompileState& cs)
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
                    expr->CalculateResources(st, cs);
                    break;
                    
                default:
                    state->SyntaxException(Exception::ERROR_syntax, expr->line, "Only assignment, call, increment, decrement and new expressions can be used as a statement.");
                    break;
                };
            }
        }
        else
        {
            expr->CalculateResources(st, cs);
        }
        curr = curr->next;
    }
}

void RetStmt::DoStmtResources(SymbolTable* st, CompileState& cs)
{
    count = 0;
    ExprList* curr = exprs;
    isInTry = cs.trystate.inTry;
    while (curr)
    {
        ++count;
        curr->expr->CalculateResources(st, cs);
        curr = curr->next;
        if (count >= PIKA_MAX_RETC)
        {
            state->SyntaxException(Exception::ERROR_syntax, line,
                                   "max number of return values exceeded (max = %d).",
                                   PIKA_MAX_RETC);
        }
    }
}

void YieldStmt::DoStmtResources(SymbolTable* st, CompileState& cs)
{
    count = 0;
    ExprList* curr = exprs;
    while (curr)
    {
        ++count;
        curr->expr->CalculateResources(st, cs);
        curr = curr->next;
        if (count >= PIKA_MAX_RETC)
        {
            state->SyntaxException(Exception::ERROR_syntax, line,
                                   "max number of yield values exceeded (max = %d).",
                                   PIKA_MAX_RETC);
        }
    }
}

void LoopStmt::DoStmtResources(SymbolTable* st, CompileState& cs)
{
    if (body)
    {
        body->CalculateResources(st, cs);
    }
}

ForToStmt::~ForToStmt() { Pika_delete(symtab); }

void ForToStmt::DoStmtResources(SymbolTable* st, CompileState& cs)
{
    PIKA_NEW(SymbolTable, symtab, (st, st->IsWithBlock()));
    
    // Create the looping variable's symbol along to extra local variables
    // (for 'to' and 'by'.)
    symbol = state->CreateLocalPlus(symtab, id->name, 2);
    
    from->CalculateResources(symtab, cs);
    to  ->CalculateResources(symtab, cs);
    step->CalculateResources(symtab, cs);
    body->CalculateResources(symtab, cs);
}

ForeachStmt::~ForeachStmt()
{
    Pika_delete(symtab);
}

void ForeachStmt::DoStmtResources(SymbolTable* st, CompileState& cs)
{
    PIKA_NEW(SymbolTable, symtab, (st, st->IsWithBlock()));
    PIKA_NEWNODE(LocalDecl, iterVar, (id));
    iterVar->line = id->line;
    PIKA_NEWNODE(IdExpr, idexpr, (id));
    
    
    idexpr->line = iterVar->name->line;
    enum_offset = cs.NextLocalOffset("");
    
    type_expr->CalculateResources(symtab, cs);
    iterVar->CalculateResources(symtab, cs);
    idexpr->CalculateResources(symtab, cs);
    in->CalculateResources(symtab, cs);
    body->CalculateResources(symtab, cs);
}

void CondLoopStmt::DoStmtResources(SymbolTable* st, CompileState& cs)
{
    if (cond) cond->CalculateResources(st, cs);
    if (body) body->CalculateResources(st, cs);
}

void IfStmt::DoStmtResources(SymbolTable* st, CompileState& cs)
{
    if (cond)       cond     ->CalculateResources(st, cs);
    if (then_part)  then_part->CalculateResources(st, cs);
    if (next)       next     ->CalculateResources(st, cs);
}

void BlockStmt::DoStmtResources(SymbolTable* st, CompileState& cs)
{
    symtab = 0;
    PIKA_NEW(SymbolTable, symtab, (st, st->IsWithBlock()));
    
    if (stmts)
    {
        stmts->CalculateResources(symtab, cs);
    }
}

BlockStmt::~BlockStmt() { Pika_delete(symtab); }

DeclStmt::DeclStmt(Decl *decl): Stmt(Stmt::STMT_decl), decl(decl) {}

void DeclStmt::DoStmtResources(SymbolTable* st, CompileState& cs)
{
    if (decl)
    {
        decl->CalculateResources(st, cs);
    }
}

ParenExpr::~ParenExpr() {}
    
void ParenExpr::CalculateResources(SymbolTable* st, CompileState& cs)
{
    expr->CalculateResources(st,cs);
}

void CallExpr::CalculateResources(SymbolTable* st, CompileState& cs)
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
        left->CalculateResources(st, cs);
    }
    if (args) args->CalculateResources(st, cs);
}

void BinaryExpr::CalculateResources(SymbolTable* st, CompileState& cs)
{
    left ->CalculateResources(st, cs);
    right->CalculateResources(st, cs);
}

void IdExpr::CalculateResources(SymbolTable* st, CompileState& cs)
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
        Def* currDef = cs.currDef;
        
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
        index = cs.AddConstant(id->name);
    }
}

void MemberExpr::CalculateResources(SymbolTable* st, CompileState& cs)
{
    index = cs.AddConstant(id->name);
}

StringExpr::~StringExpr() { Pika_free(string); }

void StringExpr::CalculateResources(SymbolTable* st, CompileState& cs)
{
    //if (!length)
    //    length = strlen(string);
    
    index = cs.AddConstant(string, length);
}

void IntegerExpr::CalculateResources(SymbolTable* st, CompileState& cs)
{
    index = cs.AddConstant(integer);
}

void RealExpr::CalculateResources(SymbolTable* st, CompileState& cs)
{
    index = cs.AddConstant(real);
}

DotExpr::DotExpr(Expr *l, Expr *r) : BinaryExpr(Expr::EXPR_dot, l, r) {}

void DotExpr::CalculateResources(SymbolTable* st, CompileState& cs)
{
    BinaryExpr::CalculateResources(st, cs);
}

FunExpr::~FunExpr() { Pika_delete(symtab); }

void FunExpr::CalculateResources(SymbolTable* st, CompileState& cs)
{
    Pika_FunctionResourceCalculation(line, st, cs, args, body, &index, &def, &symtab, (name) ? name->name : 0);
    def->isVarArg = (args) ? args->HasVarArgs() : false;
}

void PropExpr::CalculateResources(SymbolTable* st, CompileState& cs)
{
    if (nameexpr) nameexpr->CalculateResources(st, cs);
    if (getter)   getter  ->CalculateResources(st, cs);
    if (setter)   setter  ->CalculateResources(st, cs);
}

void ExprList::CalculateResources(SymbolTable* st, CompileState& cs)
{
    if (expr) expr->CalculateResources(st, cs);
    if (next) next->CalculateResources(st, cs);
}

void UnaryExpr::CalculateResources(SymbolTable* st, CompileState& cs)
{
    if (expr)
    {
        expr->CalculateResources(st, cs);
        
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

void DictionaryExpr::CalculateResources(SymbolTable* st, CompileState& cs)
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
                PIKA_NEWNODE(Id, fun->name, (Pika_strdup( strexpr->string )));
            }
        }
        */
        if (curr->name)
        {
            curr->name->CalculateResources(st, cs);
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
            curr->value->CalculateResources(st, cs);
        }
        
        curr = curr->next;
    }
}

void LoadExpr::CalculateResources(SymbolTable* st, CompileState& cs)
{
#if defined(PIKA_LOCALS_IMPLIES_CLOSE)
    if (loadkind == LK_locals)
    {
        Def* curr = cs.currDef->parent;
        
        while (curr)
        {
            curr->mustClose = true;
            curr = curr->parent;
        }
    }
#endif
}

void ArrayExpr::CalculateResources(SymbolTable* st, CompileState& cs)
{
    if (elements) elements->CalculateResources(st, cs);
}

void CondExpr::CalculateResources(SymbolTable* st, CompileState& cs)
{
    if (cond)  cond ->CalculateResources(st, cs);
    if (exprA) exprA->CalculateResources(st, cs);
    if (exprB) exprB->CalculateResources(st, cs);
}

void NullSelectExpr::CalculateResources(SymbolTable* st, CompileState& cs)
{
    if (exprA) exprA->CalculateResources(st, cs);
    if (exprB) exprB->CalculateResources(st, cs);
}

SliceExpr::~SliceExpr() {}

void SliceExpr::CalculateResources(SymbolTable* st, CompileState& cs)
{
    Id* id = 0;
    char *slice = Pika_strdup(OPSLICE_STR);
    
    PIKA_NEWNODE(Id, id, (slice));
    PIKA_NEWNODE(MemberExpr, slicefun, (id));
    
    
    slicefun->line = id->line = expr->line;
    
    slicefun->CalculateResources(st, cs);
    
    if (expr) expr->CalculateResources(st, cs);
    if (from) from->CalculateResources(st, cs);
    if (to)   to  ->CalculateResources(st, cs);
}

CatchIsBlock::~CatchIsBlock()
{
    Pika_delete(symtab);
}

void CatchIsBlock::CalculateResources(SymbolTable* st, CompileState& cs)
{
    CompileState::TryState trystate = cs.trystate;
    
    PIKA_NEW(SymbolTable, symtab, (st, false, false, false, false));
    
    symbol = symtab->Put(catchvar->name);
    symbol->offset = cs.NextLocalOffset(catchvar->name);
    
    cs.trystate = trystate;
    cs.trystate.inCatch = true;
    cs.trystate.catchVarOffset = symbol->offset;
    
    isexpr->CalculateResources(symtab, cs);
    block->CalculateResources(symtab, cs);
    
    cs.trystate = trystate;
    
    if (next) next->CalculateResources(st, cs);
}

TryStmt::~TryStmt()
{
    Pika_delete(tryTab);
    Pika_delete(catchTab);
}

void TryStmt::DoStmtResources(SymbolTable* st, CompileState& cs)
{
    CompileState::TryState trystate = cs.trystate;
    
    PIKA_NEW(SymbolTable, tryTab, (st, st->IsWithBlock()));
    
    cs.trystate.inTry = true;
    
    tryBlock->CalculateResources(tryTab, cs);
    
    cs.trystate = trystate;
    
    if (!catchis && !catchBlock)
    {
        state->SyntaxException(Exception::ERROR_syntax, line, "Attempt to create a try block without a catch block.");
    }
    if (catchis) catchis->CalculateResources(st, cs);
    
    if (catchBlock)
    {
        PIKA_NEW(SymbolTable, catchTab, (st, false, false, false, false));
        symbol = catchTab->Put(caughtVar->name);
        symbol->offset = cs.NextLocalOffset(caughtVar->name);
        
        cs.trystate = trystate;
        cs.trystate.inCatch = true;
        cs.trystate.catchVarOffset = symbol->offset;
        
        catchBlock->CalculateResources(catchTab, cs);
    }
    cs.trystate = trystate;
    if (elseblock)
    {
        elseblock->CalculateResources(st, cs);
    }
}

void RaiseStmt::DoStmtResources(SymbolTable* st, CompileState& cs)
{
    if (expr)
    {
        expr->CalculateResources(st, cs);
    }
    else if (cs.trystate.inCatch)
    {
        reraiseOffset = cs.trystate.catchVarOffset;
    }
    else
    {
        state->SyntaxException(Exception::ERROR_syntax, line, "Attempt to re-raise outside of a catch block.");
    }
}

void DeclarationTarget::CreateSymbol(SymbolTable* st, CompileState& cs)
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
        symbol->offset = cs.NextLocalOffset(namestr);
    }
    else
    {
        symbol->isWith   = with;
        symbol->isGlobal = true;
        nameindex      = cs.AddConstant(namestr);
    }
}

PropertyDecl::~PropertyDecl() {}

void PropertyDecl::CalculateResources(SymbolTable* st, CompileState& cs)
{
    PIKA_NEWNODE(StringExpr, name_expr, (Pika_strdup(name->GetName()), strlen(name->GetName())));
    
    name_expr->CalculateResources(st, cs);
    
    if (getter) getter->CalculateResources(st, cs);
    if (setter) setter->CalculateResources(st, cs);
    
    NamedTarget::CalculateResources(st, cs);
}

AssignmentStmt::AssignmentStmt(ExprList* l, ExprList* r)
        : Stmt(Stmt::STMT_decl),
        left(l),
        right(r),
        isBinaryOp(false),
        isUnpack(false),
        isCall(false),
    unpackCount(0) {}
    
void AssignmentStmt::DoStmtResources(SymbolTable* st, CompileState& cs)
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
            curr->expr->CalculateResources(st, cs);
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
            curr->expr->CalculateResources(st, cs);
            IdExpr* idleft = (IdExpr*)curr->expr;
            //
            // We want to pick the right storage kind for an undefined identifier.
            //
            // If the symbol is already defined was it done in a symboltable other than the
            // current one ?
            bool parent_block_symbol = (idleft->symbol && idleft->symbol->table != st && st->IsWithBlock());
            
            if (!idleft->symbol || parent_block_symbol)
            { // Not defined or defined elsewhere
                // Do variables default to the global scope for this block ?
                bool def_global = st->DefaultsToGlobal();
                
                // Create a symbol.
                // NOTE: We don't call shadow because the symbol does NOT exist in the current symboltable.
                idleft->symbol = st->Put(idleft->id->name);
                
                // Is this a with/class block ?
                idleft->symbol->isWith = st->IsWithBlock();
                
                if (!def_global)
                { // Create a new local variable.
                    idleft->symbol->isGlobal = false;
                    idleft->symbol->offset = cs.NextLocalOffset(idleft->id->name);
                    idleft->index = idleft->symbol->offset;
                    idleft->newLocal = true;
                }
            }
        }
        else if (!isskip)
        {
            // Dot Expression.
            curr->expr->CalculateResources(st, cs);
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
    case MOD_ASSIGN_STMT:           return OP_mod;
    case CONCAT_SPACE_ASSIGN_STMT:  return OP_catsp;
    case CONCAT_ASSIGN_STMT:        return OP_cat;
    };
    return OP_nop;
}

void VariableTarget::CalculateResources(SymbolTable* st, CompileState& cs)
{
    // First do the right-hand side expressions.
    
    size_t expr_count = 0;
    ExprList* ex = exprs;
    
    if (exprs)
        exprs->CalculateResources(st, cs);
        
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
        decls->CalculateResources(st, cs);
        
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

FinallyStmt::FinallyStmt(Stmt* block, Stmt* ensured_block)
        : Stmt(Stmt::STMT_ensureblock),
        block(block),
        symtab(0),
        ensured_block(ensured_block) {}
        
FinallyStmt::~FinallyStmt() { Pika_delete(symtab); }

void FinallyStmt::DoStmtResources(SymbolTable* st, CompileState& cs)
{
    if (block)
        block->CalculateResources(st, cs);
        
    PIKA_NEW(SymbolTable, symtab, (st, false, false, false, false));
    
    if (ensured_block)
        ensured_block->CalculateResources(symtab, cs);
}

WithStatement::WithStatement(Expr* e, Stmt* b)
        : Stmt(Stmt::STMT_with),
        with(e),
        block(b),
        symtab(0)
{}

WithStatement::~WithStatement() { Pika_delete(symtab); }

void WithStatement::DoStmtResources(SymbolTable* st, CompileState& cs)
{
    PIKA_NEW(SymbolTable, symtab, (st, true, true));
    
    if (with)
        with->CalculateResources(st, cs);
        
    if (block)
        block->CalculateResources(symtab, cs);
}

PkgDecl::PkgDecl(NameNode* nnode, Stmt* body, StorageKind sto)
        : NamedTarget(nnode, DECL_package, sto),
        symtab(0),
        id(0),
    body(body) {}
    
PkgDecl::~PkgDecl() { Pika_delete(symtab); }

void PkgDecl::CalculateResources(SymbolTable* st, CompileState& cs)
{
    PIKA_NEWNODE(StringExpr, id, (Pika_strdup(name->GetName()), strlen(name->GetName())));
    
    id->CalculateResources(st, cs);
    
    PIKA_NEW(SymbolTable, symtab, (st, true, false, false, false));
    
    body->CalculateResources(symtab, cs);
    
    NamedTarget::CalculateResources(st, cs);
}

CaseList::CaseList(ExprList* matches, Stmt* body) : matches(matches), body(body) {}

void CaseList::DoResources(SymbolTable* st, CompileState& cs)
{
    matches->CalculateResources(st, cs);
    body->CalculateResources(st, cs);
    
    if (next) next->DoResources(st, cs);
}

CaseStmt::CaseStmt(Expr* selector, CaseList* cases, Stmt* elsebody)
        : Stmt(Stmt::STMT_case),
        selector(selector),
        cases(cases),
    elsebody(elsebody) {}
    
void CaseStmt::DoStmtResources(SymbolTable* st, CompileState& cs)
{
    if (!cases && !elsebody) return;
    
    selector->CalculateResources(st, cs);
    
    if (cases)    cases->DoResources(st, cs);
    if (elsebody) elsebody->CalculateResources(st, cs);
}

void NameNode::CalculateResources(SymbolTable* st, CompileState& cs)
{
    if (idexpr)
    {
        idexpr->newLocal = true;
        idexpr->CalculateResources(st, cs);
    }
    else if (dotexpr)
    {
        dotexpr->CalculateResources(st, cs);
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
    }
    SHOULD_NEVER_HAPPEN();
    return 0;
}

void AssertStmt::DoStmtResources(SymbolTable* st, CompileState& cs)
{
    if (expr) expr->CalculateResources(st, cs);
}

ClassDecl::~ClassDecl()
{
    Pika_delete(symtab);
    
}

void ClassDecl::CalculateResources(SymbolTable* st, CompileState& cs)
{
    PIKA_NEWNODE(StringExpr, stringid, (Pika_strdup(name->GetName()), strlen(name->GetName())));
    
    stringid->CalculateResources(st, cs);
    
    if (super) super->CalculateResources(st, cs);
    
    NamedTarget::CalculateResources(st, cs);
    
    if (stmts)
    {
        PIKA_NEW(SymbolTable, symtab, (st, true, false, false, false));
        stmts->CalculateResources(symtab, cs);
    }
}

}// pika

