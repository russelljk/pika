/*
 * PAst.h
 * See Copyright Notice in Pika.h
 */
#ifndef PIKA_AST_HEADER
#define PIKA_AST_HEADER

#include "POpcode.h"
#include "PLineInfo.h"
#include "PSymbolTable.h"

#ifndef PIKA_NEWNODE
#define PIKA_NEWNODE(T, p, args)         \
do {                                     \
    void* __v = Pika_malloc(sizeof(T));  \
    if (__v)                             \
    {                                    \
        p = new (__v) T args;            \
        state->nodes += p;               \
    }                                    \
    else                                 \
    {                                    \
        p = 0;                           \
    }                                    \
} while (false)
#endif //PIKA_NEWNODE

namespace pika {

class Parser;
class LiteralPool;
class SymbolTable;
class TreeNodeList;
struct CompileState;
struct Instr;
/////////////////////////////////////
// Abstract Syntax Tree hierarchy. //
/////////////////////////////////////
struct TreeNode;
struct      Program;
struct          FunctionProg;
struct      Id;
struct      Decl;
struct          DeclarationTarget;
struct              NamedTarget;
struct                  PropertyDecl;
struct                  PkgDecl;
struct                  ClassDecl;
struct              FunctionDecl;
struct              VariableTarget;
struct              NameNode;
struct          ParamDecl;
struct          VarDecl;
struct              LocalDecl;
struct              MemberDeclaration;
struct      CatchIsBlock;
struct      Stmt;
struct          FinallyStmt;
struct          RaiseStmt;
struct          TryStmt;
struct          LabeledStmt;
struct          LoopingStmt;
struct              LoopStmt;
struct              CondLoopStmt;
struct              ForToStmt;
struct              ForeachStmt;
struct          BreakStmt;
struct          ContinueStmt;
struct          StmtList;
struct          EmptyStmt;
struct          ExprStmt;
struct          RetStmt;
struct          YieldStmt;
struct          IfStmt;
struct          ConditionalStmt;
struct          BlockStmt;
struct          DeclStmt;
struct          AssignmentStmt;
struct          UsingStmt;
struct      Expr;
struct          LoadExpr;
struct          CallExpr;
struct          CondExpr;
struct          NullSelectExpr;
struct          SliceExpr;
struct          UnaryExpr;
struct          BinaryExpr;
struct              DotExpr;
struct                  DotBindExpr;
struct          IdExpr;
struct          PropExpr;
struct          ConstantExpr;
struct              FunExpr;
struct              MemberExpr;
struct              StringExpr;
struct              IntegerExpr;
struct              RealExpr;
struct          DictionaryExpr;
struct          ArrayExpr;
struct      FieldList;
struct      ExprList;



/** Programmer specified storage modifier. */
enum StorageKind
{
    STO_none,   //!< Use default storage of the scope.
    STO_local,  //!< Local variable.
    STO_global, //!< Global variable.
    STO_member, //!< Object member variable.
};

/** Linked list of TreeNodes */
class TreeNodeList
{
public:
    TreeNodeList()
            : first(0),
            last(&first) {}
            
    void operator+=(TreeNode* t);
    
    TreeNode* first;
    TreeNode** last;
};

/** Shared state for used by the Tokenizer, Parser and AST. Keeps track of locals, 
  * literals, AST nodes and error handling when parsing. 
  */
struct CompileState
{
    CompileState(Engine*);
    
    ~CompileState();
            
    int  GetLocalOffset() const;       //!< Gets the current local offset
    void SetLocalOffset(int off);      //!< Sets the current local offset
    int  NextLocalOffset(const char*, 
                         ELocalVarType lvt = LVT_variable); //!< Gets the current local offset and then moves to the next offset.
    
    /** Creates more than one local variable. Only the first of which has a Symbol. */
    Symbol* CreateLocalPlus(SymbolTable* st, const char* name, size_t extra);
    
    /** Add an integer literal.
      *
      * @param i The integer literal
      *
      * @result The index of the literal.
      *
      */
    u2 AddConstant(pint_t i);

    /** Add an real literal.
      *
      * @param i The real literal
      *
      * @result The index of the literal.
      */
    u2 AddConstant(preal_t i);

    /** Add a function Def literal.
      *
      * @param fun The function Def literal
      *
      * @result The index of the literal.
      */
    u2 AddConstant(Def* fun);

    /** Add a String literal.
      *
      * @param str The null terminated string literal.
      *
      * @result The index of the literal.
      */
    u2 AddConstant(const char* str);
    
    /** Add a String literal.
      *
      * @param str The string literal. May contain non-ascii characters, including nulls.
      * @param len The length of the string.
      *
      * @result The index of the literal.
      */    
    u2 AddConstant(const char* str, size_t len);
    
    /** Set the line of code being processed. Needed if the script later needs to
      * be debuged.
      */
    void SetLineInfo(u2 line);
    
    /** Update the line of code being processed. Needed if the script later needs to
      * be debuged.
      *
      * @param line The line in the script.
      *
      * @result Return true if the current line is updated successfully
      */    
    bool UpdateLineInfo(int line);
    
    // Error Reporting -----------------------------------------------------------------------------

#if defined(ENABLE_SYNTAX_WARNINGS)
    /** Issues a syntax warning. Prints the message to stderr. */    
    void SyntaxWarning(WarningLevel level, int line, const char* format, ...);
#endif
    
    /** Report a syntax error. Do not use if the error is fatal. Under most circumstances an exception is not raised. 
      * However if the parser is running in REPL mode an exception is thrown because the user can correct its mistake.
      * @note Do not use if the error is not recoverable unless you manually raise an exception.
      */
    void SyntaxError(int line, const char* format, ...);           
    
    /** Report a syntax error. An exception of kind 'k' will be raised. */    
    void SyntaxException(Exception::Kind k, int line, const char *msg, ...);
    void SyntaxException(Exception::Kind k, int line, int col, const char *msg, ...); 
    
    /** Prints a summary of errors and warnings. */
    void SyntaxErrorSummary();
    
    /** Returns true if syntax errors occurred while parsing and compiling a script. */
    bool HasErrors() const { return errors != 0; }
    
    void SetParser(Parser* p) { parser = p; }
    
    struct TryState
    {
        bool inTry;
        bool inCatch;               //!< Are we in a catch block.
        u2   catchVarOffset;        //!< Location of the caught exception (used to re-raise a caught exception).
    }               trystate;    
    TreeNodeList    nodes;          //!< All the nodes we know about.
    LiteralPool*    literals;       //!< Literals used in this program. Shared by all child functions.
    Table           literalLookup;
    Engine*         engine;
    Def*            currDef;        //!< Current function definition being compiled.
    int             localOffset;    //!< Next local variable offset.
    int             localCount;     //!< Total number of lexEnv.
    u2              currentLine;    //!< Current line being processed.
    int             errors;         //!< Number of errors found.
    int             warnings;       //!< Number of warnings found.
    Instr*          endOfBlock;     //!< Last Instr of the current block (used to track local variable scope.)
    Parser*         parser; 
    bool            repl_mode;      //!< Running in REPL mode. Errors should be treated as an exception.
};

/** Base class for all AST tree nodes. */
struct TreeNode
{
    TreeNode(CompileState* s) : state(s), line(0), astnext(0) {}
    
    virtual ~TreeNode() {}
    
    virtual void   CalculateResources(SymbolTable* st) {}
    virtual Instr* GenerateCode();
    
    CompileState* state;    //!< Our CompileState
    int           line;     //!< Line number in the source script.
    TreeNode*     astnext;  //!< Next node in the linked list.
};


/** Root node when parsing a script. */
struct Program : TreeNode
{
    Program(CompileState* s, Stmt *stmts, size_t beg, size_t end)
            : TreeNode(s),
            index(0),
            def(0),
            stmts(stmts),
            symtab(0),
            scriptBeg(beg),
            scriptEnd(end) {}
            
    virtual ~Program();
    
    virtual void   CalculateResources(SymbolTable* st);
    virtual Instr* GenerateCode();
    
    u2           index;     //!< Literal index of the __main function Def.
    Def*         def;       //!< Script's function body.
    Stmt*        stmts;     //!< Script's body as AST.
    SymbolTable* symtab;    //!< Symbol table.
    size_t       scriptBeg; //!< Beginning offset in source code.
    size_t       scriptEnd; //!< Ending offset in source code.
};

/** Root node when parsing a function. */
struct FunctionProg : Program
{
    FunctionProg(CompileState* s, Stmt* stmts, size_t beg, size_t end, ParamDecl* a)
            : Program(s, stmts, beg, end), args(a)
    {}
    
    virtual ~FunctionProg() {}
    
    virtual void CalculateResources(SymbolTable* st);
    
    ParamDecl* args; //!< Parameters including variable and keyword arguments.
};

/** An valid pika identifier. */
struct Id : TreeNode
{
    Id(CompileState* s, char* name);
    virtual ~Id();
    
    char* name;
    Id*   next;
};

/** An declaration node. */
struct Decl : TreeNode
{
    enum Kind
    {
        DECL_variable,
        DECL_function,
        DECL_parameter,
        DECL_property,
        DECL_package,
        DECL_class,
        DECL_annotation,
    };
    
    Decl(CompileState* s, Kind kind)
            : TreeNode(s),
            kind(kind)
    {}
    
    virtual ~Decl()
    {}
    
    Kind kind; //!< The Decl::Kind we are.
};

/** An declaration target with a storage kind and symbol. */
struct DeclarationTarget : Decl
{
    DeclarationTarget(CompileState* s, Decl::Kind k, StorageKind sk)
            : Decl(s, k),
            storage(sk),
            with(false),
            symbol(0),
            nameindex(0)
    {}
    
    virtual ~DeclarationTarget() {}
    
    virtual const char* GetIdentifierName() = 0;
    virtual void        CreateSymbol(SymbolTable* st);
    virtual Instr*      GenerateCodeSet();
    
    StorageKind storage;   //!< Type of storage for our Symbol.
    bool        with;      //!< Symbol should be a member.
    Symbol*     symbol;    //!< Our Symbol.
    u2          nameindex; //!< Literal index of our Symbol.
};

struct AnnonationDecl : Decl, TLinked<AnnonationDecl>
{
    AnnonationDecl(CompileState* s, NameNode* name, ExprList* args) : Decl(s, Decl::DECL_annotation), TLinked<AnnonationDecl>(),  name(name), args(args) {}
    virtual ~AnnonationDecl() {}
    
    virtual void   CalculateResources(SymbolTable* st);
    virtual Instr* GenerateCode();
    virtual Instr* GenerateCodeWith(Instr*);
    
    NameNode* name; //!< Identifier or name of the annotation to call.
    ExprList* args; //!< List of arguments, not including the last which comes from the declaration or a previous annotation.
};

/** A DeclarationTarget that has a ientifier, dot.expr or index[expr] name. */
struct NamedTarget : DeclarationTarget
{
    NamedTarget(CompileState* s, NameNode* n, Decl::Kind k, StorageKind sk) : DeclarationTarget(s, k, sk), annotations(0), already_decl(false), name(n) {}
    
    virtual void        CalculateResources(SymbolTable* st);
    virtual const char* GetIdentifierName();
    virtual Instr*      GenerateCodeSet();
    virtual Instr*      GenerateCodeWith(Instr* body);
    virtual Instr*      GenerateAnnotationCode(Instr* subj);
    
    AnnonationDecl* annotations;
    bool already_decl;
    NameNode* name;
};

/** A function declaration. */
struct FunctionDecl : NamedTarget
{
    FunctionDecl(CompileState* s, NameNode*, ParamDecl*, Stmt*, size_t, size_t, StorageKind);
    virtual ~FunctionDecl();
    
    virtual void        CalculateResources(SymbolTable* st);
    virtual Instr*      GenerateCode();
    virtual Instr*      GenerateFunctionCode();
    virtual const char* GetIdentifierName();
    
    Def*         def;       //!< The function definition.
    u2           index;     //!< The literal index of the function definition.
    ParamDecl*   args;      //!< Parameters including variable and keyword arguments.
    Stmt*        body;      //!< The function's body.
    SymbolTable* symtab;    //!< Symbols for the function's body.    
    bool         varArgs;   //!< Function has variable argument.
    bool         kwArgs;    //!< Function has keyword argument.
    size_t       scriptBeg; //!< Beginning position in the source code.
    size_t       scriptEnd; //!< End position in the source code.
};

/** Function parameter declaration node. */
struct ParamDecl : Decl
{
    ParamDecl(CompileState* s, Id* name, bool vargs, bool kw, Expr* val);
    
    void            CalculateDefaultResources(SymbolTable* st);
    virtual void    CalculateResources(SymbolTable* st);
    void            Attach(ParamDecl* nxt);
    bool            HasDefaultValue();
    bool            HasVarArgs();
    bool            HasKeywordArgs();
    
    Expr*       val;        //!< Default valid or null. ie (..., param = value, ...)
    Symbol*     symbol;     //!< Parameter's local variable Symbol.
    Id*         name;       //!< Parameter name.
    bool        has_rest;   //!< Parameter is variable argument.
    bool        has_kwargs; //!< Parameter is keyword argument.
    ParamDecl*  next;       //!< Next parameter.
};

/** Global variable declaration node. */
struct VarDecl : Decl
{
    VarDecl(CompileState* s, Id* name)
            : Decl(s, Decl::DECL_variable),
            nameIndex(0),
            symbol(0),
            name(name),
            next(0)
    {}
    
    virtual void    CalculateResources(SymbolTable* st);
    virtual Instr*  GenerateCode();
    
    u2       nameIndex; //!< Literal pool index of our name.
    Symbol*  symbol;    //!< Symbol of the variable.
    Id*      name;      //!< Identifier of the variable.
    VarDecl* next;      //!< Next VarDecl or null. 
};

/** Local variable declaration node. */
struct LocalDecl : VarDecl
{
    LocalDecl(CompileState* s, Id* name) : VarDecl(s, name), newLocal(false) {}
    
    virtual void CalculateResources(SymbolTable* st);
    virtual Instr* GenerateCode();
    
    bool newLocal; //!< True if this variable has not been declared in this scope.
};

/** Instance variable declaration node. */
struct MemberDeclaration : VarDecl
{
    MemberDeclaration(CompileState* s, Id* name) : VarDecl(s, name) {}
    virtual ~MemberDeclaration() {}
    
    virtual void CalculateResources(SymbolTable* st);
};

/** A variable declaration with assignment. This only applies to declarations that
  * have a StorageKind, 
  * <pre>
  * i.e. 
  * local var1, var2, ..., varN = exp1, exp2, ..., expN
  * global var1, var2, ..., varN = exp1, exp2, ..., expN
  * member var1, var2, ..., varN = exp1, exp2, ..., expN
  * </pre>
  * Variable assignments without a storage kind are handled by AssignmentStmt.
  */
struct VariableTarget : DeclarationTarget
{
    VariableTarget(CompileState* s, VarDecl* decls, ExprList* exprs, StorageKind sk)
            : DeclarationTarget(s, Decl::DECL_variable, sk),
            curr_decl(0),
            decls(decls),
            exprs(exprs),
            unpackCount(0), isUnpack(false), isCall(false)    
    {}
    
    virtual void CalculateResources(SymbolTable* st);
    virtual Instr* GenerateCode();
    virtual const char* GetIdentifierName() { return curr_decl->name->name; }
    
    VarDecl*  curr_decl;
    VarDecl*  decls;
    ExprList* exprs;
    u2        unpackCount;
    bool      isUnpack;
    bool      isCall;
};

/** Base class for all statement nodes. */
struct Stmt : TreeNode
{
    enum Kind {
        STMT_empty,
        STMT_list,
        STMT_block,
        STMT_labeled,
        STMT_expr,
        STMT_decl,
        STMT_if,
        STMT_while,
        STMT_loop,
        STMT_foreach,
        STMT_for,
        STMT_return,
        STMT_break,
        STMT_continue,
        STMT_yield,
        STMT_try,
        STMT_raise,
        STMT_finallyblock,
        STMT_with,
        STMT_case,
        STMT_assert,
    };
    
    Stmt(CompileState* s, Kind kind) : TreeNode(s), newline(false), kind(kind) {}
    
    virtual bool    IsLoop() const { return false; }
    virtual void    DoStmtResources(SymbolTable* st) = 0;
    virtual Instr*  DoStmtCodeGen() = 0;
    
    // Derived classes of stmt should NOT override CalculateResources, instead they
    // implement DoStmtResources.
    virtual void CalculateResources(SymbolTable* st)
    {
        newline = state->UpdateLineInfo(line);
        this->DoStmtResources(st); // Allow the derived class a chance calculate its resources.
    }
    
    virtual Instr* GenerateCode();
    
    bool newline; //!< Statement lies on a new line.
    Kind kind;    //!< Type of statement.
};

/** Finally statement block. A finally statement wraps other blocks. */
struct FinallyStmt : Stmt
{
    FinallyStmt(CompileState* s, Stmt* block, Stmt* finalize_block);
    virtual ~FinallyStmt();
    
    virtual void DoStmtResources(SymbolTable* st);
    virtual Instr* DoStmtCodeGen();
    
    Stmt* block;         //!< The block we are wrapping.
    SymbolTable* symtab; //!< Block's SymbolTable.
    Stmt* finalize_block;//!< The block between finally & end.
};

/** Raise or re-raise statement.  */
struct RaiseStmt : Stmt
{
    RaiseStmt(CompileState* s, Expr* expr)
            : Stmt(s, Stmt::STMT_raise),
            reraiseOffset((u2) - 1), expr(expr)
    {}
    
    virtual void DoStmtResources(SymbolTable* st);
    virtual Instr* DoStmtCodeGen();
    
    u2 reraiseOffset; //!< Offset of the catch variable if doing a re-raise.
    Expr* expr; //!< The expression following the raise statement.
};


struct CatchIsBlock : TreeNode, TLinked<CatchIsBlock>
{
    CatchIsBlock(CompileState* s, Id* c, Expr* e, Stmt* b) 
    :   TreeNode(s),
        TLinked<CatchIsBlock>(),
        catchvar(c),
        isexpr(e),
        symbol(0),
        block(b),
        symtab(0)
    {}
    
    virtual ~CatchIsBlock();
    virtual void CalculateResources(SymbolTable*);
    
    Id*     catchvar;
    Expr*   isexpr;
    Symbol* symbol;
    Stmt*   block;
    SymbolTable* symtab;
};

struct TryStmt : Stmt
{
    TryStmt(CompileState* s, Stmt* tryBlock, Stmt* catchBlock, Id* caughtVar, CatchIsBlock* cisb, Stmt* elsebody)
            : Stmt(s, Stmt::STMT_try),
            tryBlock(tryBlock),
            catchBlock(catchBlock),
            caughtVar(caughtVar),
            symbol(0),
            tryTab(0),
            catchTab(0),
            catchis(cisb),
            elseblock(elsebody) {}
            
    virtual ~TryStmt();
    
    virtual void DoStmtResources(SymbolTable* st);
    virtual Instr* DoStmtCodeGen();
    
    Stmt*         tryBlock;  // Block containing the try statement(s).
    Stmt*         catchBlock;// Block containing the catch 'all' statement(s).
    Id*           caughtVar; // Identifier for the catch 'all' block's exception.
    Symbol*       symbol;    // Symbol for caughtVar.
    SymbolTable*  tryTab;    // Try block's symbol table.
    SymbolTable*  catchTab;  // Catch 'all' block's symbol table.
    CatchIsBlock* catchis;   // Block containing the catch is statements, identifiers, symbols and symbol table.
    Stmt*         elseblock; // Block containing the else statement(s).
};

// LabeledStmt /////////////////////////////////////////////////////////////////////////////////////

struct LabeledStmt : Stmt
{
    LabeledStmt(CompileState* s, Id* id, Stmt* stmt)
            : Stmt(s, Stmt::STMT_labeled),
            id(id),
            label(0),
            stmt(stmt)
    {}
    
    virtual void DoStmtResources(SymbolTable* st);
    virtual Instr* DoStmtCodeGen();
    
    Id* id;
    Symbol* label;
    Stmt* stmt;
};

// LoopingStmt /////////////////////////////////////////////////////////////////////////////////////

struct LoopingStmt : Stmt
{
    LoopingStmt(CompileState* s, Stmt::Kind k) : Stmt(s, k), label(0) {}
    
    virtual bool IsLoop() const { return true; }
    
    Symbol* label;
};

// BreakStmt ///////////////////////////////////////////////////////////////////////////////////////

struct BreakStmt : Stmt
{
    BreakStmt(CompileState* s, Id *id)
            : Stmt(s, Stmt::STMT_break),
            label(0),
            id(id)
    {}
    
    virtual void DoStmtResources(SymbolTable* st);
    virtual Instr* DoStmtCodeGen();
    
    Symbol* label;
    Id* id;
};

// ContinueStmt ////////////////////////////////////////////////////////////////////////////////////

struct ContinueStmt : Stmt
{
    ContinueStmt(CompileState* s, Id *id)
            : Stmt(s, Stmt::STMT_continue),
            label(0),
            id(id)
    {}
    
    virtual void DoStmtResources(SymbolTable* st);
    virtual Instr* DoStmtCodeGen();
    
    Symbol* label;
    Id* id;
};

// Expr ////////////////////////////////////////////////////////////////////////////////////////////

struct Expr : TreeNode
{
    enum Kind
    {
        EXPR_conditional,
        EXPR_nullselect,
        EXPR_slice,
        EXPR_add,
        EXPR_sub,
        EXPR_mul,
        EXPR_div,
        EXPR_intdiv,
        EXPR_mod,
        EXPR_catsp,
        EXPR_cat,
        EXPR_bind,
        EXPR_eq,
        EXPR_ne,
        EXPR_same,
        EXPR_notsame,
        EXPR_is,
        EXPR_has,
        EXPR_lt,
        EXPR_gt,
        EXPR_lte,
        EXPR_gte,
        EXPR_bitand,
        EXPR_bitor,
        EXPR_bitxor,
        EXPR_and,
        EXPR_or,
        EXPR_xor,
        EXPR_lsh,
        EXPR_rsh,
        EXPR_ursh,
        EXPR_dot,
        //prefix unary
        EXPR_lognot,
        EXPR_bitnot,
        EXPR_positive,
        EXPR_negative,
        EXPR_preincr,
        EXPR_predecr,
        //postfix unary
        EXPR_postincr,
        EXPR_postdecr,
        EXPR_call,
        EXPR_identifier,
        EXPR_member,
        EXPR_string,
        EXPR_integer,
        EXPR_real,
        EXPR_function,
        EXPR_property,
        EXPR_dictionary,
        EXPR_array,
        EXPR_pow,
        EXPR_load,
        EXPR_dotbind,
        EXPR_new,
        EXPR_arraycomp,
        EXPR_invalid,
        EXPR_paren,    
        EXPR_namenode, 
        EXPR_kwarg,
    };
    
    Expr(CompileState* s, Kind kind) : TreeNode(s), kind(kind) {}
    
    Kind kind;
};

struct ParenExpr : Expr
{
    ParenExpr(CompileState* s, Expr* e) : Expr(s, Expr::EXPR_paren), expr(e) {}
    virtual ~ParenExpr();
    
    virtual void   CalculateResources(SymbolTable* st);
    virtual Instr* GenerateCode();
    
    Expr* expr;
};

// PropertyDecl ////////////////////////////////////////////////////////////////////////////////////

struct PropertyDecl : NamedTarget
{
    PropertyDecl(CompileState* s, NameNode* name, Expr* get_expr, Expr* set_expr, StorageKind sk)
            : NamedTarget(s, name, Decl::DECL_property, sk),
            name_expr(0),
            getter(get_expr),
            setter(set_expr)
    {}
    
    virtual ~PropertyDecl();
    
    virtual void CalculateResources(SymbolTable* st);
    
    
    virtual Instr* GenerateCode();
    virtual Instr* GeneratePropertyCode();
    
    StringExpr* name_expr;
    Expr* getter;
    Expr* setter;
};

struct LoadExpr : Expr
{
    enum LoadKind
    {
        LK_super,
        LK_self,
        LK_null,
        LK_true,
        LK_false,
        LK_locals,
    };
    
    LoadExpr(CompileState* s, LoadKind k) : Expr(s, Expr::EXPR_load), loadkind(k) {}
    
    virtual void CalculateResources(SymbolTable* st);
    virtual Instr* GenerateCode();
    
    LoadKind loadkind;
};

////////////////////////////////////////////// CallExpr ////////////////////////////////////////////

struct CallExpr : Expr
{
    CallExpr(CompileState* s, Expr *left, ExprList *args)
            : Expr(s, Expr::EXPR_call),
            left(left),
            args(args), argc(0), kwargc(), retc(1), redirectedcall(0) {}
            
    virtual void CalculateResources(SymbolTable* st);
    virtual Instr* GenerateCode();
        
    Expr* left;
    ExprList* args;
    u2 argc;
    u2 kwargc;
    u2 retc;
    u1 redirectedcall;
};


////////////////////////////////////////////// CondExpr ////////////////////////////////////////////

struct CondExpr : Expr
{
    CondExpr(CompileState* s, Expr* cond, Expr* a, Expr* b, bool unless = false)
            : Expr(s, Expr::EXPR_conditional),
            cond(cond),
            exprA(a),
            exprB(b),
            unless(unless) {}
            
    virtual void CalculateResources(SymbolTable* st);
    virtual Instr* GenerateCode();
    
    Expr* cond;
    Expr* exprA;
    Expr* exprB;
    bool  unless;
};

/////////////////////////////////////////// NullSelectExpr /////////////////////////////////////////

struct NullSelectExpr : Expr
{
    NullSelectExpr(CompileState* s, Expr* a, Expr* b) : Expr(s, Expr::EXPR_nullselect), exprA(a), exprB(b) {}
    
    virtual void CalculateResources(SymbolTable* st);
    virtual Instr* GenerateCode();
    
    Expr* exprA;
    Expr* exprB;
};

///////////////////////////////////////////// SliceExpr ////////////////////////////////////////////

struct SliceExpr : Expr
{
    SliceExpr(CompileState* s, Expr* expr, Expr* from, Expr* to)
            : Expr(s, Expr::EXPR_slice),
            expr(expr),
            from(from),
            to(to),
            slicefun(0)
    {}
    
    virtual ~SliceExpr();
    
    virtual void CalculateResources(SymbolTable* st);
    virtual Instr* GenerateCode();
    
    Expr* expr;
    Expr* from;
    Expr* to;
    MemberExpr* slicefun;
};

////////////////////////////////////////////// StmtList ////////////////////////////////////////////

struct StmtList : Stmt
{
    StmtList(CompileState* s, Stmt *first, Stmt *second)
            : Stmt(s, Stmt::STMT_list),
            first(first),
            second(second) {}
            
    virtual void DoStmtResources(SymbolTable* st)
    {
        if (first) first->CalculateResources(st);
        if (second) second->CalculateResources(st);
    }
    
    virtual Instr* DoStmtCodeGen();
    
    Stmt* first;
    Stmt* second;
};

///////////////////////////////////////////// EmptyStmt ////////////////////////////////////////////

struct EmptyStmt : Stmt
{
    EmptyStmt(CompileState* s) : Stmt(s, Stmt::STMT_empty) {}
    
    virtual void DoStmtResources(SymbolTable* st) {}
    Instr* DoStmtCodeGen();
};

////////////////////////////////////////////// ExprStmt ////////////////////////////////////////////

struct ExprStmt : Stmt
{
    ExprStmt(CompileState* s, ExprList* expr) : Stmt(s, Stmt::STMT_expr), exprList(expr), autopop(true) {}
    
    virtual void DoStmtResources(SymbolTable* st);
    virtual Instr* DoStmtCodeGen();
    
    ExprList* exprList;
    bool autopop;
};

////////////////////////////////////////////// RetStmt /////////////////////////////////////////////

struct RetStmt : Stmt
{
    RetStmt(CompileState* s, ExprList* el) : Stmt(s, Stmt::STMT_return), exprs(el), count(0), isInTry(false) {}
    
    virtual void    DoStmtResources(SymbolTable* st);
    virtual Instr*  DoStmtCodeGen();
    
    ExprList* exprs;
    size_t    count;
    bool isInTry;
};

///////////////////////////////////////////// YieldStmt ////////////////////////////////////////////

struct YieldStmt : Stmt
{
    YieldStmt(CompileState* s, ExprList* el) : Stmt(s, Stmt::STMT_yield), exprs(el), count(0) {}
    
    virtual void DoStmtResources(SymbolTable* st);
    virtual Instr* DoStmtCodeGen();
    
    ExprList* exprs;
    size_t    count;
};

////////////////////////////////////////////// LoopStmt ////////////////////////////////////////////

struct LoopStmt : LoopingStmt
{
    LoopStmt(CompileState* s, Stmt *body) : LoopingStmt(s, Stmt::STMT_loop), body(body) {}
    
    virtual void DoStmtResources(SymbolTable* st);
    virtual Instr* DoStmtCodeGen();
    
    Stmt* body;
};

//////////////////////////////////////////// CondLoopStmt //////////////////////////////////////////

struct CondLoopStmt : LoopingStmt
{
    CondLoopStmt(CompileState* s, Expr* cond, Stmt* body, bool repeat, bool until)
            : LoopingStmt(s, Stmt::STMT_while),
            cond(cond),
            body(body),
            repeat(repeat),
            until(until) {}
            
    virtual void DoStmtResources(SymbolTable* st);
    virtual Instr* DoStmtCodeGen();
    
    Expr* cond;
    Stmt* body;
    bool repeat;
    bool until;
};

///////////////////////////////////////////// ForToStmt ////////////////////////////////////////////

struct ForToStmt : LoopingStmt
{
    ForToStmt(CompileState* s, Id* id, Expr* from, Expr* to, Expr* step, Stmt* body, bool down)
            : LoopingStmt(s, Stmt::STMT_for),
            id(id),
            from(from),
            to(to),
            step(step),
            body(body),
            down(down),
            symbol(0),
            symtab(0) {}
    
    virtual ~ForToStmt();
    
    virtual void   DoStmtResources(SymbolTable* st);
    virtual Instr* DoStmtCodeGen();
    
    Id*          id;
    Expr*        from;
    Expr*        to;
    Expr*        step;
    Stmt*        body;
    bool         down;
    Symbol*      symbol;
    SymbolTable* symtab;
};

//////////////////////////////////////////// ForeachStmt ///////////////////////////////////////////

struct ForeachStmt : LoopingStmt
{
    ForeachStmt(CompileState* s, Id* id, Expr* type_expr, Expr* in, Stmt* body)
            : LoopingStmt(s, Stmt::STMT_foreach),
            id(id),
            iterVar(0),
            type_expr(type_expr),
            in(in),
            body(body),
            idexpr(0),
            enum_offset(0),
            symtab(0) {}
    
    virtual ~ForeachStmt();
    
    virtual void   DoStmtResources(SymbolTable* st);
    virtual Instr* DoStmtCodeGen();
    Id*          id;
    VarDecl*     iterVar;
    Expr*        type_expr;   // The set name that will be enumerated.
    Expr*        in;          // The subject.
    Stmt*        body;        // Loop body.
    IdExpr*      idexpr;      // Expression for the loop variable.
    size_t       enum_offset; // Local var offset for the enumerator object.
    SymbolTable* symtab;
};

/////////////////////////////////////////////// IfStmt /////////////////////////////////////////////

struct IfStmt : Stmt
{
    IfStmt(CompileState* s, Expr* cond, Stmt* then_part, bool unless = false)
            : Stmt(s, Stmt::STMT_if),
            next(0),
            cond(cond),
            then_part(then_part),
            is_unless(unless) {}
            
    virtual void DoStmtResources(SymbolTable* st);
    virtual Instr* DoStmtCodeGen();
    
    IfStmt* next;
    Expr* cond;
    Stmt* then_part;
    bool is_unless;
    
    void Attach(IfStmt* nxt)
    {
        if (!next)
        {
            next = nxt;
        }
        else
        {
            IfStmt *curr = next;
            while (curr->next)
                curr = curr->next;
            curr->next = nxt;
        }
    }
};

////////////////////////////////////////// ConditionalStmt /////////////////////////////////////////

struct ConditionalStmt : Stmt
{
    ConditionalStmt(CompileState* s, IfStmt* ifpart, Stmt* elsepart)
            : Stmt(s, Stmt::STMT_if),
            ifPart(ifpart),
            elsePart(elsepart) {}
            
    virtual void DoStmtResources(SymbolTable* st)
    {
        if (ifPart) ifPart->CalculateResources(st);
        if (elsePart) elsePart->CalculateResources(st);
    }
    
    virtual Instr* DoStmtCodeGen();
    
    IfStmt* ifPart;
    Stmt* elsePart;
};

///////////////////////////////////////////// BlockStmt ////////////////////////////////////////////

struct BlockStmt : Stmt
{
    BlockStmt(CompileState* s, Stmt* stmts)
            : Stmt(s, Stmt::STMT_block),
            stmts(stmts),
            symtab(0) {}
            
    virtual ~BlockStmt();
    virtual void DoStmtResources(SymbolTable* st);
    virtual Instr* DoStmtCodeGen();
    
    Stmt* stmts;
    SymbolTable* symtab;
};

////////////////////////////////////////////// DeclStmt ////////////////////////////////////////////

struct DeclStmt : Stmt
{
    DeclStmt(CompileState* s, Decl *decl);
    
    virtual void DoStmtResources(SymbolTable* st);
    
    virtual Instr* DoStmtCodeGen();
    
    Decl* decl;
};

///////////////////////////////////////////// UnaryExpr ////////////////////////////////////////////

struct UnaryExpr : Expr
{
    UnaryExpr(CompileState* s, Expr::Kind k, Expr* expr)
            : Expr(s, k),
            expr(expr) {}
            
    virtual void   CalculateResources(SymbolTable* st);
    virtual Instr* GenerateCode();
    
    virtual Opcode GetOpcode() const
    {
        switch (kind)
        {
        case EXPR_lognot:   return OP_not;
        case EXPR_bitnot:   return OP_bitnot;
        case EXPR_positive: return OP_pos;
        case EXPR_negative: return OP_neg;
        case EXPR_preincr:  return OP_inc;
        case EXPR_predecr:  return OP_dec;
        case EXPR_postincr: return OP_inc;
        case EXPR_postdecr: return OP_dec;
        default:            return OP_nop;
        };
        return OP_nop;
    }
    
    Expr* expr;
};

///////////////////////////////////////////// BinaryExpr ///////////////////////////////////////////

struct BinaryExpr : Expr
{
    BinaryExpr(CompileState* s, Expr::Kind k, Expr* left, Expr* right)
            : Expr(s, k),
            left(left),
            right(right)
    {}
    
    virtual void CalculateResources(SymbolTable* st);
    
    virtual Opcode GetOpcode() const
    {
        switch (kind)
        {
        case EXPR_add:      return OP_add;
        case EXPR_sub:      return OP_sub;
        case EXPR_mul:      return OP_mul;
        case EXPR_div:      return OP_div;
        case EXPR_intdiv:   return OP_idiv;
        case EXPR_mod:      return OP_mod;
        case EXPR_catsp:    return OP_catsp;
        case EXPR_cat:      return OP_cat;
        case EXPR_eq:       return OP_eq;
        case EXPR_ne:       return OP_ne;
        case EXPR_same:     return OP_same;
        case EXPR_notsame:  return OP_notsame;
        case EXPR_is:       return OP_is;
        case EXPR_has:      return OP_has;
        case EXPR_lt:       return OP_lt;
        case EXPR_gt:       return OP_gt;
        case EXPR_lte:      return OP_lte;
        case EXPR_gte:      return OP_gte;
        case EXPR_bitand:   return OP_bitand;
        case EXPR_bitor:    return OP_bitor;
        case EXPR_bitxor:   return OP_bitxor;
        case EXPR_xor:      return OP_xor;
        case EXPR_lsh:      return OP_lsh;
        case EXPR_rsh:      return OP_rsh;
        case EXPR_ursh:     return OP_ursh;
        case EXPR_pow:      return OP_pow;
        default:            return OP_nop;
        };
        return OP_nop;
    }
    virtual Instr* GenerateCode();
    
    Expr* left;
    Expr* right;
};

/////////////////////////////////////////// IdExpr /////////////////////////////////////////

struct IdExpr : Expr
{
    IdExpr(CompileState* s, Id* id)
            : Expr(s, Expr::EXPR_identifier),
            depthindex(0),
            index(0),
            id(id),
            symbol(0),
            outer(false),
            inWithBlock(false), outerwith(false), newLocal(false)
    {}
    
    virtual void CalculateResources(SymbolTable* st);
    
    virtual Instr* GenerateCode();
    virtual Instr* GenerateCodeSet();
    
    bool ExplicitWith()   const { return !outerwith && symbol && symbol->isWith;     }
    bool ExplicitGlobal() const { return symbol && symbol->isGlobal;                 }
    bool IsWithSet()      const { return ExplicitWith() || (inWithBlock && !symbol); }
    bool IsWithGet()      const { return ExplicitWith();                             }
    bool IsLocal()        const { return !outer && (symbol && !symbol->isGlobal);    }
    bool IsOuter()        const { return outer;                                      }
    bool IsGlobal()       const { return (!outer && (!symbol || ExplicitGlobal()));  }
    
    u4 depthindex;
    u2 index;
    Id* id;
    Symbol* symbol;
    bool outer;
    bool inWithBlock;   // the block we live in is a with-block
    bool outerwith;     // this identifier appears in a different with block than the current block!
    bool newLocal;
};



//////////////////////////////////////////// ConstantExpr //////////////////////////////////////////

struct ConstantExpr : Expr
{
    ConstantExpr(CompileState* s, Expr::Kind k)
            : Expr(s, k),
            index(0) {}
            
    virtual Instr* GenerateCode();
    
    u2 index;
};

////////////////////////////////////////////// FunExpr /////////////////////////////////////////////

struct FunExpr : ConstantExpr
{
    FunExpr(CompileState* s, ParamDecl* args, Stmt* body, size_t begtxt, size_t endtxt, Id* name = 0)
    : ConstantExpr(s, Expr::EXPR_function),
    args(args),
    name(name),
    body(body),
    def(0),
    symtab(0),
    scriptBeg(begtxt),
    scriptEnd(endtxt) {}
            
    virtual ~FunExpr();
    
    virtual void CalculateResources(SymbolTable* st);
    virtual Instr* GenerateCode();
    
    ParamDecl* args;
    Id* name;
    Stmt* body;
    Def* def;
    SymbolTable* symtab;
    size_t scriptBeg;
    size_t scriptEnd;
};

struct PropExpr : Expr
{
    PropExpr(CompileState* s, Expr* name, Expr* getfn, Expr* setfn)
    : Expr(s, Expr::EXPR_property),
    nameexpr(name),
    getter(getfn),
    setter(setfn) {}
    
    virtual void CalculateResources(SymbolTable* st);
    virtual Instr* GenerateCode();
    
    Expr* nameexpr;
    Expr* getter;
    Expr* setter;
};

///////////////////////////////////////////// MemberExpr ///////////////////////////////////////////

struct MemberExpr : ConstantExpr
{
    MemberExpr(CompileState* s, Id *id)
            : ConstantExpr(s, Expr::EXPR_member),
            id(id) {}
            
    virtual void CalculateResources(SymbolTable* st);
    
    Id* id;
};

///////////////////////////////////////////// StringExpr ///////////////////////////////////////////

struct StringExpr : ConstantExpr
{
    StringExpr(CompileState* s, char *string, size_t len)
            : ConstantExpr(s, Expr::EXPR_string),
            string(string),
            length(len) {}
            
    virtual ~StringExpr();
    virtual void CalculateResources(SymbolTable* st);
    
    char* string;
    size_t length;
};

//////////////////////////////////////////// IntegerExpr ///////////////////////////////////////////

struct IntegerExpr : ConstantExpr
{
    IntegerExpr(CompileState* s, pint_t integer)
            : ConstantExpr(s, Expr::EXPR_integer),
            integer(integer) {}
            
    virtual void CalculateResources(SymbolTable* st);
    
    pint_t integer;
};

////////////////////////////////////////////// RealExpr ////////////////////////////////////////////

struct RealExpr : ConstantExpr
{
    RealExpr(CompileState* s, preal_t real)
            : ConstantExpr(s, Expr::EXPR_real),
            real(real) {}
            
    virtual void CalculateResources(SymbolTable* st);
    
    preal_t real;
};

////////////////////////////////////////////// DotExpr /////////////////////////////////////////////

/** A binary expression that seperates two expression by the '.' character. The rhe specifies 
  * the value to get/set from the lhe.
  *     ie  foo.bar
  *         foo.bar.bazz
  */
struct DotExpr : BinaryExpr
{
    DotExpr(CompileState* s, Expr*, Expr*);
    
    virtual void    CalculateResources(SymbolTable* st);
    
    virtual Instr*  GenerateCode();
    virtual Instr*  GenerateCodeSet();
    
    virtual Opcode  GetOpcode() const { return OP_dotget; }
    virtual Opcode  SetOpcode() const { return OP_dotset; } 
};

/** A binary expression where the right hand expression (rhe) is enclosed in square brackets and placed in a post-fix position
  * relative to the left hand expression (lhe). The rhe specifies the value to get/set from the lhe.
  *     ie  foo['bar']
  *         foo.bar['bazz']
  */
struct IndexExpr : DotExpr
{
    IndexExpr(CompileState* s, Expr* l, Expr* r) : DotExpr(s, l, r)
    {
    }
    virtual void    CalculateResources(SymbolTable* st);
    
    virtual Instr*  GenerateCode();
    virtual Instr*  GenerateCodeSet();    
    
    virtual Opcode  GetOpcode() const { return OP_subget; }
    virtual Opcode  SetOpcode() const { return OP_subset; }
};

/** A dot expression in which the value being retrieved and the object it belongs to are bound together as one value.
  *     ie  bind foo.bar
  *         bind foo.bar.bazz
  */
struct DotBindExpr : DotExpr
{
    DotBindExpr(CompileState* s, Expr *l, Expr *r, bool index = false) 
        : DotExpr(s, l, r), is_index(index)
    { 
        kind = Expr::EXPR_dotbind; 
    }
    
    virtual Instr* GenerateCode();
    virtual Instr* GenerateCodeSet();
    
    virtual Opcode  GetOpcode() const { return is_index ? OP_subget : OP_dotget; }
    virtual Opcode  SetOpcode() const { return is_index ? OP_subset : OP_dotset; }
        
    bool is_index;
};

/** A list of (key, value) pairs used as the elements of an object literal. */
struct FieldList : TreeNode
{
    FieldList(CompileState* s, Expr* name, Expr* value) : TreeNode(s), dtarget(0), name(name), value(value), next(0) {}
    
    FieldList(CompileState* s, DeclarationTarget* dt) : TreeNode(s), dtarget(dt), name(0), value(0), next(0) {}
    
    virtual void CalculateResources(SymbolTable* st)
    {
        if (dtarget)
        {
            dtarget->CalculateResources(st);
        }
        else
        {
            if (name) name ->CalculateResources(st);
            if (value) value->CalculateResources(st);
        }
    }
    
    void Attach(FieldList* nxt)
    {
        if (!next)
            next = nxt;
        else
        {
            FieldList* curr = next;
            while (curr->next)
                curr = curr->next;
            curr->next = nxt;
        }
    }
    
    DeclarationTarget* dtarget;
    Expr* name;
    Expr* value;
    FieldList* next;
};

/** An object or dictionary literal. */
struct DictionaryExpr : Expr
{
    DictionaryExpr(CompileState* s, FieldList* fields) : Expr(s, Expr::EXPR_dictionary), type_expr(0), fields(fields) {}
    
    virtual void    CalculateResources(SymbolTable* st);
    virtual Instr*  GenerateCode();
    
    Expr*       type_expr;  //!< The specified type of this object.
    FieldList*  fields;     //!< List of members for this object.
};

/** An array literal consisting of comma seperated elements enclosed by square brackets. 
  * ie  []
  *     [1, 2, 3]       
  *     [a, b, c, 1, 2, 3]
  */
struct ArrayExpr : Expr
{
    ArrayExpr(CompileState* s, ExprList* elements) : Expr(s, Expr::EXPR_array), numElements(0), elements(elements) {}
    
    virtual void CalculateResources(SymbolTable* st);
    virtual Instr* GenerateCode();
    
    size_t      numElements;    //!< Length of the array.
    ExprList*   elements;       //!< elements of the array.
};

struct KeywordExpr : Expr
{
    KeywordExpr(CompileState* s, StringExpr* name__, Expr* value__) : Expr(s, Expr::EXPR_kwarg), name(name__), value(value__) {}
    virtual ~KeywordExpr() {}
    
    virtual void CalculateResources(SymbolTable* st);
    virtual Instr* GenerateCode();
    StringExpr* name;
    Expr* value;
};

// ExprList ///////////////////////////////////////////////////////////////////////////////////////

struct ExprList : TreeNode
{
    ExprList(CompileState* s, Expr* e) : TreeNode(s), expr(e), next(0) {}
    
    virtual void CalculateResources(SymbolTable* st);
    
    size_t Count();
    
    Expr* expr;
    ExprList* next;
};

INLINE ExprList* ReverseExprList(ExprList** l)
{
    ExprList* temp1 = *l;
    ExprList* temp2 = 0;
    ExprList* temp3 = 0;
    
    while (temp1)
    {
        *l = temp1;             // set the head to last node
        temp2 = temp1->next;    // save the next ptr in temp2
        temp1->next = temp3;    // change next to privous
        temp3 = temp1;
        temp1 = temp2;
    }
    return *l;
}

/////////////////////////////////////////// AssignmentStmt /////////////////////////////////////////

struct AssignmentStmt : Stmt
{
    enum AssignKind
    {
        ASSIGN_STMT,
        BIT_OR_ASSIGN_STMT,
        BIT_XOR_ASSIGN_STMT,
        BIT_AND_ASSIGN_STMT,
        LSH_ASSIGN_STMT,
        RSH_ASSIGN_STMT,
        URSH_ASSIGN_STMT,
        ADD_ASSIGN_STMT,
        SUB_ASSIGN_STMT,
        MUL_ASSIGN_STMT,
        DIV_ASSIGN_STMT,
        IDIV_ASSIGN_STMT,
        MOD_ASSIGN_STMT,
        CONCAT_SPACE_ASSIGN_STMT,
        CONCAT_ASSIGN_STMT,
    };
    
    AssignmentStmt(CompileState* s, ExprList* l, ExprList* r);
    
    virtual void DoStmtResources(SymbolTable* st);
    virtual Instr* DoStmtCodeGen();
    Opcode GetOpcode() const;
    
    AssignKind  kind;
    ExprList*   left;
    ExprList*   right;
    bool        isBinaryOp;
    bool        isUnpack;
    bool        isCall;
    u2          unpackCount;
};

/////////////////////////////////////////// UsingStmt //////////////////////////////////////////

struct UsingStmt : Stmt
{
    UsingStmt(CompileState* s, Expr* e, Stmt* b);
    
    virtual ~UsingStmt();
    
    virtual void DoStmtResources(SymbolTable* st);
    virtual Instr* DoHeader();
    virtual Instr* DoStmtCodeGen();
    
    Expr*           with;
    Stmt*           block;
    SymbolTable*    symtab;
};

////////////////////////////////////////////// PkgDecl /////////////////////////////////////////////

struct PkgDecl : NamedTarget
{
    PkgDecl(CompileState* s, NameNode*, Stmt* body, StorageKind sto);
    
    virtual ~PkgDecl();
    virtual void CalculateResources(SymbolTable* st);
    virtual Instr* GenerateCode();
    
    SymbolTable*    symtab;
    StringExpr*     id;
    Stmt*           body;
};

////////////////////////////////////////////// NameNode ////////////////////////////////////////////

struct NameNode : Expr
{
    NameNode(CompileState* s, IdExpr* idexpr) : Expr(s, Expr::EXPR_namenode), idexpr(idexpr), dotexpr(0) {}
    
    NameNode(CompileState* s, DotExpr* dotexpr) : Expr(s, Expr::EXPR_namenode), idexpr(0), dotexpr(dotexpr) {}
    
    virtual ~NameNode() {}
    
    virtual void    CalculateResources(SymbolTable* st);
    virtual Instr*  GenerateCode();
    virtual Instr*  GenerateCodeSet();
    const char*     GetName();
    
    Expr* GetSelfExpr() { return dotexpr ? dotexpr->left : 0; }
    
    IdExpr*     idexpr;
    DotExpr*    dotexpr;
};

struct ClassDecl : NamedTarget
{
    ClassDecl(CompileState* s, NameNode* id, Expr* super, Stmt* stmts, StorageKind sk)
            : NamedTarget(s, id, Decl::DECL_class, sk),
            stringid(0),
            super(super),
            stmts(stmts),
            symtab(0)
    {}
    
    virtual ~ClassDecl();
    
    virtual void    CalculateResources(SymbolTable* st);
    virtual Instr*  GenerateCode();
    
    StringExpr*     stringid;
    Expr*           super;
    Stmt*           stmts;
    SymbolTable*    symtab;
};

}// pika

#endif
