/*
 *  PParser.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_PARSER_HEADER
#define PIKA_PARSER_HEADER

#include "PTokenDef.h"
#include "PTokenizer.h"
#include "PAst.h"
namespace pika {
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
    TokenStream(CompileState* state, std::ifstream* yyin);
    TokenStream(CompileState* state, const char* buffer, size_t len);
    TokenStream(CompileState* state, IScriptStream* stream);
    
    ~TokenStream();
    
    void Initialize();
    void Advance();
    void InitTokens();
    INLINE int GetType()     const { return curr.tokenType; }
    INLINE int GetNextType() const { return next.tokenType; }
    INLINE int GetPrevType() const { return prev.tokenType; }
    
    INLINE bool IsEndOfStream() const { return curr.tokenType == EOF || curr.tokenType == EOI; }
    INLINE bool More()  const { return curr.tokenType == EOI; }
    void            GetMore();
    void            GetNextMore();
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
        
    CompileState*  state;
    Tokenizer*     tokenizer;
    std::ifstream* yyin;      //!< File handle for the script. Null if we are parsing a string.
    Token          prev;      //!< Previous token.
    Token          curr;      //!< Current token.
    Token          next;      //!< Next token.
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
    
    Parser(CompileState* cs, std::ifstream* yyin);
    Parser(CompileState* cs, const char* buffer, size_t len);
    Parser(CompileState* cs, IScriptStream* stream);
    
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
    Stmt*           DoUsingStatement();
    Stmt*           DoPackageDeclaration();
    Stmt*           DoClassStatement();
    
    NameNode*       DoNameNode(bool);
    
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
    
    Id*             DoIdentifier();
    IdExpr*         DoIdExpression();
    Expr*           DoRealLiteralExpression();
    Expr*           DoStringLiteralExpression();
    Expr*           DoIntegerLiteralExpression();
    ExprList*       DoExpressionList(bool is_kwarg=false);
    LoadExpr*       DoSelfExpression();
    Expr*           DoFunctionExpression();
    
    // These methods take null terminated arrays of terminator characters //////////////////////////
    
    ExprList*       DoOptionalExpressionList(const int* terms, bool kwargs = false);
    Stmt*           DoStatementList(const int* terms);
    Stmt*           DoStatementListBlock(const int* terms);
    
    ////////////////////////////////////////////////////////////////////////////////////////////////
    
    Expr*           DoArrayExpression();
    Expr*           DoArrayComp(Expr*);
    
    Expr*           DoDictionaryExpression();
    FieldList*      DoDictionaryExpressionFields();
    
    bool            IsPrimaryExpression();
    ParamDecl*      DoFunctionParameters(bool close=true);
    
    // If another character needs to be present you must call this method. Otherwise the parse could fail in REPL mode.
    
    void            BufferCurrent(); // Need at least 1 token in the stream.
    void            BufferNext();    // Need at least 2 tokens in the stream.
    
    /** Checks for the desired token. If present the token stream is advanced. If EOI is encountered the input stream is buffered.
      * Otherwise an exception is thrown via Parser::Expected.
      */    
    void Match(int x);
        
    /** Checks for the desired token. If present the token stream is advanced
      * and true is returned. Otherwise false is returned and the token stream
      * remains untouched.
      */    
    bool Optional(int x);
    
    // Error reporting methods. Makes no attempt to Match.
    
    /** Throws an exception because expected token (x) is not encountered.
      * NOTE: the presence of the token is not actually tested use Parser::Match or
      * Parser::Optional to do that.
      */    
    void Expected(int x);
    
    /** Throws an exception because an un-expected token (x) is encountered. */    
    void Unexpected(int x);
    
    /** Checks for a valid end of statement token: end, ';', '\n', '}' or EOF.
      * This is basically where optional semicolons are implemented.
      *
      * The token stream is advanced iff the current token is ';'.
      */    
    void DoEndOfStatement();
    
    /** This method behaves much like DoEndOfStatement, but for commas.
      * Could be used so that function parameters,arguments or array elements are
      * seperated by either
      * newlines or commas
      * 
      * ie x = [ 1
      *          2
      *          3 ]
      *
      * instead of x = [ 1, 2, 3 ]
      */    
    bool DoCommaOrNewline();
    
    /** Determines if DoEndOfStatement will succeed. The token stream will not be
      * advanced.
      */    
    bool IsEndOfStatement();
    
    bool            DoContextualKeyword(const char* key, bool optional = false);
    StorageKind     DoStorageKind();
    
    static const int Static_end_terms[];
    static const int Static_finally_terms[];
};

}// pika

#endif
