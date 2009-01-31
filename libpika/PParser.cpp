/*
*  gParser.cpp
*  See Copyright Notice in Pika.h
*/
#include "Pika.h"
#include "PSymbolTable.h"
#include "PAst.h"
#include "PParser.h"
#include "PInstruction.h"

namespace pika
{

/* Returns true if the token 'tok' IS NOT a Terminator from the array 'terms'. */

INLINE bool IsNotTerm(const int* terms, const int tok)
{
    for (const int* c = terms; *c; ++c)
    {
        if (tok == *c)
        {
            return false;
        }
    }
    return true;
}

/* Returns true if the token 'tok' IS a Terminator from the array 'terms'. */

INLINE bool IsTerm(const int* terms, const int tok)
{
    for (const int* c = terms; *c; ++c)
    {
        if (tok == *c)
        {
            return true;
        }
    }
    return false;
}

const int Parser::Static_end_terms[]     = { TOK_end, 0 };
const int Parser::Static_finally_terms[]  = { TOK_end, TOK_finally, 0 };

///////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef TOKEN_DEF
#undef TOKEN_DEF
#endif

#define TOKEN_DEF(XTOK, XNAME) XNAME,

/*  Returns the name (string representation) of each token.
 *  This is made into a function so that we can guarantee static intialization of the array is
 *  performed before it is needed.
 */
const char** Token2String::GetNames()
{
    static const char* names[] =
    {
#       include "PToken.def"
    };
    return names;
}

const int Token2String::min  = TOK_global;
const int Token2String::max  = TOK_max;
const int Token2String::diff = TOK_global;


// TokenStream /////////////////////////////////////////////////////////////////////////////////////

TokenStream::TokenStream(CompileState* state, FILE* yyin)
        : state(state),
        yyin(yyin),
        tokenizer(0)
{
    prev.tokenType = curr.tokenType = next.tokenType = EOF;
    PIKA_NEW(Tokenizer, tokenizer, (state, yyin));
}

TokenStream::TokenStream(CompileState* state, const char* buffer, size_t len)
        : state(state),
        yyin(0),
        tokenizer(0)
{
    prev.tokenType = curr.tokenType = next.tokenType = EOF;
    PIKA_NEW(Tokenizer, tokenizer, (state, buffer, len));
}

TokenStream::~TokenStream()
{
    Pika_delete(tokenizer);
}

void TokenStream::Initialize()
{
    // Get current and next
    Advance();
    Advance();
}

void TokenStream::Advance()
{
    prev = curr;
    curr = next;
    
    tokenizer->GetNext();
    int nxttok = tokenizer->GetTokenType();
    
    if (nxttok != EOF)
    {
        next.tokenType = nxttok;
        next.line      = tokenizer->GetLn();
        next.col       = tokenizer->GetCol();
        next.begOffset = tokenizer->GetBeginOffset();
        next.endOffset = tokenizer->GetEndOffset();
        next.value     = tokenizer->GetVal();
    }
    else
    {
        next.tokenType = EOF;
    }
}

// Parser //////////////////////////////////////////////////////////////////////////////////////////

Parser::Parser(CompileState *cs, FILE* yyin)
        : root(0),
        state(cs),
        tstream(state, yyin)
{
    state->SetParser(this);
}

Parser::Parser(CompileState *cs, const char* buffer, size_t len)
        : root(0),
        state(cs),
        tstream(state, buffer, len)
{
    state->SetParser(this);
}

Parser::~Parser() {}

Program* Parser::DoParse()
{
    tstream.Initialize();
    
    DoScript();
    
    return root;
}

Program* Parser::DoFunctionParse()
{
    tstream.Initialize();
    
    DoFunctionScript();
    
    return root;
}

void Parser::DoScript()
{
    const int terms[] = {0};
    
    size_t beg   = tstream.curr.begOffset;
    Stmt*  stmts = DoStatementList(terms);
    size_t end   = tstream.prev.endOffset;
    
    PIKA_NEWNODE(Program, root, (stmts, beg, end));
}

void Parser::DoFunctionScript()
{
    const int terms[] = {TOK_end};
    
    Match(TOK_function);
    
    if (tstream.GetType() != '(')
        Expected('(');
        
    ParamDecl* args = DoFunctionArguments();
    
    size_t beg   = tstream.curr.begOffset;
    Stmt*  stmts = DoStatementList(terms);
    size_t end   = tstream.prev.endOffset;
    
    Match(TOK_end);
    PIKA_NEWNODE(FunctionProg, root, (stmts, beg, end, args));
}

// Checks for the desired token. If present the token stream is advanced.
// Otherwise an exception is thrown.

void Parser::Match(int x)
{
    if (tstream.GetType() == x)
        tstream.Advance();
    else
        Expected(x);
}

// Checks for the desired token. If present the token stream is advanced
// and true is returned. Otherwise false is returned and the token stream
// remains untouched.

bool Parser::Optional(int x)
{
    if (tstream.GetType() == x)
    {
        tstream.Advance();
        return true;
    }
    return false;
}

// Throws an exception when the expected token (x) is not encountered.
// NOTE: the presence of token (x) is not actually tested use Match or
// Optional to do that.

void Parser::Expected(int x)
{
    int line = tstream.GetLineNumber();
    
    if (x >= Token2String::min && x <= Token2String::max)
    {
        state->SyntaxException(Exception::ERROR_syntax, line, "Expected token '%s'.", Token2String::GetNames()[x-Token2String::diff]);
    }
    else if (x <= CHAR_MAX && x >= CHAR_MIN)
    {
        state->SyntaxException(Exception::ERROR_syntax, line, "Expected token '%c'.", (char)x);
    }
    else
    {
        state->SyntaxException(Exception::ERROR_syntax, line, "Expected token %d.", x);
    }
}

// Throws an exception because an un-expected token (x) is encountered.

void Parser::Unexpected(int x)
{
    int line = tstream.GetLineNumber();
    
    if (x >= Token2String::min && x <= Token2String::max)
    {
        state->SyntaxException(Exception::ERROR_syntax, line, "unexpected token '%s'.", Token2String::GetNames()[x-Token2String::diff]);
    }
    else if (x <= CHAR_MAX && x >= CHAR_MIN)
    {
        state->SyntaxException(Exception::ERROR_syntax, line, "unexpected token '%c'.", (char)x);
    }
    else
    {
        state->SyntaxException(Exception::ERROR_syntax, line, "unexpected token %d.", x);
    }
}

// Checks for a valid end of statement token: end, ';', '\n', '}' or EOF.
// This is basically where optional semicolons are implemented.
//
// The token stream is advanced iff the current token is ';'.

void Parser::DoEndOfStatement()
{
    if (IsEndOfStatement())
    {
        Optional(';');   // grab the semicolon if its there.
    }
    else
    {
        state->SyntaxException(Exception::ERROR_syntax,
                               tstream.GetLineNumber(),
                               "end of statement required. expected ';' or \\n");
    }
}

// This method behaves much like DoEndOfStatement, but for commas.
// Could be used so that function parameters,arguments or array elements are
// seperated by either
// newlines or commas
//
// ie x = [ 1
//          2
//          3 ]
//
// instead of x = [ 1, 2, 3 ]

bool Parser::DoCommaOrNewline()
{
    if (tstream.GetType() == ',')
    {
        tstream.Advance();
        return true;
    }
    else
    {
        int currline = tstream.GetLineNumber();
        int nextline = tstream.GetPreviousLineNumber();
        
        if (currline == nextline && !tstream.IsEndOfStream())
        {
            return false;
        }
    }
    return true;
}


// Determines if DoEndOfStatement will succeed. The token stream will not be
// advanced.

bool Parser::IsEndOfStatement()
{
    if (tstream.GetType() == ';')
    {
        return true;
    }
    else if (tstream.GetType() != TOK_end && tstream.GetType() != '}')
    {
        int currline = tstream.GetLineNumber();
        int nextline = tstream.GetPreviousLineNumber();
        
        if (currline == nextline && !tstream.IsEndOfStream())
        {
            return false;
        }
    }
    return true;
}

Stmt* Parser::DoStatement(bool skipExpr)
{
    Stmt* stmt = 0;
    int line = tstream.GetLineNumber();
    // TODO:: Order this in ascending order.
    
    switch (tstream.GetType())
    {
    case ';':
    {
        Match(';');
        PIKA_NEWNODE(EmptyStmt, stmt, ());
        stmt->line = line;
    }
    break;
    case TOK_with:          stmt = DoWithStatement();      break;
    case TOK_property:      stmt = DoPropertyStatement();  break;
    case TOK_try:           stmt = DoTryStatement();       break;
    case TOK_raise:         stmt = DoRaiseStatement();     break;
    case TOK_identifier:    stmt = DoLabeledStatement();   break; // possible label statement
    case TOK_global:
    case TOK_local:
    case TOK_member:
    {
        int next_type = tstream.GetNextType();
        switch (next_type)
        {
        case TOK_function:  stmt = DoFunctionStatement();   break;
        case TOK_property:  stmt = DoPropertyStatement();   break;
        case TOK_package:   stmt = DoPackageDeclaration();  break;
        case TOK_class:     stmt = DoClassStatement();      break;
        default:            stmt = DoVarStatement();        break;
        }
    }
    break;
    case TOK_function:      stmt = DoFunctionStatement();  break;
    case TOK_if:
    case TOK_unless:        stmt = DoIfStatement();        break;
    case TOK_while:         stmt = DoWhileStatement();     break;
    case TOK_until:         stmt = DoUntilStatement();     break;
    case TOK_loop:          stmt = DoLoopStatement();      break;
    case TOK_foreach:       stmt = DoForEachStatement();   break;
    case TOK_for:           stmt = DoForToStatement();     break;
    case TOK_return:        stmt = DoReturnStatement();    break;
    case TOK_yield:         stmt = DoYieldStatement();     break;
    case TOK_gen:           stmt = DoGenStatement();       break;
    case TOK_break:         stmt = DoBreakStatement();     break;
    case TOK_continue:      stmt = DoContinueStatement();  break;
    case TOK_begin:         stmt = DoBlockStatement();     break;
    case TOK_package:       stmt = DoPackageDeclaration(); break;
    case TOK_select:        stmt = DoCaseStatement();      break;
    case TOK_assert:        stmt = DoAssertStatement();    break;
    case TOK_class:         stmt = DoClassStatement();     break;
    case TOK_delete:
    {
        DeleteStmt* delstmt = 0;
        tstream.Advance();
        do
        {
            int nxtline = tstream.GetLineNumber();
            Expr* operand = DoPrefixExpression();
            
            if (operand->kind != Expr::EXPR_dot)
            {
                state->SyntaxException(Exception::ERROR_syntax,
                                       tstream.GetLineNumber(),
                                       "bind must be followed by a dot expression.");
            }
            
            DotExpr* dotexpr = (DotExpr*)operand;
            
            DeleteStmt* newstmt = 0;
            PIKA_NEWNODE(DeleteStmt, newstmt, (dotexpr));
            newstmt->line = nxtline;
            
            if (delstmt)
            {
                delstmt->Attach(newstmt);
            }
            delstmt = newstmt;
            
            if (!stmt)
            {
                stmt = delstmt;
            }
        }
        while (Optional(','));
        
        return stmt;
    }
    break;
    default:
        if (!skipExpr)
        {
            stmt = DoExpressionStatement();
        }
        else
        {
            PIKA_NEWNODE(EmptyStmt, stmt, ());
            stmt->line = -1;
        }
        break;
    }
    return stmt;
}

StorageKind Parser::DoStorageKind()
{
    if (Optional(TOK_local))
        return STO_local;
    else if (Optional(TOK_member))
        return STO_member;
    else if (Optional(TOK_global))
        return STO_global;
    return STO_none;
}

Stmt* Parser::DoClassStatement()
{
    const int class_terms[] = { TOK_end, TOK_finally, 0 };
    Stmt* stmt = 0;
    int line = tstream.GetLineNumber();
    Expr* super = 0;
    Stmt* classbody = 0;
    ClassDecl* decl = 0;
    
    StorageKind sk = DoStorageKind();
    
    Match(TOK_class);
    
    NameNode* name = DoNameNode(sk == STO_none);
    
    if (Optional(':'))
    {
        super = DoExpression();
    }
    
    classbody = DoStatementList(class_terms);
    
    PIKA_NEWNODE(ClassDecl, decl, (name, super, classbody, sk));
    PIKA_NEWNODE(DeclStmt, stmt, (decl));
    stmt->line = decl->line = line;
    
    return DoFinallyBlock(stmt);
}

Stmt* Parser::DoAssertStatement()
{
    int line = tstream.GetLineNumber();
    
    Match(TOK_assert);
    
    Expr* expr = DoExpression();
    
    DoEndOfStatement();
    
    AssertStmt* stmt;
    PIKA_NEWNODE(AssertStmt, stmt, (expr));
    stmt->line = line;
    return stmt;
}

NameNode* Parser::DoNameNode(bool canbedot)
{
    NameNode* name = 0;
    int line = tstream.GetLineNumber();
    
    if (tstream.GetNextType() == '.' && canbedot)
    {
        Expr* expr;
        if (tstream.GetType() == TOK_identifier)
            expr = DoIdExpression();
        else
            expr = DoPrimaryExpression();
        while (Optional('.'))
        {
            Expr* lhs = expr;
            Expr* rhs = 0;
            
            Id* id = DoIdentifier();
            PIKA_NEWNODE(MemberExpr, rhs, (id));
            rhs->line = id->line;
            
            PIKA_NEWNODE(DotExpr, expr, (lhs, rhs));
            expr->line = rhs->line;
        }
        
        if (expr->kind == Expr::EXPR_dot)
        {
            DotExpr* de = (DotExpr*)expr;
            PIKA_NEWNODE(NameNode, name, (de));
            name->line = line;
        }
        else
        {
            state->SyntaxException(Exception::ERROR_syntax, expr->line, "invalid name.");
        }
    }
    else
    {
        IdExpr* idexpr = DoIdExpression();
        PIKA_NEWNODE(NameNode, name, (idexpr));
        name->line = line;
    }
    return name;
}

Stmt* Parser::DoPackageDeclaration()
{
    int       line = tstream.GetLineNumber();
    PkgDecl*  decl = 0;
    Stmt*     stmt = 0;
    
    StorageKind sk = DoStorageKind();
    
    Match(TOK_package);
    
    NameNode* id   = DoNameNode(sk == STO_none);
    Stmt*     body = DoStatementList(Static_end_terms);
    
    Match(TOK_end);
    
    PIKA_NEWNODE(PkgDecl, decl, (id, body, sk));
    PIKA_NEWNODE(DeclStmt, stmt, (decl));
    
    stmt->line = decl->line = line;
    
    return stmt;
}

CaseList* Parser::DoCaseList(const int* terms)
{
    CaseList* cases = 0;
    
    while (Optional(TOK_case))
    {
        ExprList* matches = DoExpressionList();
        
        if (!Optional(TOK_then))
        {
            ExprList* curr = matches;
            while (curr && curr->next)
                curr = curr->next;
                
            int thenline = curr->line;
            if (thenline == tstream.GetLineNumber())
                Expected(TOK_then);
        }
        
        Stmt* body = DoStatementListBlock(terms);
        
        CaseList* c;
        PIKA_NEWNODE(CaseList, c, (matches, body));
        
        if (cases)
        {
            cases->Attach(c);
        }
        else
        {
            cases = c;
        }
    }
    return cases;
}

Stmt* Parser::DoCaseStatement()
{
    Stmt* stmt       = 0;
    int   line       = tstream.GetLineNumber();
    const int terms[] = { TOK_else, TOK_end, TOK_case, 0 };
    
    Match(TOK_select);
    
    Expr*     selector = DoExpression();
    CaseList* cases    = DoCaseList(terms);
    Stmt*     elsebody = 0;
    
    if (Optional(TOK_else))
    {
        elsebody = DoStatementListBlock(Static_end_terms);
    }
    
    Match(TOK_end);
    
    PIKA_NEWNODE(CaseStmt, stmt, (selector, cases, elsebody));
    stmt->line = line;
    return stmt;
}

Stmt* Parser::DoPropertyStatement()
{
    Stmt*         stmt     = 0;
    PropertyDecl* decl     = 0;
    const char*   getstr   = "get";
    const char*   setstr   = "set";
    const char*   next     = 0;
    bool          getfirst = false;
    
    StorageKind sk = DoStorageKind();
    
    Match(TOK_property);
    
    NameNode* name = DoNameNode(sk == STO_none);
    
    if (tstream.GetType() != TOK_identifier)
    {
        Unexpected(tstream.GetType());
    }
    
    if (DoContextualKeyword(getstr, true))
    {
        getfirst = true;
        next = setstr;
    }
    else if (DoContextualKeyword(setstr, true))
    {
        getfirst = false;
        next = getstr;
    }
    else
    {
        Unexpected(tstream.GetType());
    }
    Match(':');
    
    Expr* firstexpr = DoExpression();
    Expr* secondexpr = 0;
    
    DoEndOfStatement();
    
    if (tstream.GetType() == TOK_identifier)
    {
        DoContextualKeyword(next, false);
        Match(':');
        secondexpr = DoExpression();
        DoEndOfStatement();
    }
    
    Match(TOK_end);
    
    if (getfirst)
    {
        PIKA_NEWNODE(PropertyDecl, decl, (name, firstexpr, secondexpr, sk));
    }
    else
    {
        PIKA_NEWNODE(PropertyDecl, decl, (name, secondexpr, firstexpr, sk));
    }
    PIKA_NEWNODE(DeclStmt, stmt, (decl));
    stmt->line = decl->line = name->line;
    
    return stmt;
}

Stmt* Parser::DoStatementList(const int* terms)
{
    Stmt* currstmt = 0;
    
    while (IsNotTerm(terms, tstream.GetType()) && !tstream.IsEndOfStream())
    {
        Stmt* nxtstmt = DoStatement(false);
        
        if (currstmt)
        {
            Stmt* stmtseq = 0;
            PIKA_NEWNODE(StmtList, stmtseq, (currstmt, nxtstmt));
            stmtseq->line = currstmt->line;
            
            currstmt = stmtseq;
        }
        else
        {
            currstmt = nxtstmt;
        }
    }
    
    if (!currstmt)
    {
        PIKA_NEWNODE(EmptyStmt, currstmt, ());
        currstmt->line = tstream.GetLineNumber();
    }
    
    return currstmt;
}

Stmt* Parser::DoFunctionBody()
{
    const int terms[] = { TOK_end, TOK_finally, 0 };
    Stmt* stmt = DoStatementList(terms);
    return DoFinallyBlock(stmt);
}

Stmt* Parser::DoStatementListBlock(const int* terms)
{
    Stmt* stmt = 0;
    Stmt* stmtList = DoStatementList(terms);
    
    PIKA_NEWNODE(BlockStmt, stmt, (stmtList));
    stmt->line = stmtList->line;
    return stmt;
}

Stmt* Parser::DoFinallyBlock(Stmt* in)
{
    Stmt* out = 0;
    
    if (Optional(TOK_finally))
    {
        Stmt* finallydBlock = DoStatementList(Static_end_terms);
        
        Match(TOK_end);
        
        PIKA_NEWNODE(FinallyStmt, out, (in, finallydBlock));
        out->line = in->line;
    }
    else
    {
        Match(TOK_end);
        out = in;
    }
    return out;
}

Stmt* Parser::DoBlockStatement()
{
    Match(TOK_begin);
    const int terms[] = { TOK_end, TOK_finally, 0 };
    Stmt* block = DoStatementListBlock(terms);
    
    return DoFinallyBlock(block);
}

Stmt* Parser::DoWithStatement()
{
    int line = tstream.GetLineNumber();
    
    Match(TOK_with);
    
    Expr* with = DoExpression();
    
    if (!Optional(TOK_do))
    {
        if (with->line == tstream.GetLineNumber())
            Expected(TOK_do);
    }
    const int terms[] = { TOK_end, TOK_finally, 0 };
    Stmt* block = DoStatementList(terms);
    
    WithStatement* stmt;
    PIKA_NEWNODE(WithStatement, stmt, (with, block));
    stmt->line = line;
    
    if (tstream.GetType() == TOK_end)
    {
        int eline = tstream.GetLineNumber();
        
        Match(TOK_end);
        Stmt* out = 0;
        Stmt* finallydBlock = 0;
        PIKA_NEWNODE(EmptyStmt, finallydBlock, ());
        finallydBlock->line = eline;
        PIKA_NEWNODE(FinallyStmt, out, (stmt, finallydBlock));
        out->line = eline;
        return out;
    }    
    return DoFinallyBlock(stmt);
}

Stmt* Parser::DoTryStatement()
{
    Stmt* catchblock = 0;
    Id* caughtvar = 0;
    CatchIsBlock* cis = 0;
    int line = tstream.GetLineNumber();
    
    Match(TOK_try);
    
    const int try_terms[] = { TOK_catch, TOK_finally, 0 };
    Stmt* tryblock = DoStatementListBlock(try_terms);
    const int terms[] = { TOK_end, TOK_catch, TOK_else, TOK_finally, 0 };
    
    
    while (tstream.GetType() == TOK_catch)
    {
        Match(TOK_catch);
        
        Id* id = DoIdentifier();
        
        if (Optional(TOK_is))
        {
        
            Expr* is = DoExpression();
            
            if (!Optional(TOK_do))
            {
                if (id->line == tstream.GetLineNumber())
                    Expected(TOK_do);
            }
            
            Stmt* body = DoStatementListBlock(terms);
            
            CatchIsBlock* c;
            PIKA_NEWNODE(CatchIsBlock, c, (id, is, body));
            
            if (!cis)
                cis = c;
            else
                cis->Attach(c);
        }
        else
        {
            if (caughtvar || catchblock)
                state->SyntaxError(tstream.GetLineNumber(), "duplicate untyped catch block (one allowed per try statement.)");
            caughtvar = id;
            if (!Optional(TOK_do))
            {
                if (caughtvar->line == tstream.GetLineNumber())
                    Expected(TOK_do);
            }
            
            catchblock = DoStatementListBlock(terms);
            break;
        }
    }
    Stmt* elsebody = 0;
    if (Optional(TOK_else))
    {
        const int elseterms[] = { TOK_end, TOK_finally, 0 };
        elsebody = DoStatementListBlock(elseterms);
    }
    
    Stmt* stmt = 0;
    PIKA_NEWNODE(TryStmt, stmt, (tryblock, catchblock, caughtvar, cis, elsebody));
    stmt->line = line;
    
    return DoFinallyBlock(stmt);
}

Stmt* Parser::DoLabeledStatement()
{
    if (tstream.GetType() != TOK_identifier)
        Expected(TOK_identifier);
        
    if (tstream.GetNextType() == ':')
    {
        // <identifier> ':' <statement>
        
        Id* id = DoIdentifier();
        Match(':');
        
        Stmt* stmt    = DoStatement(true);
        Stmt* lblstmt = 0;
        
        PIKA_NEWNODE(LabeledStmt, lblstmt, (id, stmt));
        
        lblstmt->line = id->line;
        
        return lblstmt;
    }
    return DoExpressionStatement(); // not a label, only possibility is an expression
}

Stmt* Parser::DoVarStatement()
{
    Decl* decl = DoVarDeclaration();
    
    DoEndOfStatement();
    
    Stmt* stmt = 0;
    PIKA_NEWNODE(DeclStmt, stmt, (decl));
    
    stmt->line = decl->line;
    
    return stmt;
}

Decl* Parser::DoVarDeclaration()
{
    // local  var1, ..., varN [ = expr1, ..., exprN ]
    // member var1, ..., varN [ = expr1, ..., exprN ]
    // global var1, ..., varN [ = expr1, ..., exprN ]
    
    VarDecl*    decl      = 0;
    VarDecl*    firstdecl = 0;
    bool        is_local  = false;
    bool        is_member = false;
    StorageKind sto       = STO_none;
    
    if (Optional(TOK_local))
    {
        sto = STO_local;
        is_local = true;
    }
    else if (Optional(TOK_member))
    {
        sto = STO_member;
        is_member = true;
    }
    else
    {
        Match(TOK_global);
        sto = STO_global;
    }
    
    do
    {
        VarDecl* nxtdecl = 0;
        Id* name = DoIdentifier();
        
        if (is_local)
        {
            PIKA_NEWNODE(LocalDecl, nxtdecl, (name));
        }
        else if (is_member)
        {
            PIKA_NEWNODE(MemberDeclaration, nxtdecl, (name));
        }
        else
        {
            PIKA_NEWNODE(VarDecl, nxtdecl, (name));
        }
        
        nxtdecl->line = name->line;
        
        if (!firstdecl)
            firstdecl = nxtdecl;
            
        if (decl)
            decl->next = nxtdecl;
            
        decl = nxtdecl;
        
        if (tstream.GetType() != ',')
            break;
            
        Match(',');
    }
    while (!tstream.IsEndOfStream());
    
    if (Optional('='))
    {
        VariableTarget* vt;
        
        ExprList* el = DoExpressionList();
        PIKA_NEWNODE(VariableTarget, vt, (firstdecl, el, sto));
        vt->line = firstdecl->line;
        return vt;
    }
    
    return firstdecl;
}

Stmt* Parser::DoFunctionStatement()
{
    FunctionDecl* decl = 0;
    Stmt*         stmt = 0;
    size_t        beg  = tstream.curr.begOffset;
    
    StorageKind sk = DoStorageKind();
    
    if (TOK_function   == tstream.GetType()     &&
        TOK_identifier != tstream.GetNextType() &&
        TOK_self       != tstream.GetNextType() &&
        STO_none == sk
        )
    {
        return DoExpressionStatement(); // possible a function expression (definitely not a function statement)
    }
    
    Match(TOK_function);
    
    NameNode*  name = DoNameNode(sk == STO_none); // only allow dot names for functions without a storage specifier.
    ParamDecl* args = DoFunctionArguments();
    Stmt*      body = DoFunctionBody();
    size_t     end  = tstream.prev.endOffset;
    
    PIKA_NEWNODE(FunctionDecl, decl, (name, args, body, beg, end, sk));
    PIKA_NEWNODE(DeclStmt, stmt, (decl));
    decl->line = stmt->line = name->line;
    return stmt;
}

Stmt* Parser::DoIfStatement()
{
    Expr* cond = 0;
    Stmt* body = 0;
    IfStmt* stmt = 0;
    Stmt* elsePart = 0;
    bool unless = false;
    
    if (!Optional(TOK_if))
    {
        Match(TOK_unless);
        unless = true;
    }
    
    cond = DoExpression();
    
    if (!Optional(TOK_then))
    {
        if (cond->line == tstream.GetLineNumber())
            Expected(TOK_then);
    }
    
    const int first_terms[] = { TOK_end, TOK_elseif, TOK_else, TOK_finally, 0 };
    const int terms[] = { TOK_end, TOK_elseif, TOK_else, 0 };
    body = DoStatementListBlock(first_terms);
    
    PIKA_NEWNODE(IfStmt, stmt, (cond, body, unless));
    stmt->line = cond->line;
    
    while (Optional(TOK_elseif))
    {
        unless = false;
        
        IfStmt* elseifStmt = 0;
        // elseif <expr> then stmt
        
        cond = DoExpression();
        Optional(TOK_then);
        
        body = DoStatementListBlock(terms);
        
        PIKA_NEWNODE(IfStmt, elseifStmt, (cond, body, unless));
        elseifStmt->line = cond->line;
        stmt->Attach(elseifStmt);
    }
    
    if (Optional(TOK_else))
    {
        elsePart = DoStatementListBlock(Static_end_terms);
    }
    
    Match(TOK_end);
    
    Stmt* condStmt = 0;
    PIKA_NEWNODE(ConditionalStmt, condStmt, (stmt, elsePart));
    condStmt->line = stmt->line;
    
    condStmt->line = cond->line;
    return condStmt;
}

Stmt* Parser::DoWhileStatement()
{
    Expr* cond = 0;
    Stmt* body = 0;
    Stmt* stmt = 0;
    
    Match(TOK_while);
    
    cond = DoExpression();
    
    if (!Optional(TOK_do))
    {
        if (cond->line == tstream.GetLineNumber())
            Expected(TOK_do);
    }
    
    body = DoStatementListBlock(Static_finally_terms);
    
    //Match(TOK_end);
    
    PIKA_NEWNODE(CondLoopStmt, stmt, (cond, body, false, false));
    stmt->line = cond->line;
           
    return DoFinallyBlock(stmt);
}

Stmt* Parser::DoUntilStatement()
{
    Expr* cond = 0;
    Stmt* body = 0;
    Stmt* stmt = 0;
    
    Match(TOK_until);
    
    cond = DoExpression();
    
    if (!Optional(TOK_do))
    {
        if (cond->line == tstream.GetLineNumber())
            Expected(TOK_do);
    }
    
    body = DoStatementListBlock(Static_finally_terms);
    
    //Match(TOK_end);
    
    PIKA_NEWNODE(CondLoopStmt, stmt, (cond, body, false, true));
    stmt->line = cond->line;
    
    return DoFinallyBlock(stmt);
}

Stmt* Parser::DoLoopStatement()
{
    Stmt* body = 0;
    Stmt* stmt = 0;
    int line = tstream.GetLineNumber();
    
    Match(TOK_loop);
    
    body = DoStatementListBlock(Static_finally_terms);
    //Match(TOK_end);
    
    PIKA_NEWNODE(LoopStmt, stmt, (body));
    stmt->line = line;
    
    return DoFinallyBlock(stmt);
}

bool Parser::DoContextualKeyword(const char* key, bool optional)
{
    if (tstream.GetType() == TOK_identifier)
    {
        int comp = StrCmp(key, tstream.GetString());
        
        if (comp != 0)
        {
            if (!optional)
            {
                state->SyntaxException(Exception::ERROR_syntax, tstream.GetLineNumber(), "Expected token '%s'.", key);
            }
            return false;
        }
        else
        {
            Pika_free(tstream.GetString());
            tstream.Advance();
            return true;
        }
    }
    return false;
}

// for <id> := <expr> to <expr> [ step <expr> ] [ do ]
// <stmt_list>
// end

void Parser::DoForToHeader(ForToHeader* header)
{
    header->line = tstream.GetLineNumber();
    
    Match(TOK_for);
    
    header->id = DoIdentifier();
    
    Match('=');
    
    header->from = DoExpression();
    
    if (!Optional(TOK_to))
    {
        Match(TOK_downto);
        header->isdown = true;
    }
    else
    {
        header->isdown = false;
    }
    
    header->to = DoExpression();
    
    if (Optional(TOK_by))
    {
        header->step = DoExpression();
    }
    else
    {
        Expr* estep;
        PIKA_NEWNODE(IntegerExpr, estep, ((header->isdown) ? -1 : 1));
        estep->line = header->to->line;
        header->step = estep;
    }
    
}

Stmt* Parser::DoForToStatement()
{
    Stmt* stmt = 0;
    Stmt* body = 0;
    
    ForToHeader header = { 0, 0, 0, 0, false, 0 };
    DoForToHeader(&header);
    
    if (!Optional(TOK_do))
    {
        if (header.step->line == tstream.GetLineNumber())
            Expected(TOK_do);
    }
    
    body = DoStatementListBlock(Static_finally_terms);
    
    
    //Match(TOK_end);
    
    PIKA_NEWNODE(ForToStmt, stmt, (header.id, header.from, header.to, header.step, body, header.isdown));
    stmt->line = header.line;
    return DoFinallyBlock(stmt);
}

void Parser::DoForEachHeader(ForEachHeader* header)
{
    int line = tstream.GetLineNumber();
    header->line = line;
    
    Match(TOK_foreach);
    
    Id* id = DoIdentifier();
    LocalDecl* vdecl;
    PIKA_NEWNODE(LocalDecl, vdecl, (id));
    vdecl->line = id->line;
    header->each = vdecl;
    
    Match(TOK_in);
    
    if (tstream.GetType() ==  TOK_identifier && tstream.GetNextType() == TOK_of)
    {
        header->of = DoFieldName();
        Match(TOK_of);
    }
    else
    {
        StringExpr* of;
        PIKA_NEWNODE(StringExpr, of, (Pika_strdup(""), 0));
        of->line = line;
        header->of = of;
    }
    header->subject = DoExpression();
}

Stmt* Parser::DoForEachStatement()
{
    ForEachHeader header;
    DoForEachHeader(&header);
    if (!Optional(TOK_do))
    {
        if (header.subject->line == tstream.GetLineNumber())
            Expected(TOK_do);
    }
    
    Stmt* body = DoStatementListBlock(Static_finally_terms);
    
    //Match(TOK_end);
    
    Stmt* stmt = 0;
    PIKA_NEWNODE(ForeachStmt, stmt, (header.each, header.of, header.subject, body));
    stmt->line = header.line;
       
    return DoFinallyBlock(stmt);
}

StringExpr* Parser::DoFieldName()
{
    StringExpr* name = 0;
    
    if (tstream.GetType() == TOK_identifier)
    {
        char* idstr = tstream.GetString();
        
        PIKA_NEWNODE(StringExpr, name, (idstr, tstream.GetStringLength()));
        
        tstream.Advance();
    }
    else
    {
        Expected(TOK_identifier);
    }
    return name;
}

Stmt* Parser::DoRaiseStatement()
{
    Stmt* stmt = 0;
    Expr* expr = 0;
    
    int line = tstream.GetLineNumber();
    
    Match(TOK_raise);
    
    if (!IsEndOfStatement())
    {
        expr = DoExpression();
    }
    
    PIKA_NEWNODE(RaiseStmt, stmt, (expr));
    
    if (expr) stmt->line = expr->line;
    else      stmt->line = line;
    
    stmt = DoOptionalJumpStatement(stmt);
    
    DoEndOfStatement();
    
    return stmt;
}

Stmt* Parser::DoReturnStatement()
{
    Stmt* stmt = 0;
    ExprList* exprList = 0;
    int line = tstream.GetLineNumber();
    
    Match(TOK_return);
    
    if (!IsEndOfStatement() && (tstream.GetType() != TOK_if) && (tstream.GetType() != TOK_unless))
    {
        exprList = DoExpressionList();
    }
    
    PIKA_NEWNODE(RetStmt, stmt, (exprList));
    
    if (exprList)
    {
        stmt->line = exprList->line;
    }
    else
    {
        stmt->line = line;
    }
    
    stmt = DoOptionalJumpStatement(stmt);
    
    DoEndOfStatement();
    
    return stmt;
}

Stmt* Parser::DoYieldStatement()
{
    Stmt* stmt = 0;
    ExprList* exprList = 0;
    int line = tstream.GetLineNumber();
    
    Match(TOK_yield);
    
    if (!IsEndOfStatement() && (tstream.GetType() != TOK_if) && (tstream.GetType() != TOK_unless))
    {
        exprList = DoExpressionList();
    }
    
    PIKA_NEWNODE(YieldStmt, stmt, (exprList));
    
    if (exprList)
    {
        stmt->line = exprList->line;
    }
    else
    {
        stmt->line = line;
    }
    
    stmt = DoOptionalJumpStatement(stmt);
    
    DoEndOfStatement();
    
    return stmt;
}

Stmt* Parser::DoGenStatement()
{
    Stmt* stmt = 0;
    ExprList* exprList = 0;
    int line = tstream.GetLineNumber();
    
    Match(TOK_gen);
    
    if (!IsEndOfStatement() && (tstream.GetType() != TOK_if) && (tstream.GetType() != TOK_unless))
    {
        exprList = DoExpressionList();
    }
    
    PIKA_NEWNODE(YieldStmt, stmt, (exprList));
    
    if (exprList)
    {
        stmt->line = exprList->line;
    }
    else
    {
        stmt->line = line;
    }
    
    stmt = DoOptionalJumpStatement(stmt);
    
    DoEndOfStatement();
    
    return stmt;
}

Stmt* Parser::DoBreakStatement()
{
    Stmt* stmt = 0;
    Id* id = 0;
    
    int line = tstream.GetLineNumber();
    
    Match(TOK_break);
    
    if (!IsEndOfStatement())
    {
        if (tstream.GetType() == TOK_identifier)
        {
            id = DoIdentifier();
        }
    }
    
    PIKA_NEWNODE(BreakStmt, stmt, (id));
    stmt->line = line;
    
    stmt = DoOptionalJumpStatement(stmt);
    
    DoEndOfStatement();
    
    return stmt;
}

Stmt* Parser::DoContinueStatement()
{
    Stmt* stmt = 0;
    Id* id = 0;
    
    int line = tstream.GetLineNumber();
    
    Match(TOK_continue);
    
    if (!IsEndOfStatement())
    {
        if (tstream.GetType() == TOK_identifier)
        {
            id = DoIdentifier();
        }
    }
    
    PIKA_NEWNODE(ContinueStmt, stmt, (id));
    stmt->line = line;
    
    stmt = DoOptionalJumpStatement(stmt);
    
    DoEndOfStatement();
    
    return stmt;
}

Stmt* Parser::DoOptionalJumpStatement(Stmt* stmt)
{
    int line = tstream.GetNextLineNumber();
    
    if (line != stmt->line)
        return stmt;
        
    if (tstream.GetType() == TOK_if ||
            tstream.GetType() == TOK_unless )
    {
        bool unless = false;
        if (!Optional(TOK_if))
        {
            Match(TOK_unless);
            unless = true;
        }
        Expr* cond = DoExpression();
        Stmt* ifstmt = 0;
        
        PIKA_NEWNODE(IfStmt, ifstmt, (cond, stmt, unless));
        
        ifstmt->line = cond->line = stmt->line;
        
        return ifstmt;
    }
    return stmt;
}

static bool IsAssignment(int t)
{
    switch (t)
    {
    case '=':
    case TOK_catspassign:
    case TOK_catassign:
    case TOK_bitorassign:
    case TOK_bitxorassign:
    case TOK_bitandassign:
    case TOK_lshassign:
    case TOK_rshassign:
    case TOK_urshassign:
    case TOK_addassign:
    case TOK_subassign:
    case TOK_mulassign:
    case TOK_divassign:
    case TOK_modassign:
        return true;
    default:
        return false;
    }
    return false;
}

static AssignmentStmt::AssignKind GetAssignmentKind(int t)
{
    switch (t)
    {
    case TOK_bitorassign:  return AssignmentStmt::BIT_OR_ASSIGN_STMT;
    case TOK_bitxorassign: return AssignmentStmt::BIT_XOR_ASSIGN_STMT;
    case TOK_bitandassign: return AssignmentStmt::BIT_AND_ASSIGN_STMT;
    case TOK_lshassign:    return AssignmentStmt::LSH_ASSIGN_STMT;
    case TOK_rshassign:    return AssignmentStmt::RSH_ASSIGN_STMT;
    case TOK_urshassign:   return AssignmentStmt::URSH_ASSIGN_STMT;
    case TOK_addassign:    return AssignmentStmt::ADD_ASSIGN_STMT;
    case TOK_subassign:    return AssignmentStmt::SUB_ASSIGN_STMT;
    case TOK_mulassign:    return AssignmentStmt::MUL_ASSIGN_STMT;
    case TOK_divassign:    return AssignmentStmt::DIV_ASSIGN_STMT;
    case TOK_modassign:    return AssignmentStmt::MOD_ASSIGN_STMT;
    case TOK_catspassign:  return AssignmentStmt::CONCAT_SPACE_ASSIGN_STMT;
    case TOK_catassign:    return AssignmentStmt::CONCAT_ASSIGN_STMT;
    default:               return AssignmentStmt::ASSIGN_STMT;
    }
    return AssignmentStmt::ASSIGN_STMT;
}

Stmt* Parser::DoExpressionStatement()
{
    Stmt* stmt = 0;
    ExprList* expr = DoExpressionList();
    
    if (IsAssignment(tstream.GetType()))
    {
    
        AssignmentStmt::AssignKind akind = GetAssignmentKind(tstream.GetType());
        
        tstream.Advance();
        
        ExprList* lhs = expr;
        ExprList* rhs = DoExpressionList();
        
        DoEndOfStatement();
        PIKA_NEWNODE(AssignmentStmt, stmt, (lhs, rhs));
        
        if (akind != AssignmentStmt::ASSIGN_STMT)
        {
            ((AssignmentStmt*)stmt)->kind = akind;
            ((AssignmentStmt*)stmt)->isBinaryOp = true;
        }
    }
    else
    {
        DoEndOfStatement();
        PIKA_NEWNODE(ExprStmt, stmt, (expr));
    }
    
    stmt->line = expr->line;
    
    return stmt;
}

/////////////////////////////////////////////////////////////////////////////////////
//
// Expression Parsing
//
/////////////////////////////////////////////////////////////////////////////////////

Expr* Parser::DoExpression()
{
    return DoNullSelectExpression();
}

//
// lhs-expression ?? rhs-expression
//
Expr* Parser::DoNullSelectExpression()
{
    Expr* expr = DoConditionalExpression();
    
    while (tstream.GetType() == TOK_nullselect)
    {
        Expr* lhs = expr;
        tstream.Advance();
        Expr* rhs = DoConditionalExpression();
        
        PIKA_NEWNODE(NullSelectExpr, expr, (lhs, rhs));
        expr->line = lhs->line; // !!! set line number
    }
    return expr;
}

//
// cond-expression ? then-expression : else-expression
//
Expr* Parser::DoConditionalExpression()
{
    Expr* expr = DoConcatExpression();
    
    if (tstream.GetType() == '?')
    {
        Expr* cond = expr;
        tstream.Advance();
        
        Expr* then = DoExpression();
        
        Match(':');
        
        Expr* otherwise = DoConditionalExpression();
        
        PIKA_NEWNODE(CondExpr, expr, (cond, then, otherwise));
        expr->line = cond->line; // !!! set line number
    }
    return expr;
}

Expr* Parser::DoConcatExpression()
{
    Expr* expr = DoLogOrExpression();
    
    while (tstream.GetType() == TOK_cat || tstream.GetType() == TOK_catspace)
    {
        Expr::Kind k = (tstream.GetType() == TOK_catspace) ? Expr::CONCAT_SPACE_EXPR : Expr::CONCAT_EXPR;
        
        Expr* lhs = expr;
        
        tstream.Advance();
        Expr* rhs = DoLogOrExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (k, lhs, rhs));
        expr->line = lhs->line; // !!! set line number
    }
    return expr;
}
//
// lhs-expression or rhs-expression
//
Expr* Parser::DoLogOrExpression()
{
    Expr* expr = DoLogXorExpression();
    
    while (tstream.GetType() == TOK_or || tstream.GetType() == TOK_pipepipe)
    {
        Expr::Kind k = Expr::LOG_OR_EXPR;
        
        Expr* lhs = expr;
        tstream.Advance();
        Expr* rhs = DoLogXorExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (k, lhs, rhs));
        expr->line = lhs->line; // !!! set line number
    }
    return expr;
}
//
// lhs-expression xor rhs-expression
//
Expr* Parser::DoLogXorExpression()
{
    Expr* expr = DoLogAndExpression();
    
    while (tstream.GetType() == TOK_xor || tstream.GetType() == TOK_caretcaret)
    {
        Expr::Kind k = Expr::LOG_XOR_EXPR;
        
        Expr* lhs = expr;
        tstream.Advance();
        Expr* rhs = DoLogAndExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (k, lhs, rhs));
        expr->line = lhs->line; // !!! set line number
    }
    return expr;
}
//
// lhs-expression and rhs-expression
//
Expr* Parser::DoLogAndExpression()
{
    Expr* expr = DoOrExpression();
    
    while (tstream.GetType() == TOK_and || tstream.GetType() == TOK_ampamp)
    {
        Expr::Kind k = Expr::LOG_AND_EXPR;
        
        Expr* lhs = expr;
        tstream.Advance();
        Expr* rhs = DoOrExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (k, lhs, rhs));
        expr->line = lhs->line; // !!! set line number
    }
    return expr;
}
//
// lhs-expression | rhs-expression
//
Expr* Parser::DoOrExpression()
{
    Expr* expr = DoXorExpression();
    
    while (tstream.GetType() == '|')
    {
        Expr::Kind k = Expr::BIT_OR_EXPR;
        
        Expr* lhs = expr;
        tstream.Advance();
        Expr* rhs = DoXorExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (k, lhs, rhs));
        expr->line = lhs->line; // !!! set line number
    }
    return expr;
}
//
// lhs-expression ^ rhs-expression
//
Expr* Parser::DoXorExpression()
{
    Expr* expr = DoAndExpression();
    
    while (tstream.GetType() == '^')
    {
        Expr::Kind k = Expr::BIT_XOR_EXPR;
        
        Expr* lhs = expr;
        tstream.Advance();
        Expr* rhs = DoAndExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (k, lhs, rhs));
        expr->line = lhs->line; // !!! set line number
    }
    return expr;
}
//
// lhs & rhs
//
Expr* Parser::DoAndExpression()
{
    Expr* expr = DoEqualExpression();
    
    while (tstream.GetType() == '&')
    {
        Expr::Kind k = Expr::BIT_AND_EXPR;
        
        Expr* lhs = expr;
        tstream.Advance();
        Expr* rhs = DoEqualExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (k, lhs, rhs));
        expr->line = lhs->line; // !!! set line number
    }
    return expr;
}
//
// lhs == rhs
// lhs != rhs
// lhs === rhs
// lhs !== rhs
// lhs is rhs
//
Expr* Parser::DoEqualExpression()
{
    Expr* expr = DoCompExpression();
    
    while ( tstream.GetType() == TOK_eq      ||
            tstream.GetType() == TOK_ne      ||
            tstream.GetType() == TOK_same    ||
            tstream.GetType() == TOK_notsame ||
            tstream.GetType() == TOK_is      || 
            tstream.GetType() == TOK_has       )
    {
        Expr::Kind k;
        
        if (tstream.GetType() == TOK_eq)
        {
            k = Expr::EQUAL_EXPR;
        }
        else if (tstream.GetType() == TOK_ne)
        {
            k = Expr::NOT_EQUAL_EXPR;
        }
        else if (tstream.GetType() == TOK_same)
        {
            k = Expr::SAME_EXPR;
        }
        else if (tstream.GetType() == TOK_notsame)
        {
            k = Expr::NOT_SAME_EXPR;
        }
        else if (tstream.GetType() == TOK_is)
        {
//            if (tstream.GetNextType() == TOK_not)
//            {
//                k = Expr::IS_NOT_EXPR;
//                tstream.Advance();
//            }
//            else
//            {
                k = Expr::IS_EXPR;
//            }
        }
        else
        {
            // TOK_has
            k = Expr::HAS_EXPR;
        }
        
        Expr* lhs = expr;
        tstream.Advance();
        
        if (k == Expr::SAME_EXPR && tstream.GetType() == TOK_not)
        {
            k = Expr::NOT_SAME_EXPR;
            tstream.Advance();
        }
        
        Expr* rhs = DoCompExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (k, lhs, rhs));
        expr->line = lhs->line; // !!! set line number
    }
    return expr;
}
//
// lhs < rhs
// lhs > rhs
// lhs <= rhs
// lhs >= rhs
//
Expr* Parser::DoCompExpression()
{
    Expr* expr = DoShiftExpression();
    
    while ( tstream.GetType() == '<'     ||
            tstream.GetType() == '>'     ||
            tstream.GetType() == TOK_lte ||
            tstream.GetType() == TOK_gte)
    {
        Expr::Kind k;
        
        if (tstream.GetType() == '<')
        {
            k = Expr::LT_EXPR;
        }
        else if (tstream.GetType() == '>')
        {
            k = Expr::GT_EXPR;
        }
        else if (tstream.GetType() == TOK_lte)
        {
            k = Expr::LTE_EXPR;
        }
        else
        {
            k = Expr::GTE_EXPR;
        }
        
        Expr* lhs = expr;
        tstream.Advance();
        Expr* rhs = DoShiftExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (k, lhs, rhs));
        expr->line = lhs->line; // !!! set line number
    }
    return expr;
}
//
// lhs << rhs
// lhs >> rhs
// lhs >>> rhs
//
Expr* Parser::DoShiftExpression()
{
    Expr* expr = DoAddExpression();
    
    while (tstream.GetType() == TOK_lsh || tstream.GetType() == TOK_rsh ||
            tstream.GetType() == TOK_ursh)
    {
        Expr::Kind k;
        
        if (tstream.GetType() == TOK_lsh)
        {
            k = Expr::EXPR_lsh;
        }
        else if (tstream.GetType() == TOK_rsh)
        {
            k = Expr::EXPR_rsh;
        }
        else
        {
            k = Expr::EXPR_ursh;
        }
        Expr* lhs = expr;
        tstream.Advance();
        Expr* rhs = DoAddExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (k, lhs, rhs));
        expr->line = lhs->line;
    }
    return expr;
}
//
// lhs + rhs
// lhs - rhs
//
Expr* Parser::DoAddExpression()
{
    Expr* expr = DoMulExpression();
    
    while (tstream.GetType() == '+' || tstream.GetType() == '-')
    {
        Expr::Kind k = (tstream.GetType() == '+') ? Expr::ADD_EXPR : Expr::SUB_EXPR;
        
        Expr* lhs = expr;
        
        tstream.Advance();
        Expr* rhs = DoMulExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (k, lhs, rhs));
        expr->line = lhs->line;
    }
    return expr;
}
//
// lhs * rhs
// lhs / rhs
// lhs % rhs
// lhs mod rhs
//
Expr* Parser::DoMulExpression()
{
    Expr* expr = DoPrefixExpression();
    
    while (tstream.GetType() == '*' || tstream.GetType() == '/' || tstream.GetType() == TOK_div ||
            tstream.GetType() == TOK_mod || tstream.GetType() == '%')
    {
        Expr::Kind k;
        
        if (tstream.GetType() == '*')
        {
            k = Expr::MUL_EXPR;
        }
        else if (tstream.GetType() == '/')
        {
            k = Expr::DIV_EXPR;
        }
        else if (tstream.GetType() == TOK_div)
        {
            k = Expr::INTDIV_EXPR;
        }
        else
        {
            k = Expr::MOD_EXPR;
        }
        
        Expr* lhs = expr;
        tstream.Advance();
        Expr* rhs = DoPrefixExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (k, lhs, rhs));
        expr->line = lhs->line; // !!! set line number
    }
    return expr;
}

static Expr::Kind GetPrefixType(int t)
{
    switch (t)
    {
    case '~':           return Expr::EXPR_bitnot;
    case '!':           return Expr::EXPR_lognot;
    case TOK_not:       return Expr::EXPR_lognot;
    case TOK_increment: return Expr::EXPR_preincr;
    case TOK_decrement: return Expr::EXPR_predecr;
    case '+':           return Expr::EXPR_positive;
    case '-':           return Expr::EXPR_negative;
    case TOK_typeof:    return Expr::EXPR_typeof;
    }
    return Expr::EXPR_invalid;
}

Expr* Parser::DoPrefixExpression()
{
    int lextok = tstream.GetType();
    
    switch (lextok)
    {
    case '~':
    case '!':
    case '+':
    case '-':
    case TOK_not:
    case TOK_increment:
    case TOK_decrement:
    case TOK_typeof:
    {
        // ~ | ! | + | - | not | ++ | -- <expression>
        
        int line = tstream.GetLineNumber();
        Expr::Kind k = GetPrefixType(lextok);
        tstream.Advance();
        Expr* operand = DoPrefixExpression();
        Expr* expr = 0;
        
        PIKA_NEWNODE(UnaryExpr, expr, (k, operand));
        expr->line = line;
        
        return expr;
    }
    break;
    case TOK_bind:
    {
        int line = tstream.GetLineNumber();
        tstream.Advance();
        
        Expr*    operand = DoPrefixExpression();
        Expr*    expr    = 0;
        DotExpr* dotexpr = 0;
        
        if (operand->kind != Expr::EXPR_dot)
        {
            state->SyntaxException(Exception::ERROR_syntax, tstream.GetLineNumber(), "bind must be followed by a dot expression.");
        }
        
        dotexpr = (DotExpr*)operand;
        Expr* lhs = dotexpr->left;
        Expr* rhs = dotexpr->right;
        PIKA_NEWNODE(DotBindExpr, expr, (lhs, rhs));
        expr->line = line;
        return expr;
    }
    break;
    case TOK_new:
        return DoNewExpression();
    }
    return DoPostfixExpression();
}

Expr* Parser::DoNewExpression()
{
    Match(TOK_new);
    
    Expr* callExpr = DoPostfixExpression();
    
    if (callExpr->kind == Expr::EXPR_call)
    {
        CallExpr* ce = (CallExpr*)callExpr;
        ce->kind = Expr::EXPR_new;
    }
    else
    {
        CallExpr* ce = 0;
        ExprList* callArgs = 0;
        
        if (IsPrimaryExpression()                       &&
                (tstream.GetPrevType() == TOK_identifier)    && // ????
                (tstream.GetLineNumber() == tstream.GetPreviousLineNumber()))
        {
            callArgs = DoExpressionList();
        }
        PIKA_NEWNODE(CallExpr, ce, (callExpr, callArgs));
        ce->kind = Expr::EXPR_new;
        callExpr = ce;
        
        //state->SyntaxException(Exception::ERROR_syntax, tstream.GetLineNumber(), "new must be followed by a call expression");
    }
    return callExpr;
}

bool Parser::IsPrimaryExpression()
{
    switch (tstream.GetType())
    {
    case '{':
    case '~':
    case '!':
//  case '[':   // We have a problem between array index [] and array literals [] with paran free calls
    case TOK_function:
    case TOK_identifier:
    case TOK_stringliteral:
    case TOK_integerliteral:
    case TOK_realliteral:
    case TOK_locals:
    case TOK_self:
    case TOK_super:
    case TOK_null:
    case TOK_true:
    case TOK_false:
    case TOK_not:
    case TOK_typeof:
        return tstream.GetPreviousLineNumber() == tstream.GetLineNumber();
    }
    return false;
}

Expr* Parser::DoPostfixExpression()
{
    Expr* expr = DoPrimaryExpression();
    
    for (;;)
    {
        if (IsPrimaryExpression() && tstream.GetPrevType() == TOK_identifier)
        {
            /*//////////////////////////////////////////////////////////////////////////////////////
            
            Call expression WITHOUT parenthesis.
            
            The first argument must be a primary expression and lie on the same line as the function
            being called.
            
            Each subsequent argument is comma seperated and can be any valid expression.
            
            FIXED:  5'hello' attempts to call the literal 5 with the argument 'hello'. This is now
            a compile time error because the previous token MUST be an identifier.            
          
            --------------------------------------------------------------------------------------------
            
            TODO:  This may need to be changed.
            Epecially if we ever change how dot-expr are composed.
            
            TODO:  What about [] reads.
            
            //////////////////////////////////////////////////////////////////////////////////////*/
            Expr*     lhs  = expr;
            ExprList* args = DoExpressionList();
            
            PIKA_NEWNODE(CallExpr, expr, (lhs, args));
            expr->line = lhs->line;
            
            return expr; // TODO: continue?
        }
        
        switch (tstream.GetType())
        {
            //
            // Postfix increment.
            //
        case TOK_increment:
        {
            int line = tstream.GetLineNumber();
            
            if (line > expr->line)
                return expr;
                
            Expr* lhs = expr;
            tstream.Advance();
            PIKA_NEWNODE(UnaryExpr, expr, (Expr::EXPR_postincr, lhs));
            expr->line = lhs->line;
            return expr;
        }
        //
        // Postfix decrement.
        //
        case TOK_decrement:
        {
            int line = tstream.GetLineNumber();
            
            if (line > expr->line)
                return expr;
                
            Expr* lhs = expr;
            tstream.Advance();
            PIKA_NEWNODE(UnaryExpr, expr, (Expr::EXPR_postdecr, lhs));
            expr->line = lhs->line;
            return expr;
        }
        //
        // Call or new expression.
        //
        case '(':
        {
            Expr* lhs = expr;
            
            // Check that the '(' occurs on the same line as the callee
            if (tstream.GetLineNumber() != lhs->line)
                return expr;
            Match('(');
            const int call_terms[] = { ')', 0 };
            ExprList* callargs = DoOptionalExpressionList(call_terms);
            int line = tstream.GetLineNumber();
            
            Match(')');
            
            PIKA_NEWNODE(CallExpr, expr, (lhs, callargs));
            expr->line = line;
        }
        continue;
        //
        // Subcript operator or slice operator.
        //
        case '[':
        {
            Expr* lhs = expr;
            
            Match('[');
            
            Expr* rhs = DoExpression();
            
            if (tstream.GetType() == ']')
            {
                int line = tstream.GetLineNumber();
                
                Match(']');
                
                PIKA_NEWNODE(IndexExpr, expr, (lhs, rhs));
                expr->line = line;
            }
            else
            {
                Match(TOK_slice);
                
                Expr* fromexpr = rhs;
                Expr* toexpr   = DoExpression();
                int   line     = tstream.GetLineNumber();
                
                Match(']');
                
                PIKA_NEWNODE(SliceExpr, expr, (lhs, fromexpr, toexpr));
                expr->line = line;
            }
        }
        continue;
        //
        // Dot operator.
        //
        case '.':
        {
            tstream.Advance();
            Expr* lhs = expr;
            Expr* rhs = 0;
            Id*   id  = DoIdentifier();
            
            PIKA_NEWNODE(MemberExpr, rhs, (id));
            PIKA_NEWNODE(DotExpr,    expr, (lhs, rhs));
            
            expr->line = rhs->line = id->line;
        }
        continue;
        //
        // expr ** expr
        //
        case TOK_starstar:
        {
            tstream.Advance();
            
            Expr* lhs = expr;
            Expr* rhs = DoPrimaryExpression();
            
            PIKA_NEWNODE(BinaryExpr, expr, (Expr::EXPR_pow, lhs, rhs));
            expr->line = lhs->line;
        }
        default:
            return expr;
        }
    }
    return expr;
}

Expr* Parser::DoPrimaryExpression()
{
    Expr* expr   = 0;
    int   lextok = tstream.GetType();
    int   line   = tstream.GetLineNumber();
    
    switch (lextok)
    {
//  case TOK_do:
    case TOK_function:  expr = DoFunctionExpression();     break;
    case '[':           expr = DoArrayExpression();        break;
    case '{':           expr = DoDictionaryExpression();   break;
    case '(':
    {
        Match('(');
        expr = DoExpression();
        
        Match(')');
    }
    break;
    case TOK_identifier:     expr = DoIdExpression();             break;
    case TOK_stringliteral:  expr = DoStringLiteralExpression();  break;
    case TOK_integerliteral: expr = DoIntegerLiteralExpression(); break;
    case TOK_realliteral:    expr = DoRealLiteralExpression();    break;
    case TOK_locals:
    {
        tstream.Advance();
        PIKA_NEWNODE(LoadExpr, expr, (LoadExpr::LK_locals));
        expr->line = line;
    }
    break;
    case TOK_self:
    {
        tstream.Advance();
        PIKA_NEWNODE(LoadExpr, expr, (LoadExpr::LK_self));
        expr->line = line;
    }
    break;
    /*
     * super
     */
    case TOK_super:
    {
        Match(TOK_super);
        
        ExprList* callargs = 0;
        Expr* lhs = 0;
        
        PIKA_NEWNODE(LoadExpr, lhs, (LoadExpr::LK_super));
        lhs->line = line;
        
        if (Optional('('))
        {
            const int call_terms[] = { ')', 0 };
            callargs = DoOptionalExpressionList(call_terms);
            line = tstream.GetLineNumber();
            Match(')');
        }
        else
        {
            if (IsPrimaryExpression() && tstream.GetLineNumber() == tstream.GetPreviousLineNumber())
                callargs = DoExpressionList();
        }
        
        PIKA_NEWNODE(CallExpr, expr, (lhs, callargs));
        ((CallExpr*)expr)->redirectedcall = true;
        expr->line = line;
    }
    break;
    case TOK_null:
    {
        tstream.Advance();
        PIKA_NEWNODE(LoadExpr, expr, (LoadExpr::LK_null));
        expr->line = line;
    }
    break;
    case TOK_true:
    {
        tstream.Advance();
        PIKA_NEWNODE(LoadExpr, expr, (LoadExpr::LK_true));
        expr->line = line;
    }
    break;
    case TOK_false:
    {
        tstream.Advance();
        PIKA_NEWNODE(LoadExpr, expr, (LoadExpr::LK_false));
        expr->line = line;
    }
    break;
    default: Unexpected(lextok);
    }
    return expr;
}

Id* Parser::DoIdentifier()
{
    Id* id = 0;
    
    if (tstream.GetType() == TOK_identifier)
    {
        int line = tstream.GetLineNumber();
        char* value = tstream.GetString();
        
        tstream.Advance();
        
        PIKA_NEWNODE(Id, id, (value));
        id->line = line;
    }
    else
    {
        Expected(TOK_identifier);
    }
    return id;
}

IdExpr* Parser::DoIdExpression()
{
    IdExpr* expr = 0;
    int     line = tstream.GetLineNumber();
    Id*     id   = DoIdentifier();
    
    PIKA_NEWNODE(IdExpr, expr, (id));
    expr->line = line;
    
    return expr;
}

Expr* Parser::DoRealLiteralExpression()
{
    Expr* expr  = 0;
    int   line  = tstream.GetLineNumber();
    preal_t value = tstream.GetReal();
    
    Match(TOK_realliteral);
    
    PIKA_NEWNODE(RealExpr, expr, (value));
    expr->line = line;
    
    return expr;
}

Expr* Parser::DoStringLiteralExpression()
{
    Expr*  expr  = 0;
    int    line  = tstream.GetLineNumber();
    char*  value = tstream.GetString();
    size_t len   = tstream.GetStringLength();
    
    Match(TOK_stringliteral);

    PIKA_NEWNODE(StringExpr, expr, (value, len));
    expr->line = line;
    
    return expr;
}

Expr* Parser::DoIntegerLiteralExpression()
{
    Expr* expr = 0;
    int line = tstream.GetLineNumber();
    pint_t value = tstream.GetInteger();
    
    Match(TOK_integerliteral);
    
    PIKA_NEWNODE(IntegerExpr, expr, (value));
    expr->line = line;
    
    return expr;
}

ExprList* Parser::DoExpressionList()
{
    ExprList* el = 0;
    ExprList* firstel = 0;
    
    do
    {
        Expr* expr = DoExpression();
        ExprList* nxtel = 0;
        
        PIKA_NEWNODE(ExprList, nxtel, (expr));
        nxtel->line = expr->line;
        
        if (!firstel)
            firstel = nxtel;
            
        if (el)
            el->next = nxtel;
            
        el = nxtel;
        
        if (!Optional(','))
        {
            break;
        }
    }
    while (!tstream.IsEndOfStream());
    
    return firstel;
}

ExprList* Parser::DoOptionalExpressionList(const int* terms, bool optcomma)
{
    ExprList* el = 0;
    ExprList* firstel = 0;
    
    while (!tstream.IsEndOfStream() && IsNotTerm(terms, tstream.GetType()) )
    {
        Expr* expr = DoExpression();
        ExprList* nxtel = 0;
        
        PIKA_NEWNODE(ExprList, nxtel, (expr));
        
        if (!firstel)
            firstel = nxtel;
            
        if (el)
            el->next = nxtel;
            
        el = nxtel;
        
        if (tstream.GetType() != ',' && IsNotTerm(terms, tstream.GetType()) )
        {
            if (IsTerm(terms, EOF))
                return firstel;
            else
                Expected(terms[0]);
        }
        
        if (tstream.GetType() == ',')
        {
            Match(',');
            
            if (IsTerm(terms, tstream.GetType()) && !optcomma)
                Unexpected(terms[0]);
        }
    }
    
    return firstel;
}

Expr* Parser::DoFunctionExpression()
{
    Expr*      expr = 0;
    int        line = tstream.GetLineNumber();
    size_t     beg  = tstream.curr.begOffset;
    ParamDecl* args = 0;
    
    Match(TOK_function);
    if (tstream.GetType() == '(')
    {
        args = DoFunctionArguments();
    }
    
    Stmt*  body = DoFunctionBody();
    size_t end  = tstream.prev.endOffset;
    
    PIKA_NEWNODE(FunExpr, expr, (args, body, beg, end));
    expr->line = line;
    
    return expr;
}

struct CompItem : TreeNode
{
    CompItem(ForToHeader h, Expr* c, bool unless) 
        : header(h), cond(c), isunless(unless), symbol(0), next(0) 
    {}
    
    virtual ~CompItem() {}
    
    virtual void CalculateResources(SymbolTable* st, CompileState& cs)
    {
        symbol = cs.CreateLocalPlus(st, header.id->name, 2);
        header.id  ->CalculateResources(st, cs);
        header.to  ->CalculateResources(st, cs);
        header.step->CalculateResources(st, cs);
        
        if (cond)        
            cond->CalculateResources(st, cs);        
        if (next) 
            next->CalculateResources(st, cs);
    }
    
    Instr* DoGen(Instr* iexpr, Instr* itarget, Instr* iend)
    {
        return 0;
    }
    
    ForToHeader header;
    Expr*       cond;
    bool        isunless;
    Symbol*     symbol;
    CompItem*   next;
};

struct ArrayComp : Expr
{
    ArrayComp(Expr* e, CompItem* i)
            : Expr(Expr::EXPR_arraycomp),
            symtab(0),
            expr(e),
            items(i) {}
            
    virtual ~ArrayComp()
    {
        Pika_delete(symtab);
    }
    
    virtual void DoStmtResources(SymbolTable* st, CompileState& cs)
    {
        PIKA_NEW(SymbolTable, symtab, (st, false, false));
        incr_off = state->NextLocalOffset("");
        items->CalculateResources(symtab, cs);
        expr->CalculateResources(symtab, cs);
    }
    
    Instr* IncrLoop()
    {
        Instr* ipushlocal = Instr::Create(OP_pushlocal);
        Instr* iopincr    = Instr::Create(OP_inc);
        Instr* isetlocal  = Instr::Create(OP_setlocal);
        
        ipushlocal->Attach(iopincr)->Attach(isetlocal);
        ipushlocal->operand = isetlocal->operand = incr_off;
        return ipushlocal;
    }
    
    virtual Instr* GenerateCode()
    {
        //  [for if?]+ expr
        // BEGIN BLOCK
        Instr* iexpr   = expr->GenerateCode();
        Instr* iincr   = IncrLoop();
        Instr* itarget = Instr::Create(JMP_TARGET);
        Instr* iend    = Instr::Create(JMP_TARGET);
        
        iexpr        ->
        Attach(iincr)->
        Attach(itarget)
        ;
        Instr* icomp = items->DoGen(iexpr, itarget, iend);
        // END BLOCK
        return icomp;
    }
    
    int          incr_off;
    SymbolTable* symtab;
    Expr*        expr;
    CompItem*    items;
};

Expr* Parser::DoArrayExpression()
{
    Expr* expr = 0;
    
    Match('[');
    
    const int arr_terms[] = { ']', /*':',*/ 0 };
    ExprList* elems = DoOptionalExpressionList(arr_terms, true);
    /*
    // If we got only a single expression.
    if (elems && elems->next == 0)
    {
        // Check to see if this is an array comprehension expression.
        if (Optional(':'))
        {
            CompItem* head = 0;
            CompItem* curr = 0;
            do
            {
                ForToHeader header = { 0, 0, 0, 0, false, 0 };
                Expr* cond   = 0;
                bool unless  = false;
                
                DoForToHeader(&header);
                
                if (Optional(TOK_if))
                {
                    cond = DoExpression();
                }
                else if (Optional(TOK_unless))
                {
                    cond = DoExpression();
                    unless = true;
                }
                CompItem* item = 0;
                PIKA_NEWNODE(CompItem, item, (header, cond, unless));
                
                if (!head)
                {
                    curr = head = item;
                }
                else
                {
                    curr->next = item;
                    curr = item;
                }
            }
            while (tstream.GetType() == TOK_for);
            
            //PIKA_NEWNODE(ArrayComp, expr, (elems->expr, head));
            //int compline = tstream.GetLineNumber();    
            //Match(']');
            //expr->line = compline;
            //return expr;
        }
    }*/
    int line = tstream.GetLineNumber();
    
    Match(']');
    
    PIKA_NEWNODE(ArrayExpr, expr, (elems));
    expr->line = line;
    
    return expr;
}

Expr* Parser::DoDictionaryExpression()
{
    Expr* expr = 0;
    
    Match('{');
    
    FieldList* fields = DoDictionaryExpressionFields();
    
    int line = tstream.GetLineNumber();
    
    Match('}');
    
    PIKA_NEWNODE(DictionaryExpr, expr, (fields));
    expr->line = line;
    
    return expr;
}

FieldList* Parser::DoDictionaryExpressionFields()
{
    FieldList* fields = 0;
    
    while (tstream.GetType() != '}' && !tstream.IsEndOfStream())
    {
        Expr* name = 0;
        Expr* value = 0;
        
        if (tstream.GetType() == TOK_function)
        {
            int line = tstream.GetLineNumber();
            size_t beg = tstream.curr.begOffset;
            
            Match(TOK_function);
            
            Id*        id   = DoIdentifier();
            ParamDecl* args = DoFunctionArguments();
            Stmt*      body = DoFunctionBody();
            size_t     end  = tstream.prev.endOffset;
            
            PIKA_NEWNODE(FunExpr, value, (args, body, beg, end, id));
            
            value->line = line;
        }
        else if (tstream.GetType() == TOK_property)
        {
            const char* getstr   = "get";
            const char* setstr   = "set";
            const char* next     = 0;
            bool        getfirst = false;
            
            Match(TOK_property);
            
            name = DoFieldName();
            
            if (tstream.GetType() != TOK_identifier)
            {
                Unexpected(tstream.GetType());
            }
            
            if (DoContextualKeyword(getstr, true))
            {
                getfirst = true;
                next = setstr;
            }
            else if (DoContextualKeyword(setstr, true))
            {
                getfirst = false;
                next = getstr;
            }
            else
            {
                Unexpected(tstream.GetType());
            }
            
            Match(':');
            
            Expr* firstexpr = DoExpression();
            Expr* secondexpr = 0;
            
            DoEndOfStatement();
            
            if (tstream.GetType() == TOK_identifier)
            {
                DoContextualKeyword(next, false);
                Match(':');
                secondexpr = DoExpression();
                DoEndOfStatement();
            }
            
            Match(TOK_end);
            
            if (getfirst)
            {
                PIKA_NEWNODE(PropExpr, value, (name, firstexpr, secondexpr));
            }
            else
            {
                PIKA_NEWNODE(PropExpr, value, (name, secondexpr, firstexpr));
            }
            
            value->line = name->line;
            name = 0;
        }
        else
        {
            name = (tstream.GetType() == TOK_identifier) ?
                   DoFieldName()                         :
                   DoStringLiteralExpression();
                   
            Match(':');
            
            value = DoExpression();
        }
        
        if (!DoCommaOrNewline())
        {
            if (tstream.GetType() != '}')
            {
                Expected('}');
            }
        }
        
        FieldList* nxt = 0;
        PIKA_NEWNODE(FieldList, nxt, (name, value));
        
        if (fields)
        {
            fields->Attach(nxt);
        }
        else
        {
            fields = nxt;
        }
    }
    return fields;
}

ParamDecl* Parser::DoFunctionArguments()
{
    ParamDecl* args     = 0;
    ParamDecl* currarg  = 0;
    bool       def_vals = false;
    bool       close    = false;
    
    if (tstream.GetPreviousLineNumber() != tstream.GetLineNumber())
    {
        close = true;
        if (!Optional('('))
            return 0;
    }
    else
    {
        close = Optional('(');
    }
    
    while (tstream.GetType() != ')' && !tstream.IsEndOfStream())
    {
        bool rest_param = false;
        Id* id = 0;
        Expr* def_expr = 0;
        
        if (tstream.GetType() == TOK_rest)
        {
            Match(TOK_rest);
            rest_param = true;
        }
        
        id = DoIdentifier();
        
        if (Optional('='))
        {
            if (rest_param)
            {
                state->SyntaxError(tstream.GetLineNumber(), "rest argument declaration (...) cannot have a default value");
            }
            def_vals = true;
            def_expr = DoExpression();
        }
        
        if (def_vals && !def_expr && !rest_param)
        {
            state->SyntaxError(tstream.GetLineNumber(), "Expected default value for argument '%s'.", id->name);
        }
        
        PIKA_NEWNODE(ParamDecl, currarg, (id, rest_param, def_expr));
        currarg->line = id->line;
        
        if (!args)
        {
            args = currarg;
        }
        else
        {
            args->Attach(currarg);
        }
        
        if (tstream.GetType() != ')')
        {
            if (rest_param || tstream.GetType() != ',')
            {
                if (rest_param)
                {
                    state->SyntaxError(tstream.GetLineNumber(), "A variable argument declaration must be the last argument.");
                }
                
                if (close)
                {
                    Expected(')');
                }
                else
                {
                    break;
                }
            }
            else
            {
                Match(',');
            }
        }
    }
    
    if (close)
    {
        Match(')');
    }
    
    return args;
}

}// pika
