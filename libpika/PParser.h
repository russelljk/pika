/*
 *  PParser.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_PARSER_HEADER
#define PIKA_PARSER_HEADER

#include "PTokenDef.h"
#include "PTokenizer.h"
#include "PAst.h"
namespace pika
{
struct Program;
struct Expr;
struct ExprList;
struct Stmt;
struct Id;
struct IdExpr;
struct StringExpr;
struct NameNode;
struct Decl;
struct ParamDecl;
struct CaseList;
struct CompileState;
struct FieldList;
struct LocalDecl;
class LiteralPool;

// Token ///////////////////////////////////////////////////////////////////////////////////////////

struct Token
{
    YYSTYPE value;     //!< Union of values associated with the token.
    int     tokenType; //!< The type of the token.
    int     line;      //!< The line located at.
    int     col;       //!< The column located at.
    size_t  begOffset; //!< Offset to the start of the token in the source code.
    size_t  endOffset; //!< Offset to the end of the token in the source code.
};

/** A stream of tokens from the tokenizer. */
struct TokenStream
{
    TokenStream(CompileState* state, FILE* yyin);
    TokenStream(CompileState* state, const char* buffer, size_t len);
    
    ~TokenStream();
    
    void Initialize();
    void Advance();
    
    INLINE int GetType()     const { return curr.tokenType; }
    INLINE int GetNextType() const { return next.tokenType; }
    INLINE int GetPrevType() const { return prev.tokenType; }
    
    INLINE bool IsEndOfStream() const { return curr.tokenType == EOF; }
    
    INLINE pint_t  GetInteger()      const { return curr.value.integer; }
    INLINE preal_t GetReal()         const { return curr.value.real; }
    INLINE char*   GetString()       const { return curr.value.str; }
    INLINE size_t  GetStringLength() const { return curr.value.len; }

    INLINE int GetLineNumber() const { return curr.line; }
    INLINE int GetNextLineNumber()     const { return(next.tokenType == EOF) ? (curr.tokenType == EOF) ? prev.line : curr.line : next.line; }
    INLINE int GetPreviousLineNumber() const { return prev.line; }
    
    INLINE int GetCol() const { return curr.col; }
    INLINE int GetPrevCol() const { return prev.col; }
    INLINE int GetNextCol() const { return(next.tokenType == EOF) ? (curr.tokenType == EOF) ? prev.col : curr.col : next.col; }
        
    CompileState* state;
    Tokenizer*    tokenizer;
    FILE*         yyin;      //!< File handle for the script. Null if we are parsing a string.
    Token         prev;      //!< Previous token.
    Token         curr;      //!< Current token.
    Token         next;      //!< Next token.
};

// Parser //////////////////////////////////////////////////////////////////////////////////////////

struct ForHeader
{    
    Id*  id;    //!< Loop variable.
    int  line;  //!< Line the statement starts.
};

struct ForToHeader
{
    ForHeader head;   //!< For statement header.
    Expr*     from;   //!< Starting point.
    Expr*     to;     //!< Ending point.
    Expr*     step;   //!< Step increment.
    bool      isdown; //!< Are we steping down or up.
};

struct ForEachHeader
{
    ForHeader head;    //!< For statement header.    
    Expr*     of;      //!< Name of the set we are enumerating.
    Expr*     subject; //!< The object we are enumerating.
};

class Parser
{
public:
    Program*      root;     //!< Root node of the AST.
    CompileState* state;
    TokenStream   tstream;
    
    Parser(CompileState* cs, FILE* yyin);
    Parser(CompileState* cs, const char* buffer, size_t len);
    
    ~Parser();
    
    Program*        DoParse();
    Program*        DoFunctionParse();
private:
    void            DoScript();
    void            DoFunctionScript();
    
    // Statement parsing -------------------------------------------------------
    
    Stmt*           DoStatement(bool);
    Stmt*           DoPropertyStatement();

    Stmt*           DoFunctionBody();
    Stmt*           DoBlockStatement();

    StringExpr*     DoFieldName();

    Stmt*           DoTryStatement();
    Stmt*           DoLabeledStatement();

    Stmt*           DoVarStatement();
    Decl*           DoVarDeclaration();

    Stmt*           DoFunctionStatement();
    Stmt*           DoIfStatement();
    Stmt*           DoWhileStatement();
    Stmt*           DoUntilStatement();
    Stmt*           DoLoopStatement();

    Stmt*           DoForStatement();
    Stmt*           DoForToStatement(ForHeader*);
    Stmt*           DoForEachStatement(ForHeader*);

    void            DoForToHeader(ForToHeader*);
    void            DoForEachHeader(ForEachHeader*);
    
    Stmt*           DoReturnStatement();
    Stmt*           DoRaiseStatement();
    Stmt*           DoYieldStatement();
        
    Stmt*           DoBreakStatement();
    Stmt*           DoContinueStatement();
    Stmt*           DoOptionalJumpStatement(Stmt*);
    Stmt*           DoExpressionStatement();

    Stmt*           DoFinallyBlock(Stmt*);
    Stmt*           DoWithStatement();
    Stmt*           DoPackageDeclaration();
    Stmt*           DoCaseStatement();
    Stmt*           DoClassStatement();
    
    NameNode*       DoNameNode(bool);
    
    Stmt*           DoAssertStatement();
    
    void            DoBlockBegin(int, int, int);
    // Expression parsing ------------------------------------------------------
    
    Expr*           DoExpression();
    
    Expr*           DoNullSelectExpression();
    Expr*           DoConditionalExpression();
    Expr*           DoLogOrExpression();
    Expr*           DoLogXorExpression();
    Expr*           DoLogAndExpression();
    Expr*           DoOrExpression();
    Expr*           DoXorExpression();
    Expr*           DoAndExpression();
    Expr*           DoEqualExpression();
    Expr*           DoCompExpression();
    Expr*           DoShiftExpression();
    Expr*           DoConcatExpression();
    Expr*           DoAddExpression();
    Expr*           DoMulExpression();
    Expr*           DoPrefixExpression();
    Expr*           DoPostfixExpression();
    Expr*           DoPrimaryExpression();
    Expr*           DoNewExpression();
    
    Id*             DoIdentifier();
    IdExpr*         DoIdExpression();
    Expr*           DoRealLiteralExpression();
    Expr*           DoStringLiteralExpression();
    Expr*           DoIntegerLiteralExpression();
    ExprList*       DoExpressionList();
    
    Expr*           DoFunctionExpression();
    
    // These methods take null terminated arrays of terminator characters //////////////////////////
    
    ExprList*       DoOptionalExpressionList(const int* terms, bool optcomma = false);
    Stmt*           DoStatementList(const int* terms);
    Stmt*           DoStatementListBlock(const int* terms);
    CaseList*       DoCaseList(const int* terms);
    
    ////////////////////////////////////////////////////////////////////////////////////////////////
    
    Expr*           DoArrayExpression();
    Expr*           DoArrayComp(Expr*);
    
    Expr*           DoDictionaryExpression();
    FieldList*      DoDictionaryExpressionFields();
    
    bool            IsPrimaryExpression();
    ParamDecl*      DoFunctionParameters();
    
    void            Match(int x);
    bool            Optional(int x);
    void            Expected(int x);
    void            Unexpected(int x);
    
    void            DoEndOfStatement();
    bool            DoCommaOrNewline();
    bool            IsEndOfStatement();
    
    bool            DoContextualKeyword(const char* key, bool optional = false);
    StorageKind     DoStorageKind();
    
    static const int Static_end_terms[];
    static const int Static_finally_terms[];
};

}// pika

#endif