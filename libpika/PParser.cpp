/*
*  gParser.cpp
*  See Copyright Notice in Pika.h
*/
#include "Pika.h"
#include "PSymbolTable.h"
#include "PAst.h"
#include "PParser.h"
#include "PInstruction.h"

namespace pika {

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

// Needed for REPL mode. Returns true if the array 'terms' is non-empty.
INLINE bool HasTerm(const int* terms)
{
    size_t count = 0;
    for (const int* c = terms; *c; ++c)
    {
        ++count;
    }
    return count > 0;
}

/* Returns true if the token 'tok' IS a one of the terminating tokens from the array 'terms'. */

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

const int Parser::Static_end_terms[] = { TOK_end, 0 };
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
#       include "PTokenDef.inl"
    };
    return names;
}

const int Token2String::min  = TOK_global;
const int Token2String::max  = TOK_max;
const int Token2String::diff = TOK_global;


// TokenStream /////////////////////////////////////////////////////////////////////////////////////

TokenStream::TokenStream(CompileState* state, std::ifstream* yyin)
    : state(state), tokenizer(0),
    yyin(yyin)        
{
    InitTokens();
    PIKA_NEW(Tokenizer, tokenizer, (state, yyin));
}

TokenStream::TokenStream(CompileState* state, const char* buffer, size_t len)
    : state(state), tokenizer(0),
    yyin(0)        
{
    InitTokens();
    PIKA_NEW(Tokenizer, tokenizer, (state, buffer, len));
}
    
TokenStream::TokenStream(CompileState* state, IScriptStream* stream) 
: state(state), tokenizer(0),
yyin(0) 
{
    InitTokens();
    PIKA_NEW(Tokenizer, tokenizer, (state, stream));
}

void TokenStream::InitTokens()
{
    prev.tokenType = curr.tokenType = next.tokenType = EOF;
    Pika_memzero(&prev, sizeof(Token));
    Pika_memzero(&curr, sizeof(Token));
    Pika_memzero(&next, sizeof(Token)); 
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
    
    if (nxttok != EOF && nxttok != EOI)
    {
        next.tokenType = nxttok;
        next.line = tokenizer->GetLn();
        next.col = tokenizer->GetCol();
        next.begOffset = tokenizer->GetBeginOffset();
        next.endOffset = tokenizer->GetEndOffset();
        next.value = tokenizer->GetVal();
    }
    else
    {
        next.tokenType = nxttok;
    }
}

void TokenStream::GetMore()
{ 
    if (curr.tokenType == EOI)
    {
        Token oldprev = prev;
        
        tokenizer->GetMoreInput(); 
        Advance(); 
        Advance();
        
        if (prev.tokenType == EOI)
        {
            prev = oldprev;
        }
    }
}

void TokenStream::GetNextMore()
{ 
    tokenizer->GetMoreInput();
    tokenizer->GetNext();
    int nxttok = tokenizer->GetTokenType();
    if (nxttok != EOF && nxttok != EOI)
    {
        next.tokenType = nxttok;
        next.line = tokenizer->GetLn();
        next.col = tokenizer->GetCol();
        next.begOffset = tokenizer->GetBeginOffset();
        next.endOffset = tokenizer->GetEndOffset();
        next.value = tokenizer->GetVal();
    }
    else
    {
        next.tokenType = nxttok;
    }    
}

// Parser //////////////////////////////////////////////////////////////////////////////////////////

Parser::Parser(CompileState *cs, std::ifstream* yyin)
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

    Parser::Parser(CompileState* cs, IScriptStream* stream) : root(0),
    state(cs),
    tstream(state, stream)
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
    const int terms[] = { 0 };
    
    size_t beg   = tstream.curr.begOffset;
    Stmt*  stmts = DoStatementList(terms);
    size_t end   = tstream.prev.endOffset;
    
    PIKA_NEWNODE(Program, root, (state, stmts, beg, end));
}

void Parser::DoFunctionScript()
{
    const int terms[] = {TOK_end};
    
    Match(TOK_function);
    
    BufferCurrent();
    if (tstream.GetType() != '(')
        Expected('(');
        
    ParamDecl* params = DoFunctionParameters();
    
    size_t beg   = tstream.curr.begOffset;
    Stmt*  stmts = DoStatementList(terms);
    size_t end   = tstream.prev.endOffset;
    
    Match(TOK_end);
    PIKA_NEWNODE(FunctionProg, root, (state, stmts, beg, end, params));
}

void Parser::Match(int x)
{
    if (tstream.GetType() == EOI)
    {
        // If were in REPL mode.
        if (tstream.More())
        {
            // Get more tokens
            tstream.GetMore();            
        }  
        // continue as though nothing happened.
    }
    if (tstream.GetType() == x)
        tstream.Advance();
    else
        Expected(x);
}

bool Parser::Optional(int x)
{
    if (tstream.GetType() == x)
    {
        tstream.Advance();
        return true;
    }
    return false;
}

void Parser::Expected(int x)
{
    int line = tstream.GetLineNumber();
    int col  = tstream.GetCol();
    
    if (x >= Token2String::min && x <= Token2String::max)
    {
        state->SyntaxException(Exception::ERROR_syntax, line, col, "Expected token '%s'.", Token2String::GetNames()[x-Token2String::diff]);
    }
    else if (x <= CHAR_MAX && x >= CHAR_MIN)
    {
        state->SyntaxException(Exception::ERROR_syntax, line, col, "Expected token '%c'.", (char)x);
    }
    else
    {
        state->SyntaxException(Exception::ERROR_syntax, line, col, "Expected token %d.", x);
    }
}

void Parser::Unexpected(int x)
{
    int line = tstream.GetLineNumber();
    int col = tstream.GetCol();
    if (x == EOF)
    {
        state->SyntaxException(Exception::ERROR_syntax, line, col, "unexpected end of file reached.");
    }
    else if (x == EOI)
    {
        state->SyntaxException(Exception::ERROR_syntax, line, col, "unexpected end of input reached.");
    }
    else if (x >= Token2String::min && x <= Token2String::max)
    {
        state->SyntaxException(Exception::ERROR_syntax, line, col, "unexpected token '%s'.", Token2String::GetNames()[x-Token2String::diff]);
    }
    else if (x <= CHAR_MAX && x >= CHAR_MIN)
    {
        state->SyntaxException(Exception::ERROR_syntax, line, col, "unexpected token '%c'.", (char)x);
    }
    else
    {
        state->SyntaxException(Exception::ERROR_syntax, line, col, "unexpected token %d.", x);
    }
}

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
                               tstream.GetCol(),
                               "end of statement required. expected ';' or \\n");
    }
}

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

bool Parser::IsEndOfStatement()
{
    if (tstream.GetType() == ';')
    {
        return true;
    }
    else if (tstream.GetType() != TOK_end && tstream.GetType() != TOK_else && tstream.GetType() != '}')
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
        PIKA_NEWNODE(EmptyStmt, stmt, (state));
        stmt->line = line;
    }
    break;
    case '@':
    {
        AnnotationDecl* annotations = 0;
        while (tstream.GetType() == '@')
        {
            int start_line = tstream.GetLineNumber();
            tstream.Advance();
            BufferCurrent();
            
            if (tstream.GetLineNumber() != start_line)
            {
                state->SyntaxException(Exception::ERROR_syntax, start_line, "annotation must be contained to a single line.");
            }
            NameNode* name = DoNameNode(true);
            BufferCurrent();
            ExprList* deco_args = 0;
            if (tstream.GetLineNumber() == start_line)
            {
                if (Optional('('))
                {
                    const int annotation_terms[] = { ')', EOI, 0 };
                    deco_args = DoOptionalExpressionList(annotation_terms, true); 
                    Match(')');                    
                }
                else
                {
                    deco_args = DoExpressionList(true);
                }
            }            
            AnnotationDecl* deco_new = 0;
            PIKA_NEWNODE(AnnotationDecl, deco_new, (state, name, deco_args));
            if (!annotations)
            {
                annotations = deco_new;
            }
            else
            {
                annotations->Attach(deco_new);
            }
            if (tstream.GetType() == EOI)
                break;
            DoEndOfStatement();
        }
        BufferCurrent();
        stmt = DoStatement(false);
        if (stmt->kind != Stmt::STMT_decl)
        {
            state->SyntaxException(Exception::ERROR_syntax, stmt->line, "annotation must be followed by a declaration: class, function, package or property.");
        }
        else
        {
            Decl* decl = ((DeclStmt*)stmt)->decl;
            switch (decl->kind)
            {
            case Decl::DECL_function:
            case Decl::DECL_property:
            case Decl::DECL_package:
            case Decl::DECL_class: {
                NamedTarget* target = (NamedTarget*)decl;
                target->annotations = annotations;
                break;
            }
            default:
                state->SyntaxException(Exception::ERROR_syntax, stmt->line, "annotation must be followed by a declaration: class, function, package or property.");
            }
        }
    }
    break;
    case TOK_global:
    case TOK_local:
    case TOK_member:
    {
        BufferNext();
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
    case TOK_package:       stmt = DoPackageDeclaration(); break;
    case TOK_property:      stmt = DoPropertyStatement();  break;    
    case TOK_class:         stmt = DoClassStatement();     break;
        
    case TOK_if:            stmt = DoIfStatement();        break;
    case TOK_while:         stmt = DoWhileStatement();     break;
    case TOK_loop:          stmt = DoLoopStatement();      break;
    case TOK_for:           stmt = DoForStatement();       break;
    case TOK_return:        stmt = DoReturnStatement();    break;
    case TOK_yield:         stmt = DoGenerateStatement();  break;
    case TOK_break:         stmt = DoBreakStatement();     break;
    case TOK_continue:      stmt = DoContinueStatement();  break;
    case TOK_do:            stmt = DoBlockStatement();     break;
    
    break;
    case TOK_using:      stmt = DoUsingStatement();    break;
    case TOK_try:        stmt = DoTryStatement();     break;
    case TOK_raise:      stmt = DoRaiseStatement();   break;
    case TOK_identifier: stmt = DoLabeledStatement(); break; // possible label statement
    default:
        if (!skipExpr)
        {
            stmt = DoExpressionStatement();
        }
        else
        {
            PIKA_NEWNODE(EmptyStmt, stmt, (state));
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

/*
    class <id> [ : <super-expression> ]
    <statements>*
    end
 */
Stmt* Parser::DoClassStatement()
{
    const int class_terms[] = { TOK_end, TOK_finally, 0 };
    Stmt* stmt = 0;
    int line = tstream.GetLineNumber();
    Expr* super = 0;
    Stmt* classbody = 0;
    ClassDecl* decl = 0;
    
    StorageKind sk = DoStorageKind();
    
    BufferNext();
    Match(TOK_class);
    
    NameNode* name = DoNameNode(sk == STO_none);
    
    if (Optional(':'))
    {
        BufferCurrent();
        super = DoExpression();
    }
    
    BufferCurrent();
    classbody = DoStatementList(class_terms);
    
    PIKA_NEWNODE(ClassDecl, decl, (state, name, super, classbody, sk));
    PIKA_NEWNODE(DeclStmt, stmt, (state, decl));
    stmt->line = decl->line = line;
    
    return DoFinallyBlock(stmt);
}

void Parser::DoBlockBegin(int tok, int prevln, int currln)
{
    if (prevln == currln)
    {
        if (Optional(tok))
            return;    
        if (!Optional(';'))
        {
            Expected(';');
        }
        else
        {
            BufferCurrent();
        }
    }
}

LoadExpr* Parser::DoSelfExpression()
{
    LoadExpr* expr = 0;
    int line = tstream.GetLineNumber();
    Match(TOK_self);
    PIKA_NEWNODE(LoadExpr, expr, (state, LoadExpr::LK_self));
    expr->line = line;
    return expr;
}

NameNode* Parser::DoNameNode(bool canbedot)
{
    NameNode* name = 0;
    int line = tstream.GetLineNumber();
    /*
        id0.id1.[...].idN.name
        self.id1.[...].idN.name
     */
    if ((tstream.GetNextType() == '.' || tstream.GetNextType() == '[') && canbedot)
    {
        Expr* expr;
        BufferCurrent();
        if (tstream.GetType() == TOK_identifier)
        {
            expr = DoIdExpression();
        }
        else
        {
            expr = DoSelfExpression();
        }
        
        while (tstream.GetType() == '.' || tstream.GetType() == '[')
        {
            Expr* lhs = expr;
            Expr* rhs = 0;
            
            BufferCurrent();
            if (Optional('.'))
            {
                BufferCurrent();
                Id* id = DoIdentifier();
                PIKA_NEWNODE(MemberExpr, rhs, (state, id));
                rhs->line = id->line;            
                
                PIKA_NEWNODE(DotExpr, expr, (state, lhs, rhs));
                expr->line = rhs->line;                
            }
            else {                
            
                Match('[');
                BufferCurrent();
                rhs = DoExpression();
                Match(']');
                
                PIKA_NEWNODE(IndexExpr, expr, (state, lhs, rhs));
                expr->line = rhs->line;
            }
        }
        
        if (expr->kind == Expr::EXPR_dot)
        {
            DotExpr* de = (DotExpr*)expr;
            PIKA_NEWNODE(NameNode, name, (state, de));
            name->line = line;
        }
        else
        {
            state->SyntaxException(Exception::ERROR_syntax, expr->line, "invalid name specified.");
        }
    }
    else
    {
        IdExpr* idexpr = DoIdExpression();
        PIKA_NEWNODE(NameNode, name, (state, idexpr));
        name->line = line;
    }
    return name;
}

Stmt* Parser::DoPackageDeclaration()
{
    int line = tstream.GetLineNumber();
    PkgDecl* decl = 0;
    Stmt* stmt = 0;    
    StorageKind sk = DoStorageKind();
    
    BufferNext();
    Match(TOK_package);
     
    NameNode* id = DoNameNode(sk == STO_none);
    
    BufferCurrent();
    Stmt* body = DoStatementList(Static_end_terms);
    
    Match(TOK_end);
    
    PIKA_NEWNODE(PkgDecl, decl, (state, id, body, sk));
    PIKA_NEWNODE(DeclStmt, stmt, (state, decl));
    
    stmt->line = decl->line = line;
    
    return stmt;
}

Stmt* Parser::DoPropertyStatement()
{
    Stmt* stmt = 0;
    PropertyDecl* decl = 0;
    const char* getstr = "get";
    const char* setstr = "set";
    const char* next = 0;
    bool getfirst = false;
    
    StorageKind sk = DoStorageKind();

    BufferNext();
    Match(TOK_property);
    
    NameNode* name = DoNameNode(sk == STO_none);

    BufferCurrent();    
    // Next character must be get or set.
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
        state->SyntaxException(Exception::ERROR_syntax, tstream.GetLineNumber(),  tstream.GetCol(), "expected identifier '%s' or '%s'.", getstr, setstr);
    }
       
    Match(':');
    
    BufferCurrent();    
    Expr* firstexpr = DoExpression();
    Expr* secondexpr = 0;
    
    DoEndOfStatement();
    
    BufferCurrent();
    if (tstream.GetType() == TOK_identifier)
    {
        DoContextualKeyword(next, false);
        Match(':');
        
        BufferCurrent();
        secondexpr = DoExpression();
        DoEndOfStatement();
    }
    
    Match(TOK_end);
    
    if (getfirst)
    {
        PIKA_NEWNODE(PropertyDecl, decl, (state, name, firstexpr, secondexpr, sk));
    }
    else
    {
        PIKA_NEWNODE(PropertyDecl, decl, (state, name, secondexpr, firstexpr, sk));
    }
    PIKA_NEWNODE(DeclStmt, stmt, (state, decl));
    stmt->line = decl->line = name->line;
    
    return stmt;
}

Stmt* Parser::DoStatementList(const int* terms)
{
    Stmt* currstmt = 0;
    
    while (IsNotTerm(terms, tstream.GetType()) && !tstream.IsEndOfStream())
    {
        Stmt* nxtstmt = DoStatement(false);
        if (HasTerm(terms) && IsNotTerm(terms, tstream.GetType()))
            BufferCurrent();
        if (currstmt)
        {
            Stmt* stmtseq = 0;
            PIKA_NEWNODE(StmtList, stmtseq, (state, currstmt, nxtstmt));
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
        PIKA_NEWNODE(EmptyStmt, currstmt, (state));
        currstmt->line = tstream.GetLineNumber();
    }
    
    return currstmt;
}

Stmt* Parser::DoFunctionBody()
{
    BufferCurrent();
    const int terms[] = { TOK_end, TOK_finally, 0 };
    Stmt* stmt = DoStatementList(terms);
    return DoFinallyBlock(stmt);
}

Stmt* Parser::DoStatementListBlock(const int* terms)
{
    Stmt* stmt = 0;
    Stmt* stmtList = DoStatementList(terms);
    
    PIKA_NEWNODE(BlockStmt, stmt, (state, stmtList));
    stmt->line = stmtList->line;
    return stmt;
}

Stmt* Parser::DoFinallyBlock(Stmt* in)
{
    Stmt* out = 0;
    
    BufferCurrent();
    if (Optional(TOK_finally))
    {
        BufferCurrent();
        Stmt* finalizeBlock = DoStatementList(Static_end_terms);
        
        Match(TOK_end);
        
        PIKA_NEWNODE(FinallyStmt, out, (state, in, finalizeBlock));
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
    BufferNext();
    Match(TOK_do);
    const int terms[] = { TOK_end, TOK_finally, 0 };
    Stmt* block = DoStatementListBlock(terms);
    BufferCurrent();
    
    return DoFinallyBlock(block);
}

Stmt* Parser::DoUsingStatement()
{
    int line = tstream.GetLineNumber();
    
    Match(TOK_using);
    
    Expr* with = DoExpression();
    BufferCurrent();
    DoBlockBegin(TOK_do, with->line, tstream.GetLineNumber());
    
    const int terms[] = { TOK_end, TOK_finally, 0 };
    Stmt* block = DoStatementList(terms);
    
    UsingStmt* stmt;
    PIKA_NEWNODE(UsingStmt, stmt, (state, with, block));
    stmt->line = line;
    
    if (tstream.GetType() == TOK_end)
    {
        int eline = tstream.GetLineNumber();
        
        Match(TOK_end);
        Stmt* out = 0;
        Stmt* finalizeBlock = 0;
        PIKA_NEWNODE(EmptyStmt, finalizeBlock, (state));
        finalizeBlock->line = eline;
        PIKA_NEWNODE(FinallyStmt, out, (state, stmt, finalizeBlock));
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
    BufferCurrent();
    
    const int try_terms[] = { TOK_catch, TOK_finally, 0 };
    Stmt* tryblock = DoStatementList(try_terms);
    BufferCurrent();
    
    const int terms[] = { TOK_end, TOK_catch, TOK_else, TOK_finally, 0 };
        
    while (tstream.GetType() == TOK_catch)
    {
        Match(TOK_catch);
        BufferCurrent();
        
        Id* id = DoIdentifier();
        BufferCurrent();
        
        if (Optional(TOK_is))
        {            
            BufferCurrent();
            Expr* is = DoExpression();
            
            BufferCurrent();
            DoBlockBegin(TOK_then, id->line, tstream.GetLineNumber());
            
            Stmt* body = DoStatementListBlock(terms);
            
            BufferCurrent();
            CatchIsBlock* c;
            PIKA_NEWNODE(CatchIsBlock, c, (state, id, is, body));
            
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
            DoBlockBegin(TOK_then, caughtvar->line, tstream.GetLineNumber());
            
            catchblock = DoStatementListBlock(terms);
            BufferCurrent();
            break;
        }
    }
    
    Stmt* elsebody = 0;
    if (Optional(TOK_else))
    {
        BufferCurrent();
        const int elseterms[] = { TOK_end, TOK_finally, 0 };
        elsebody = DoStatementListBlock(elseterms);
        BufferCurrent();
    }
    
    Stmt* stmt = 0;
    PIKA_NEWNODE(TryStmt, stmt, (state, tryblock, catchblock, caughtvar, cis, elsebody));
    stmt->line = line;
    
    return DoFinallyBlock(stmt);
}

Stmt* Parser::DoLabeledStatement()
{
    if (tstream.GetType() != TOK_identifier)
        Expected(TOK_identifier);
        
    if (tstream.GetNextType() == ':' && tstream.GetLineNumber() == tstream.GetNextLineNumber())
    {
        // <identifier> ':' <statement>
        
        Id* id = DoIdentifier();
        Match(':');
        BufferCurrent();
        Stmt* stmt    = DoStatement(true);
        Stmt* lblstmt = 0;
        
        PIKA_NEWNODE(LabeledStmt, lblstmt, (state, id, stmt));
        
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
    PIKA_NEWNODE(DeclStmt, stmt, (state, decl));
    
    stmt->line = decl->line;
    
    return stmt;
}

Decl* Parser::DoVarDeclaration()
{
    // local  var1, ..., varN [ = expr1, ..., exprN ]
    // member var1, ..., varN [ = expr1, ..., exprN ]
    // global var1, ..., varN [ = expr1, ..., exprN ]
    
    VarDecl* decl = 0;
    VarDecl* firstdecl = 0;
    bool is_local = false;
    bool is_member = false;
    StorageKind sto = STO_none;
    
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
    BufferCurrent();
    bool has_placeholders = false;
    do
    {
        VarDecl* nxtdecl = 0;
        Id* name = 0;
        int line = 0;
        if (tstream.GetType() == TOK_null) {
            // This is a place holder, meaning the programmer wants to skip the
            // value.
            //
            // a, b, null, c = 1, 2, 3, 4
            //
            line = tstream.GetLineNumber();
            tstream.Advance();
            has_placeholders = true;
        } else {
            name = DoIdentifier();
            line = name->line;
        }
        
        if (is_local)
        {
            PIKA_NEWNODE(LocalDecl, nxtdecl, (state, name));
        }
        else if (is_member)
        {
            PIKA_NEWNODE(MemberDeclaration, nxtdecl, (state, name));
        }
        else
        {
            PIKA_NEWNODE(VarDecl, nxtdecl, (state, name));
        }
        
        nxtdecl->line = line;
        
        if (!firstdecl)
            firstdecl = nxtdecl;
            
        if (decl)
            decl->next = nxtdecl;
            
        decl = nxtdecl;
        
        if (tstream.GetType() != ',')
            break;
            
        Match(',');
        BufferCurrent();
    }
    while (!tstream.IsEndOfStream());
    
    if (Optional('='))
    {
        BufferCurrent();
        VariableTarget* vt;
        
        ExprList* el = DoExpressionList();
        PIKA_NEWNODE(VariableTarget, vt, (state, firstdecl, el, sto));
        vt->line = firstdecl->line;
        return vt;
    }
    else if (has_placeholders)
    {
        Expected('=');
    }
    return firstdecl;
}

void Parser::BufferNext()
{
    // TODO: Does the current type need to be checked or can we assume that next==EOI iff curr!=EOF and curr != EOI ?
    if (tstream.GetNextType() == EOI)
    {
        tstream.GetNextMore();          
    }
}

void Parser::BufferCurrent()
{
    if (tstream.More())
        tstream.GetMore();
}

Stmt* Parser::DoFunctionStatement()
{
    FunctionDecl* decl = 0;
    Stmt* stmt = 0;
    size_t beg  = tstream.curr.begOffset;
    
    StorageKind sk = DoStorageKind();
    
    BufferNext();
    
    if (TOK_function   == tstream.GetType()     && 
        TOK_identifier != tstream.GetNextType() &&
        TOK_self       != tstream.GetNextType() &&
        STO_none       == sk) // expression cannot have a storage kind
    {
        return DoExpressionStatement(); // possible a function expression (definitely not a function statement)
    }
    
    Match(TOK_function);
    
    NameNode* name = DoNameNode(sk == STO_none); // only allow dot names for functions without a storage specifier.
    ParamDecl* params = DoFunctionParameters();
    
    Stmt* body = DoFunctionBody();
    size_t end = tstream.prev.endOffset;
    
    PIKA_NEWNODE(FunctionDecl, decl, (state, name, params, body, beg, end, sk));
    PIKA_NEWNODE(DeclStmt, stmt, (state, decl));
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
    
    BufferNext();
    
    Match(TOK_if);

    BufferNext();
    
    cond = DoExpression();
    
    BufferCurrent();
    DoBlockBegin(TOK_then, cond->line, tstream.GetLineNumber());
    
    const int first_terms[] = { TOK_end, TOK_elseif, TOK_else, TOK_finally, 0 };
    const int terms[] = { TOK_end, TOK_elseif, TOK_else, 0 };
    body = DoStatementListBlock(first_terms);
    
    PIKA_NEWNODE(IfStmt, stmt, (state, cond, body, unless));
    stmt->line = cond->line;
    
    while (Optional(TOK_elseif))
    {
        unless = false;
        
        IfStmt* elseifStmt = 0;
        // elseif <expr> then stmt

        BufferCurrent();        
        cond = DoExpression();
        
        DoBlockBegin(TOK_then, cond->line, tstream.GetLineNumber());
        
        body = DoStatementListBlock(terms);
        
        PIKA_NEWNODE(IfStmt, elseifStmt, (state, cond, body, unless));
        elseifStmt->line = cond->line;
        stmt->Attach(elseifStmt);
    }
    
    if (Optional(TOK_else))
    {
        BufferCurrent();
        elsePart = DoStatementListBlock(Static_end_terms);
    }
    
    Match(TOK_end);
    
    Stmt* condStmt = 0;
    PIKA_NEWNODE(ConditionalStmt, condStmt, (state, stmt, elsePart));
    condStmt->line = stmt->line;
    
    condStmt->line = cond->line;
    return condStmt;
}

Stmt* Parser::DoWhileStatement()
{
    Expr* cond = 0;
    Stmt* body = 0;
    Stmt* stmt = 0;
    
    BufferNext();
    Match(TOK_while);
    
    cond = DoExpression();
    BufferCurrent(); 
    DoBlockBegin(TOK_do, cond->line, tstream.GetLineNumber());

    body = DoStatementListBlock(Static_finally_terms);
    
    PIKA_NEWNODE(CondLoopStmt, stmt, (state, cond, body, false, false));
    stmt->line = cond->line;

    return DoFinallyBlock(stmt);
}

Stmt* Parser::DoLoopStatement()
{
    Stmt* body = 0;
    Stmt* stmt = 0;
    int line = tstream.GetLineNumber();
    
    Match(TOK_loop);
    
    BufferCurrent();
    body = DoStatementListBlock(Static_finally_terms);
    
    
    PIKA_NEWNODE(LoopStmt, stmt, (state, body));
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
                state->SyntaxException(Exception::ERROR_syntax, tstream.GetLineNumber(),  tstream.GetCol(), "expected token '%s'.", key);
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
    Match('=');
    BufferCurrent();
    
    header->from = DoExpression();
    BufferCurrent();

    Match(TOK_to);
    header->isdown = false;
    BufferCurrent();
    
    header->to = DoExpression();
    
    BufferCurrent();
    if (Optional(TOK_by))
    {
        BufferCurrent();
        header->step = DoExpression();
    }
    else
    {
        Expr* estep;
        PIKA_NEWNODE(IntegerExpr, estep, (state, (header->isdown) ? -1 : 1));
        estep->line = header->to->line;
        header->step = estep;
    }    
}

Stmt* Parser::DoForStatement()
{
    ForHeader header = { 0, 0 };
    
    int line = tstream.GetLineNumber();
    header.line = line;
        
    Match(TOK_for);
    Id* id = 0;
    size_t num_ids = 0;
    do
    {
        BufferCurrent();        
        ++num_ids;
        Id* nid = DoIdentifier();
        if (!id) {
            id = nid;
        }
        else {
            id->Attach(nid);
        }
    } 
    while (Optional(','));
    
    header.id = id;
    BufferCurrent();    
    if (tstream.GetType() == TOK_in)
    {
        return DoForEachStatement(&header);
    }
    else
    {
        if (num_ids != 1)
            state->SyntaxError(tstream.GetLineNumber(), "for to statement can have only 1 loop variable.");
        return DoForToStatement(&header);
    }
    return 0;
}

Stmt* Parser::DoForToStatement(ForHeader* fh)
{
    Stmt* stmt = 0;
    Stmt* body = 0;
    
    ForToHeader header = { {0, 0}, 0, 0, 0, false };
    header.head = *fh;
    DoForToHeader(&header);
    
    BufferCurrent();
    DoBlockBegin(TOK_do, header.step->line, tstream.GetLineNumber());
    
    body = DoStatementListBlock(Static_finally_terms);
    
    PIKA_NEWNODE(ForToStmt, stmt, (state, header.head.id, header.from, header.to, header.step, body, header.isdown));
    stmt->line = header.head.line;
    return DoFinallyBlock(stmt);
}

void Parser::DoForEachHeader(ForEachHeader* header)
{
    Match(TOK_in);
    BufferCurrent();
    BufferNext();
    if (tstream.GetType() == TOK_identifier && tstream.GetNextType() == TOK_of)
    {
        header->of = DoFieldName();
        Match(TOK_of);
        BufferCurrent();
    }
    else if (tstream.GetType() == TOK_stringliteral && tstream.GetNextType() == TOK_of)
    {
        header->of = DoStringLiteralExpression();
        Match(TOK_of);
        BufferCurrent();
    }
    else
    {
        StringExpr* of;
        PIKA_NEWNODE(StringExpr, of, (state, Pika_strdup(""), 0));
        of->line = header->head.line;
        header->of = of;
    }
    BufferCurrent();
    header->subject = DoExpression();
}

Stmt* Parser::DoForEachStatement(ForHeader* fh)
{
    ForEachHeader header = { {0,0}, 0, 0 };
    header.head = *fh;
    DoForEachHeader(&header);

    BufferCurrent();
    DoBlockBegin(TOK_do, header.subject->line, tstream.GetLineNumber());
        
    Stmt* body = DoStatementListBlock(Static_finally_terms);    
    Stmt* stmt = 0;
    PIKA_NEWNODE(ForeachStmt, stmt, (state, header.head.id, header.of, header.subject, body));
    stmt->line = header.head.line;
       
    return DoFinallyBlock(stmt);
}

StringExpr* Parser::DoFieldName()
{
    StringExpr* name = 0;
    
    if (tstream.GetType() == TOK_identifier)
    {
        char* idstr = tstream.GetString();
        
        PIKA_NEWNODE(StringExpr, name, (state, idstr, tstream.GetStringLength()));
        
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
    
    PIKA_NEWNODE(RaiseStmt, stmt, (state, expr));
    
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
    
    if (!IsEndOfStatement())
    {
        exprList = DoExpressionList();
    }
    
    PIKA_NEWNODE(CtrlStmt, stmt, (state, exprList, Stmt::STMT_return));
    
    if (exprList)
    {
        stmt->line = exprList->line;
    }
    else
    {
        stmt->line = line;
    }
        
    DoEndOfStatement();
    
    return stmt;
}

Stmt* Parser::DoGenerateStatement()
{
    Stmt* stmt = 0;
    ExprList* exprList = 0;
    int line = tstream.GetLineNumber();
    
    Match(TOK_yield);
    
    if (!IsEndOfStatement())
    {
        exprList = DoExpressionList();
    }
    
    PIKA_NEWNODE(CtrlStmt, stmt, (state, exprList, Stmt::STMT_generate));
    
    if (exprList)
    {
        stmt->line = exprList->line;
    }
    else
    {
        stmt->line = line;
    }
        
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
    
    PIKA_NEWNODE(BreakStmt, stmt, (state, id));
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
    
    PIKA_NEWNODE(ContinueStmt, stmt, (state, id));
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
        
    if (tstream.GetType() == TOK_if)
    {
        bool unless = false;
        
        Match(TOK_if);

        BufferCurrent();
        Expr* cond = DoExpression();
        Stmt* ifstmt = 0;
        
        PIKA_NEWNODE(IfStmt, ifstmt, (state, cond, stmt, unless));
        
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
    case TOK_idivassign:    
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
    case TOK_idivassign:   return AssignmentStmt::IDIV_ASSIGN_STMT;
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
        BufferNext();
        tstream.Advance();
        
        ExprList* lhs = expr;
        ExprList* rhs = DoExpressionList();
        
        DoEndOfStatement();
        PIKA_NEWNODE(AssignmentStmt, stmt, (state, lhs, rhs));
        
        if (akind != AssignmentStmt::ASSIGN_STMT)
        {
            ((AssignmentStmt*)stmt)->kind = akind;
            ((AssignmentStmt*)stmt)->isBinaryOp = true;
        }
    }
    else
    {
        DoEndOfStatement();
        PIKA_NEWNODE(ExprStmt, stmt, (state, expr));
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
    return DoConditionalExpression();
}

// cond ? expr : expr
Expr* Parser::DoConditionalExpression()
{
    Expr* expr = DoNullSelectExpression();
    
    if (tstream.GetType() == '?')
    {        
        Expr* cond = expr;
        
        BufferNext();        
        tstream.Advance();
        
        Expr* then = DoExpression();
        
        Match(':');
        BufferCurrent();
        
        Expr* otherwise = DoConditionalExpression();
        
        PIKA_NEWNODE(CondExpr, expr, (state, cond, then, otherwise));
        expr->line = cond->line; // !!! set line number
    } else if (tstream.GetType() == TOK_if) {
        int line = tstream.GetLineNumber();
        
        if (line > expr->line)
            return expr;
        bool unless = false;

        BufferNext();
        Match(TOK_if);
        
        Expr* cond  = DoExpression();
        
        BufferNext();
        Match(TOK_else);
        
        Expr* other = DoConditionalExpression();
                    
        Expr* resExpr = 0;
        PIKA_NEWNODE(CondExpr, resExpr, (state, cond, expr, other, unless));
        resExpr->line = expr->line;
        return resExpr;
    }
    return expr;
}

// lhs ?? rhs
Expr* Parser::DoNullSelectExpression()
{
    Expr* expr = DoConcatExpression();
    
    while (tstream.GetType() == TOK_nullselect)
    {        
        Expr* lhs = expr;
        
        BufferNext();
        tstream.Advance();
        
        Expr* rhs = DoConditionalExpression();
        
        PIKA_NEWNODE(NullSelectExpr, expr, (state, lhs, rhs));
        expr->line = lhs->line; // !!! set line number
    }
    return expr;
}

Expr* Parser::DoConcatExpression()
{
    Expr* expr = DoLogOrExpression();
    
    while (tstream.GetType() == TOK_cat || tstream.GetType() == TOK_catspace)
    {        
        Expr::Kind k = (tstream.GetType() == TOK_catspace) 
                            ? Expr::EXPR_catsp 
                            : Expr::EXPR_cat;
        Expr* lhs = expr;
        
        BufferNext();   
        tstream.Advance();
        
        Expr* rhs = DoLogOrExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (state, k, lhs, rhs));
        expr->line = lhs->line; // !!! set line number
    }
    return expr;
}

// lhs or rhs
Expr* Parser::DoLogOrExpression()
{
    Expr* expr = DoLogXorExpression();
    
    while (tstream.GetType() == TOK_or || tstream.GetType() == TOK_pipepipe)
    {
        Expr::Kind k = Expr::EXPR_or;
        Expr* lhs = expr;
        
        BufferNext();
        tstream.Advance();
        
        Expr* rhs = DoLogXorExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (state, k, lhs, rhs));
        expr->line = lhs->line; // !!! set line number
    }
    return expr;
}

// lhs xor rhs
Expr* Parser::DoLogXorExpression()
{
    Expr* expr = DoLogAndExpression();
    
    while (tstream.GetType() == TOK_xor || tstream.GetType() == TOK_caretcaret)
    {
        Expr::Kind k = Expr::EXPR_xor;
        Expr* lhs = expr;
        
        BufferNext();
        tstream.Advance();
        
        Expr* rhs = DoLogAndExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (state, k, lhs, rhs));
        expr->line = lhs->line; // !!! set line number
    }
    return expr;
}

// lhs and rhs
Expr* Parser::DoLogAndExpression()
{
    Expr* expr = DoOrExpression();
    
    while (tstream.GetType() == TOK_and || tstream.GetType() == TOK_ampamp)
    {
        Expr::Kind k = Expr::EXPR_and;
        Expr* lhs = expr;

        BufferNext();
        tstream.Advance();
        
        Expr* rhs = DoOrExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (state, k, lhs, rhs));
        expr->line = lhs->line; // !!! set line number
    }
    return expr;
}

// lhs | rhs
Expr* Parser::DoOrExpression()
{
    Expr* expr = DoXorExpression();
    
    while (tstream.GetType() == '|')
    {
        Expr::Kind k = Expr::EXPR_bitor;
        Expr* lhs = expr;
        
        BufferNext();
        tstream.Advance();
        Expr* rhs = DoXorExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (state, k, lhs, rhs));
        expr->line = lhs->line; // !!! set line number
    }
    return expr;
}

// lhs ^ rhs
Expr* Parser::DoXorExpression()
{
    Expr* expr = DoAndExpression();
    
    while (tstream.GetType() == '^')
    {
        Expr::Kind k = Expr::EXPR_bitxor;
        Expr* lhs = expr;

        BufferNext();
        tstream.Advance();
        
        Expr* rhs = DoAndExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (state, k, lhs, rhs));
        expr->line = lhs->line; // !!! set line number
    }
    return expr;
}

// lhs & rhs
Expr* Parser::DoAndExpression()
{
    Expr* expr = DoEqualExpression();
    
    while (tstream.GetType() == '&')
    {
        Expr::Kind k = Expr::EXPR_bitand;        
        Expr* lhs = expr;
        
        BufferNext();
        tstream.Advance();
        
        Expr* rhs = DoEqualExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (state, k, lhs, rhs));
        expr->line = lhs->line; // !!! set line number
    }
    return expr;
}

// lhs == rhs
// lhs != rhs
// lhs === rhs
// lhs !== rhs
// lhs is rhs
Expr* Parser::DoEqualExpression()
{
    Expr* expr = DoCompExpression();
    
    while (tstream.GetType() == TOK_eq      ||
           tstream.GetType() == TOK_ne      ||
           tstream.GetType() == TOK_same    ||
           tstream.GetType() == TOK_notsame ||
           tstream.GetType() == TOK_is      ||
           tstream.GetType() == TOK_has     )
    {
        Expr::Kind k;        
        if (tstream.GetType() == TOK_eq)
        {
            k = Expr::EXPR_eq;
        }
        else if (tstream.GetType() == TOK_ne)
        {
            k = Expr::EXPR_ne;
        }
        else if (tstream.GetType() == TOK_same)
        {
            k = Expr::EXPR_same;
        }
        else if (tstream.GetType() == TOK_notsame)
        {
            k = Expr::EXPR_notsame;
        }
        else if (tstream.GetType() == TOK_is)
        {
                k = Expr::EXPR_is;
        }
        else // TOK_has
        {
            k = Expr::EXPR_has;
        }
        
        Expr* lhs = expr;
        
        BufferNext();
        tstream.Advance();
        /*
        if (k == Expr::EXPR_is && tstream.GetType() == TOK_not)
        {
            k = Expr::EXPR_isnot;
            
            BufferNext();            
            tstream.Advance();
        }
        */
        Expr* rhs = DoCompExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (state, k, lhs, rhs));
        expr->line = lhs->line; // !!! set line number
    }
    return expr;
}

// lhs < rhs
// lhs > rhs
// lhs <= rhs
// lhs >= rhs
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
            k = Expr::EXPR_lt;
        }
        else if (tstream.GetType() == '>')
        {
            k = Expr::EXPR_gt;
        }
        else if (tstream.GetType() == TOK_lte)
        {
            k = Expr::EXPR_lte;
        }
        else
        {
            k = Expr::EXPR_gte;
        }
        
        Expr* lhs = expr;

        BufferNext();        
        tstream.Advance();
        
        Expr* rhs = DoShiftExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (state, k, lhs, rhs));
        expr->line = lhs->line; // !!! set line number
    }
    return expr;
}

// lhs << rhs
// lhs >> rhs
// lhs >>> rhs
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
        
        BufferNext();
        tstream.Advance();
        
        Expr* rhs = DoAddExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (state, k, lhs, rhs));
        expr->line = lhs->line;
    }
    return expr;
}

// lhs + rhs
// lhs - rhs
Expr* Parser::DoAddExpression()
{
    Expr* expr = DoMulExpression();
    
    while (tstream.GetType() == '+' || tstream.GetType() == '-')
    {
        Expr::Kind k = (tstream.GetType() == '+') 
                ? Expr::EXPR_add 
                : Expr::EXPR_sub;
        
        Expr* lhs = expr;
        
        BufferNext();
        tstream.Advance();
        
        Expr* rhs = DoMulExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (state, k, lhs, rhs));
        expr->line = lhs->line;
    }
    return expr;
}

// lhs * rhs
// lhs // rhs
// lhs / rhs
// lhs % rhs
// lhs mod rhs
Expr* Parser::DoMulExpression()
{
    Expr* expr = DoRedirectExpression();
    
    while (tstream.GetType() == '*' || tstream.GetType() == '/' || tstream.GetType() == TOK_div ||
           tstream.GetType() == TOK_mod || tstream.GetType() == '%')
    {        
        Expr::Kind k;        
        if (tstream.GetType() == '*')
        {
            k = Expr::EXPR_mul;
        }
        else if (tstream.GetType() == '/')
        {
            k = Expr::EXPR_div;
        }
        else if (tstream.GetType() == TOK_div)
        {
            k = Expr::EXPR_intdiv;
        }
        else
        {
            k = Expr::EXPR_mod;
        }
        
        Expr* lhs = expr;
        
        BufferNext();        
        tstream.Advance();
        
        Expr* rhs = DoRedirectExpression();
        
        PIKA_NEWNODE(BinaryExpr, expr, (state, k, lhs, rhs));
        expr->line = lhs->line; // !!! set line number
    }
    return expr;
}

Expr* Parser::DoRedirectExpression()
{
    Expr* expr = DoPrefixExpression();
    
    while (tstream.GetType() == TOK_implies) {
        BufferNext();
        tstream.Advance();
        
        Expr* left = expr;
        Expr* right = DoPrefixExpression();
        if (right->kind == Expr::EXPR_paren)
            right = ((ParenExpr*)right)->Unwrap();
        if (right->kind != Expr::EXPR_call)
        {
            state->SyntaxException(Exception::ERROR_syntax, right->line, "redirect self must be applied to a function call.");
        }
        else
        {
            PIKA_NEWNODE(HijackExpr, expr, (state, left, (CallExpr*)right));
            expr->line = left->line;
        }
    }
    return expr;
}

static Expr::Kind GetPrefixType(int t)
{
    switch (t)
    {
    case '~':           return Expr::EXPR_bitnot;
    case '!':
    case TOK_not:       return Expr::EXPR_lognot;
    case TOK_increment: return Expr::EXPR_preincr;
    case TOK_decrement: return Expr::EXPR_predecr;
    case '+':           return Expr::EXPR_positive;
    case '-':           return Expr::EXPR_negative;
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
    {
        // ~ ! + - not ++ --
        
        int line = tstream.GetLineNumber();
        Expr::Kind k = GetPrefixType(lextok);
        
        BufferNext();
        tstream.Advance();
        Expr* operand = DoPrefixExpression();
        Expr* expr = 0;
        
        PIKA_NEWNODE(UnaryExpr, expr, (state, k, operand));
        expr->line = line;
        
        return expr;
    }
    break;
    case TOK_bind:
    {
        int line = tstream.GetLineNumber();

        BufferNext();
        tstream.Advance();
        
        Expr* operand = DoPrefixExpression();
        Expr* expr = 0;
        DotExpr* dotexpr = 0;
        
        if (operand->kind != Expr::EXPR_dot)
        {
            state->SyntaxException(Exception::ERROR_syntax, tstream.GetLineNumber(), "bind must be followed by a dot expression.");
        }
        
        dotexpr = (DotExpr*)operand;
        Expr* lhs = dotexpr->left;
        Expr* rhs = dotexpr->right;
        PIKA_NEWNODE(DotBindExpr, expr, (state, lhs, rhs, dotexpr->GetOpcode() == OP_subget));
        expr->line = line;
        return expr;
    }
    break;
    }
    return DoPostfixExpression();
}

bool Parser::IsPrimaryExpression()
{
    switch (tstream.GetType())
    {
    case '{': // object literal
    case '~': // complement
    case '!': // not
    case '\\':// lambda expression
//  case '[':   // We have a problem between array index [] and array literals [] with paren free calls
    case TOK_function:
    case TOK_identifier:
    case TOK_stringliteral:
    case TOK_string_r:    
    case TOK_integerliteral:
    case TOK_realliteral:
    case TOK_locals:
    case TOK_self:
    case TOK_super:
    case TOK_null:
    case TOK_true:
    case TOK_false:
        return tstream.GetPreviousLineNumber() == tstream.GetLineNumber();
    }
    return false;
}

Expr* Parser::DoPostfixExpression()
{
    Expr* expr = DoPrimaryExpression();
    
    for (;;)
    {
        if (IsPrimaryExpression() && (tstream.GetPrevType() == TOK_identifier 
            || tstream.GetPrevType() == TOK_super|| tstream.GetPrevType() == TOK_self))
        {
            /*
            --------------------------------------------------------------------------------------------
            
            Call expression WITHOUT parenthesis.
            
            The first argument must be super, self or an identifier and lie on the same line as the function
            being called.
            
            Each subsequent argument is comma seperated and can be any valid expression.
            
            FIXED:  5'hello' attempts to call the literal 5 with the argument 'hello'. This is now
            a compile time error because the previous token MUST be an identifier.            
            
            --------------------------------------------------------------------------------------------           
            */
            Expr*     lhs  = expr;
            ExprList* args = DoExpressionList(true, true);
            
            PIKA_NEWNODE(CallExpr, expr, (state, lhs, args));
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
        
            if (line > tstream.GetPreviousLineNumber())
                return expr;
            
            Expr* lhs = expr;
            tstream.Advance();
            PIKA_NEWNODE(UnaryExpr, expr, (state, Expr::EXPR_postincr, lhs));
            expr->line = lhs->line;
            return expr;
        }
        //
        // Postfix decrement.
        //        
        case TOK_decrement:
        {
            int line = tstream.GetLineNumber();
            
            if (line > tstream.GetPreviousLineNumber())
                return expr;
            
            Expr* lhs = expr;
                        
            tstream.Advance();
            PIKA_NEWNODE(UnaryExpr, expr, (state, Expr::EXPR_postdecr, lhs));
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
            if (tstream.GetLineNumber() != tstream.GetPreviousLineNumber())
                return expr;

            BufferNext();
            Match('(');
            const int call_terms[] = { ')', EOI, 0 };
            ExprList* callargs = DoOptionalExpressionList(call_terms, true, true);
            int line = tstream.GetLineNumber();
            
            Match(')');
            
            PIKA_NEWNODE(CallExpr, expr, (state, lhs, callargs));
            expr->line = line;
        }
        continue;
        //
        // Subcript operator or slice operator.
        //
        case '[':
        {
            Expr* lhs = expr;
            
            BufferNext();
            Match('[');
            
            Expr* rhs = DoExpression();

            BufferCurrent();
            if (tstream.GetType() == ']')
            {
                int line = tstream.GetLineNumber();
                
                Match(']');
                
                PIKA_NEWNODE(IndexExpr, expr, (state, lhs, rhs));
                expr->line = line;
            }
            else
            {
                Match(TOK_to);
                BufferCurrent();
                Expr* fromexpr = rhs;
                Expr* toexpr = DoExpression();
                int line = tstream.GetLineNumber();
                
                Match(']');
                
                PIKA_NEWNODE(SliceExpr, expr, (state, lhs, fromexpr, toexpr));
                expr->line = line;
            }
        }
        continue;
        //
        // Dot operator.
        //
        case '.':
        {
            BufferNext();
            tstream.Advance();
            Expr* lhs = expr;
            Expr* rhs = 0;
            Id* id = DoIdentifier();
            
            PIKA_NEWNODE(MemberExpr, rhs, (state, id));
            PIKA_NEWNODE(DotExpr,    expr, (state, lhs, rhs));
            
            expr->line = rhs->line = id->line;
        }
        continue;
        //
        // expr=>call(args)
        //
//        case TOK_implies:
//        {
//            BufferNext();
//            tstream.Advance();
//            
//            Expr* left = expr;
//            Expr* right = DoPrefixExpression();
//            if (right->kind == Expr::EXPR_paren)
//                right = ((ParenExpr*)right)->Unwrap();
//            if (right->kind != Expr::EXPR_call)
//            {
//                state->SyntaxException(Exception::ERROR_syntax, right->line, "redirect self must be applied to a function call.");
//            }
//            else
//            {
//                PIKA_NEWNODE(HijackExpr, expr, (state, left, (CallExpr*)right));
//                expr->line = left->line;
//            }
//        }
//        continue;
        //
        // expr ** expr
        //
        case TOK_starstar:
        {
            BufferNext();
            tstream.Advance();
            
            Expr* lhs = expr;
            Expr* rhs = DoPrefixExpression();
            
            PIKA_NEWNODE(BinaryExpr, expr, (state, Expr::EXPR_pow, lhs, rhs));
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
    Expr* expr = 0;
    int lextok = tstream.GetType();
    int line = tstream.GetLineNumber();
    
    switch (lextok)
    {
    case TOK_function: expr = DoFunctionExpression();   break;
    case '[':          expr = DoArrayExpression();      break;
    case '{':          expr = DoDictionaryExpression(); break;
    case '(':
    {
        BufferNext();
        Match('(');
        if (!Optional(')')) {
            expr = DoExpression();
            Expr* pexpr=0;
            PIKA_NEWNODE(ParenExpr, pexpr, (state, expr));
            pexpr->line =line;
            expr = pexpr;
            Match(')');
        } else {
            Expr* emptyExpr = 0;
            PIKA_NEWNODE(EmptyExpr, emptyExpr, (state));
            expr = emptyExpr;
        }
    }
    break;
    case '\\':
    {
        // Lambda Expression
        // \(arg1, ... ,argN)-> <expression>
        // \-> <expression>
        
        int line = tstream.GetLineNumber();
        Match('\\');
        BufferCurrent();
        ParamDecl* params = 0;
        
        if (tstream.GetType() != TOK_implies)
            params = DoFunctionParameters(false);
        Match(TOK_implies);
        BufferCurrent();
        
        size_t beg = tstream.curr.begOffset;
        Expr* expr = DoExpression();
        size_t end = tstream.curr.endOffset;

        ExprList* elist = 0;
        PIKA_NEWNODE(ExprList, elist, (state, expr));
        elist->line = expr->line;

        ExprStmt* body = 0;
        PIKA_NEWNODE(ExprStmt, body, (state, elist));
        body->line = expr->line;
        
        Expr* fun;
        PIKA_NEWNODE(FunExpr, fun, (state, params, body, beg, end));
        fun->line = line;
        return fun;
    }
    break;
    
    case TOK_identifier:     expr = DoIdExpression();             break;
    case TOK_stringliteral:  expr = DoStringLiteralExpression();  break;
    case TOK_integerliteral: expr = DoIntegerLiteralExpression(); break;
    case TOK_realliteral:    expr = DoRealLiteralExpression();    break;
    case TOK_string_r:       expr = DoStringLiteralExpression();  break;
    case TOK_locals:
    {
        tstream.Advance();
        PIKA_NEWNODE(LoadExpr, expr, (state, LoadExpr::LK_locals));
        expr->line = line;
    }
    break;
    case TOK_self:
    {
        expr = DoSelfExpression();
    }
    break;
    /*
     * super
     */
    case TOK_super:
    {
        Match(TOK_super);        
        PIKA_NEWNODE(LoadExpr, expr, (state, LoadExpr::LK_super));
        expr->line = line;
    }
    break;
    case TOK_null:
    {
        tstream.Advance();
        PIKA_NEWNODE(LoadExpr, expr, (state, LoadExpr::LK_null));
        expr->line = line;
    }
    break;
    case TOK_true:
    {
        tstream.Advance();
        PIKA_NEWNODE(LoadExpr, expr, (state, LoadExpr::LK_true));
        expr->line = line;
    }
    break;
    case TOK_false:
    {
        tstream.Advance();
        PIKA_NEWNODE(LoadExpr, expr, (state, LoadExpr::LK_false));
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
        
        PIKA_NEWNODE(Id, id, (state, value));
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
    int line = tstream.GetLineNumber();
    Id* id = DoIdentifier();
    
    PIKA_NEWNODE(IdExpr, expr, (state, id));
    expr->line = line;
    
    return expr;
}

Expr* Parser::DoRealLiteralExpression()
{
    Expr* expr = 0;
    int line = tstream.GetLineNumber();
    preal_t value = tstream.GetReal();
    
    Match(TOK_realliteral);
    
    PIKA_NEWNODE(RealExpr, expr, (state, value));
    expr->line = line;
    
    return expr;
}

Expr* Parser::DoStringLiteralExpression()
{
    Expr* expr = 0;
    int line  = tstream.GetLineNumber();
    char* value = tstream.GetString();
    size_t len = tstream.GetStringLength();
    if (Optional(TOK_string_r))
    {
        bool need_more = true;
        // Get the first part of the string-constructor
        PIKA_NEWNODE(StringExpr, expr, (state, value, len));
        expr->line = line;
        
        // Start cat'ing the string parts together.
        while (need_more)
        {
            // Grab the expression in-between the braces. And concat it to what we already have.
            Expr* rhs = 0;
            if (tstream.GetType() == TOK_self && 
                tstream.GetNextType() != '.'  &&
                tstream.GetNextType() != '[')
            {
                rhs = DoSelfExpression();
            }
            else
            {
                rhs = DoNameNode(true);
            }
            PIKA_NEWNODE(BinaryExpr, expr, (state, BinaryExpr::EXPR_cat, expr, rhs));
            rhs->line = line;
            expr->line = line;
            
            value = tstream.GetString();
            len = tstream.GetStringLength();
            
            if (Optional(TOK_string_b))
            {
                // Grab another segment of the string...
                PIKA_NEWNODE(StringExpr, rhs, (state, value, len));
            }
            else
            {
                // Grab the last part of the string...
                Match(TOK_string_l);
                PIKA_NEWNODE(StringExpr, rhs, (state, value, len));
                need_more = false;
            }
            // cat the previous parts to the new part.
            PIKA_NEWNODE(BinaryExpr, expr, (state, BinaryExpr::EXPR_cat, expr, rhs));
            expr->line = line;
            rhs->line = line;            
        }
    }
    else
    {
        Match(TOK_stringliteral);
        PIKA_NEWNODE(StringExpr, expr, (state, value, len));
        expr->line = line;
    }
    return expr;
}

Expr* Parser::DoIntegerLiteralExpression()
{
    Expr* expr = 0;
    int line = tstream.GetLineNumber();
    pint_t value = tstream.GetInteger();
    
    Match(TOK_integerliteral);
    
    PIKA_NEWNODE(IntegerExpr, expr, (state, value));
    expr->line = line;
    
    return expr;
}

ExprList* Parser::DoExpressionList(bool is_call, bool can_apply)
{
    ExprList* el = 0;
    ExprList* firstel = 0;
    bool has_kwarg = false;
    bool has_apply_va = false;
    bool has_apply_kw = false;
    
    do
    {
        Expr* expr = 0;
        switch (tstream.GetType()) {
        case ':': {
            if (has_apply_kw)
                state->SyntaxException(Exception::ERROR_syntax, tstream.GetLineNumber(), tstream.GetCol(), "applied variable argument, :vararg ,must come before an applied keyword argument, ::kwarg.");
            int line = tstream.GetLineNumber();
            tstream.Advance();
            BufferCurrent();
            Expr* cexpr = DoExpression();
            has_apply_va = true;
            PIKA_NEWNODE(ApplyArg, expr, (state, cexpr, false));
            expr->line = line;
            break;        
        }
        case TOK_coloncolon: {
            int line = tstream.GetLineNumber();
            tstream.Advance();
            BufferCurrent();
            Expr* cexpr = DoExpression();
            has_apply_kw = true;
            PIKA_NEWNODE(ApplyArg, expr, (state, cexpr, true));
            expr->line = line;
            break;
        }
        case TOK_identifier: {
            if (tstream.GetNextType() == ':') {
                if (has_apply_kw || has_apply_va) {
                    state->SyntaxException(Exception::ERROR_syntax, tstream.GetLineNumber(), tstream.GetCol(), "keyword arguments must come before any applied arguments.");
                }
                
                int line = tstream.GetLineNumber();
                StringExpr* sexpr = DoFieldName();
                BufferNext();
                Match(':');            
                Expr* valexpr = DoExpression();
            
                PIKA_NEWNODE(KeywordExpr, expr, (state, sexpr, valexpr));
                expr->line = line;
            
                if (!has_kwarg)
                    has_kwarg = true;                
                break;
            }
            // Fall through if next token is not ':' ...
            // |
            // |
            // V        
        }
        default:
            if (has_kwarg || has_apply_va || has_apply_kw)
                state->SyntaxException(Exception::ERROR_syntax, tstream.GetLineNumber(), tstream.GetCol(), "regular arguments must proceed keyword arguments and applied arguments.");
            expr = DoExpression();
        }
                
        ExprList* nxtel = 0;
        
        PIKA_NEWNODE(ExprList, nxtel, (state, expr));
        nxtel->line = expr->line;
        
        if (!firstel)
            firstel = nxtel;
            
        if (el)
            el->next = nxtel;
            
        el = nxtel;
        
        if (Optional(','))
        {
            BufferCurrent();
        }
        else
        {
            break;
        }
    }
    while (!tstream.IsEndOfStream());
    
    return firstel;
}

ExprList* Parser::DoOptionalExpressionList(const int* terms, bool is_call, bool can_apply)
{
    ExprList* el = 0;
    ExprList* firstel = 0;
    bool has_kwarg = false;
    bool has_apply_va = false;
    bool has_apply_kw = false;
    
    while (!tstream.IsEndOfStream() && IsNotTerm(terms, tstream.GetType()) )
    {
        BufferNext();
        Expr* expr = 0;

        switch (tstream.GetType()) {
        case ':': {
            if (has_apply_kw)
                state->SyntaxException(Exception::ERROR_syntax, tstream.GetLineNumber(), tstream.GetCol(), "applied variable argument, :vararg ,must come before an applied keyword argument, ::kwarg.");
            int line = tstream.GetLineNumber();
            tstream.Advance();
            BufferCurrent();
            Expr* cexpr = DoExpression();
            has_apply_va = true;
            PIKA_NEWNODE(ApplyArg, expr, (state, cexpr, false));
            expr->line = line;
            break;        
        }
        case TOK_coloncolon: {
            int line = tstream.GetLineNumber();
            tstream.Advance();
            BufferCurrent();
            Expr* cexpr = DoExpression();
            has_apply_kw = true;
            PIKA_NEWNODE(ApplyArg, expr, (state, cexpr, true));
            expr->line = line;
            break;
        }
        case TOK_identifier: {
            if (tstream.GetNextType() == ':') {
                if (has_apply_kw || has_apply_va) {
                    state->SyntaxException(Exception::ERROR_syntax, tstream.GetLineNumber(), tstream.GetCol(), "keyword arguments must come before any applied arguments.");
                }
                
                int line = tstream.GetLineNumber();
                StringExpr* sexpr = DoFieldName();
                BufferNext();
                Match(':');            
                Expr* valexpr = DoExpression();
            
                PIKA_NEWNODE(KeywordExpr, expr, (state, sexpr, valexpr));
                expr->line = line;
            
                if (!has_kwarg)
                    has_kwarg = true;                
                break;
            }
            // Fall through if next token is not ':' ...
            // |
            // |
            // V        
        }
        default:
            if (has_kwarg || has_apply_va || has_apply_kw)
                state->SyntaxException(Exception::ERROR_syntax, tstream.GetLineNumber(), tstream.GetCol(), "regular arguments must proceed keyword arguments and applied arguments.");
            expr = DoExpression();
        }
        
        ExprList* nxtel = 0;
        
        PIKA_NEWNODE(ExprList, nxtel, (state, expr));
        
        if (!firstel)
            firstel = nxtel;
        
        if (el)
            el->next = nxtel;
        
        el = nxtel;
        
        if (tstream.GetType() != ',' && IsNotTerm(terms, tstream.GetType()) )
        {
            if (IsTerm(terms, EOF))
                return firstel;
            else {
                Expected(terms[0]);
            }
        }
        
        if (tstream.GetType() == ',')
        {
            Match(',');
            BufferCurrent();
            if (IsTerm(terms, tstream.GetType()))
                Unexpected(terms[0]);
        }
    }    
    return firstel;
}

Expr* Parser::DoFunctionExpression()
{
    ParamDecl* params = 0;
    Id* id = 0;
    int line = tstream.GetLineNumber();
    size_t beg = tstream.curr.begOffset;
    
    BufferNext(); // need '(' as next token    
    Match(TOK_function);

    BufferNext();
    if (tstream.GetType() == TOK_identifier)
    {
        id = DoIdentifier();
        BufferNext();
    }
    params = DoFunctionParameters();
    
    Stmt* body = DoFunctionBody();
    size_t end = tstream.prev.endOffset;
    
    Expr* expr = 0;    
    PIKA_NEWNODE(FunExpr, expr, (state, params, body, beg, end, id));
    expr->line = line;
    
    return expr;
}

Expr* Parser::DoArrayExpression()
{
    Expr* expr = 0;

    BufferNext();
    Match('[');
    
    const int arr_terms[] = { ']', EOI, 0 };
    ExprList* elems = DoOptionalExpressionList(arr_terms, false);
    
    int line = tstream.GetLineNumber();
    
    Match(']');
    
    PIKA_NEWNODE(ArrayExpr, expr, (state, elems));
    expr->line = line;
    
    return expr;
}

Expr* Parser::DoDictionaryExpression()
{
    Expr* expr = 0;
    
    BufferNext();
    Match('{');
    
    FieldList* fields = DoDictionaryExpressionFields();
    
    int line = tstream.GetLineNumber();
    
    Match('}');
    
    PIKA_NEWNODE(DictionaryExpr, expr, (state, fields));
    expr->line = line;
    
    return expr;
}

FieldList* Parser::DoDictionaryExpressionFields()
{
    FieldList* fields = 0;

    BufferCurrent();
    while (tstream.GetType() != '}' && !tstream.IsEndOfStream())
    {
        Expr* name = 0;
        Expr* value = 0;
        
        name = (tstream.GetType() == TOK_identifier) ?
                DoFieldName() :
                DoStringLiteralExpression();
               
        Match(':');
        BufferCurrent();
        value = DoExpression();
        BufferCurrent();
                
        BufferCurrent();
        if (!DoCommaOrNewline())
        {
            if (tstream.GetType() != '}')
            {
                Expected('}');
            }
        }
        else
        {
            BufferCurrent();
        }
        
        FieldList* nxt = 0;
        PIKA_NEWNODE(FieldList, nxt, (state, name, value));
        
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

ParamDecl* Parser::DoFunctionParameters(bool close)
{
    ParamDecl* params = 0;
    ParamDecl* currParam = 0;
    bool def_vals = false;

    if (close)
    {        
        Match('(');
        BufferCurrent();
    }
    else
    {
        if ((close = Optional('(')))
            BufferCurrent();
    }
    
    while (tstream.GetType() != ')' && !tstream.IsEndOfStream())
    {
        bool rest_param = false;
        bool kw_param = false;
        
        Id* id = 0;
        Expr* def_expr = 0;
        
        if (tstream.GetType() == ':')
        {
            Match(':');
            BufferCurrent();
            rest_param = true;
        }
        else if (tstream.GetType() == TOK_coloncolon)
        {
            Match(TOK_coloncolon);
            BufferCurrent();
            kw_param = true;
        }
        id = DoIdentifier();
        BufferCurrent();
        
        if (Optional('='))
        {
            if (rest_param)
            {
                state->SyntaxError(tstream.GetLineNumber(), "rest parameter declaration (...) cannot have a default value");
            }
            def_vals = true;
            
            BufferCurrent();
            def_expr = DoExpression();
            BufferCurrent();
        }
        
        if (def_vals && !def_expr && !rest_param && !kw_param)
        {
            state->SyntaxError(tstream.GetLineNumber(), "Expected default value for parameter '%s'.", id->name);
        }
        
        PIKA_NEWNODE(ParamDecl, currParam, (state, id, rest_param, kw_param, def_expr));
        currParam->line = id->line;
        
        if (!params)
        {
            params = currParam;
        }
        else
        {
            params->Attach(currParam);
        }
        
        if (tstream.GetType() != ')')
        {
            if ((kw_param || rest_param) && tstream.GetType() == ',')
            {
                if (kw_param) {
                    state->SyntaxError(tstream.GetLineNumber(), "keyword argument declaration '::' must be the last parameter.");
                } else {
                    BufferNext();
                    if (tstream.GetNextType() != TOK_coloncolon) {
                        state->SyntaxError(tstream.GetLineNumber(), "variable argument declaration ':' can only be followed by keyword argument declaration '::'.");
                    } else {
                        Match(',');
                        BufferCurrent();
                    }
                }
            }
            else if (tstream.GetType() != ',')
            {
                if (close) Expected(')');
                else       break;
            }
            else
            {
                Match(',');
                BufferCurrent();
            }

        }
    }
    
    if (close)
    {
        Match(')');
    }
    
    return params;
}

}// pika
