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
        p->state = state;                \
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
struct          ParamDecl;
struct          VarDecl;
struct              LocalDecl;
struct              MemberDeclaration;
struct      CatchIsBlock;
struct      Stmt;
struct          AssertStmt;
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
struct          WithStatement;
struct          CaseStmt;
struct          DeleteStmt;
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
struct      NameNode;
struct      CaseList;


// StorageKind /////////////////////////////////////////////////////////////////////////////////////

enum StorageKind
{
    STO_none,
    STO_local,
    STO_global,
    STO_member,
};

// TreeNodeList ////////////////////////////////////////////////////////////////////////////////////

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

// CompileState /////////////////////////////////////////////////////////////////////////////////////

struct CompileState
{
    CompileState(Engine*);
    
    ~CompileState();
    
    // Local Offset --------------------------------------------------------------------------------
    
    int  GetLocalOffset() const;       // Gets the current local offset
    void SetLocalOffset(int off);      // Sets the current local offset
    int  NextLocalOffset(const char*); // Gets the current local offset and then moves to the next offset.
    Symbol* CreateLocalPlus(SymbolTable* st, const char* name, size_t extra);
    
    // Literal Handling ----------------------------------------------------------------------------
    
    u2 AddConstant(pint_t i);
    u2 AddConstant(preal_t i);
    u2 AddConstant(Def* fun);
    u2 AddConstant(const char* str);
    u2 AddConstant(const char* str, size_t len);
    
    // Debug line info -----------------------------------------------------------------------------
    
    void SetLineInfo(u2 line);
    bool UpdateLineInfo(int line);
    
    // Error Reporting -----------------------------------------------------------------------------
#if defined(ENABLE_SYNTAX_WARNINGS)    

    void SyntaxWarning(WarningLevel level, int line, const char* format, ...);
#endif
    void   SyntaxError(int line, const char* format, ...);           
    void   SyntaxException(Exception::Kind k, int line,          const char *msg, ...);
    void   SyntaxException(Exception::Kind k, int line, int col, const char *msg, ...); 
    void   SyntaxErrorSummary();
    
    /** Returns true if syntax errors occurred while parsing and compiling a script. */
    bool HasErrors() const { return errors != 0; }
    
    void   SetParser(Parser* p) { parser = p; }
    struct TryState
    {
        bool inTry;
        bool inCatch;               // Are we in a catch block.
        u2   catchVarOffset;        // Location of the caught exception (used to re-raise a caught exception).
    }               trystate;
    TreeNodeList    nodes;          // All the nodes we know about.
    LiteralPool*    literals;       // Literals used in this program. Shared by all child functions.
    Table           literalLookup;
    Engine*         engine;
    Def*            currDef;        // Current function definition being compiled.
    int             localOffset;    // Next local variable offset.
    int             localCount;     // Total number of lexEnv.
    u2              currentLine;    // Current line being processed.
    int             errors;         // Number of errors found.
    int             warnings;       // Number of warnings found.
    Instr*          endOfBlock;     // Last Instr of the current block (used to track local variable scope.)
    Parser*         parser;
};

// TreeNode ////////////////////////////////////////////////////////////////////////////////////////

struct TreeNode
{
    TreeNode() : state(0), line(0), astnext(0) {}
    
    virtual ~TreeNode() {}
    
    virtual void   CalculateResources(SymbolTable* st, CompileState& cs) {}
    virtual Instr* GenerateCode();
    
    CompileState* state;
    int           line;
    TreeNode*     astnext;
};


// Program /////////////////////////////////////////////////////////////////////////////////////////

struct Program : TreeNode
{
    Program(Stmt *stmts, size_t beg, size_t end)
            : index(0),
            def(0),
            stmts(stmts),
            symtab(0),
            scriptBeg(beg),
            scriptEnd(end) {}
            
    virtual ~Program();
    
    virtual void   CalculateResources(SymbolTable* st, CompileState& cs);
    virtual Instr* GenerateCode();
    
    u2              index;
    Def*    def;
    Stmt*           stmts;
    SymbolTable*    symtab;
    size_t          scriptBeg;
    size_t          scriptEnd;
};

struct FunctionProg : Program
{
    FunctionProg(Stmt* stmts, size_t beg, size_t end, ParamDecl* a)
            : Program(stmts, beg, end), args(a)
    {}
    
    virtual ~FunctionProg() {}
    
    virtual void CalculateResources(SymbolTable* st, CompileState& cs);
    
    ParamDecl* args;
};

// Id //////////////////////////////////////////////////////////////////////////////////////

struct Id : TreeNode
{
    Id(char* name);
    virtual ~Id();
    
    char* name;
    Id*   next;
};

// Decl /////////////////////////////////////////////////////////////////////////////////////

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
    };
    
    Decl(Kind kind)
            : kind(kind)
    {}
    
    virtual ~Decl()
    {}
    
    Kind kind;
};

// DeclarationTarget ///////////////////////////////////////////////////////////////////////////////

struct DeclarationTarget : Decl
{
    DeclarationTarget(Decl::Kind k, StorageKind sk)
            : Decl(k),
            storage(sk),
            with(false),
            symbol(0),
            nameindex(0)
    {}
    
    virtual ~DeclarationTarget() {}
    
    virtual const char* GetIdentifierName() = 0;
    virtual void        CreateSymbol(SymbolTable* st, CompileState& cs);
    virtual Instr*      GenerateCodeSet();
    
    
    StorageKind storage;
    bool        with;
    Symbol*     symbol;
    u2          nameindex;
};

// NamedTarget /////////////////////////////////////////////////////////////////////////////////////

struct NamedTarget : DeclarationTarget
{
    NamedTarget(NameNode* n, Decl::Kind k, StorageKind sk) : DeclarationTarget(k, sk), already_decl(false), name(n) {}
    
    virtual void        CalculateResources(SymbolTable* st, CompileState& cs);
    virtual const char* GetIdentifierName();
    virtual Instr*      GenerateCodeSet();
    virtual Instr*      GenerateCodeWith(Instr* body);
    
    bool        already_decl;
    NameNode*   name;
};

// FunctionDecl ////////////////////////////////////////////////////////////////////////////////////

struct FunctionDecl : DeclarationTarget
{
    FunctionDecl(NameNode*, ParamDecl*, Stmt*, size_t, size_t, StorageKind);
    virtual ~FunctionDecl();
    
    virtual void        CalculateResources(SymbolTable* st, CompileState& cs);
    virtual Instr*      GenerateCode();
    virtual const char* GetIdentifierName();
    
    Def*    def;
    u2              index;
    ParamDecl*      args;
    Stmt*           body;
    SymbolTable*    symtab;
    NameNode*       name;
    bool            varArgs;
    size_t          scriptBeg;
    size_t          scriptEnd;
};

// ParamDecl ///////////////////////////////////////////////////////////////////////////////////////

struct ParamDecl : Decl
{
    ParamDecl(Id* name, bool vargs, Expr* val);
    
    void            CalculateDefaultResources(SymbolTable* st, CompileState& cs);
    virtual void    CalculateResources(SymbolTable* st, CompileState& cs);
    void            Attach(ParamDecl* nxt);
    bool            HasDefaultValue();
    bool            HasVarArgs();
    Expr*       val;
    Symbol*     symbol;
    Id*         name;
    bool        has_rest; // Parameter was proceeded by '...'
    ParamDecl*  next;
};

// VarDecl /////////////////////////////////////////////////////////////////////////////////////////

struct VarDecl : Decl
{
    VarDecl(Id* name)
            : Decl(Decl::DECL_variable),
            nameIndex(0),
            symbol(0),
            name(name),
            next(0)
    {}
    
    virtual void    CalculateResources(SymbolTable* st, CompileState& cs);
    virtual Instr*  GenerateCode();
    
    u2          nameIndex;
    Symbol*     symbol;
    Id*         name;
    VarDecl*    next;
};

// LocalDecl ///////////////////////////////////////////////////////////////////////////////////////

struct LocalDecl : VarDecl
{
    LocalDecl(Id* name) : VarDecl(name), newLocal(false) {}
    
    virtual void CalculateResources(SymbolTable* st, CompileState& cs);
    virtual Instr* GenerateCode();
    bool newLocal;
};

// MemberDeclaration ///////////////////////////////////////////////////////////////////////////////

struct MemberDeclaration : VarDecl
{
    MemberDeclaration(Id* name) : VarDecl(name) {}
    virtual ~MemberDeclaration() {}
    
    virtual void CalculateResources(SymbolTable* st, CompileState& cs);
};

// VariableTarget //////////////////////////////////////////////////////////////////////////////////

struct VariableTarget : DeclarationTarget
{
    VariableTarget(VarDecl* decls, ExprList* exprs, StorageKind sk)
            : DeclarationTarget(Decl::DECL_variable, sk),
            curr_decl(0),
            decls(decls),
            exprs(exprs),
            unpackCount(0), isUnpack(false), isCall(false)    
    {}
    
    virtual void CalculateResources(SymbolTable* st, CompileState& cs);
    virtual Instr* GenerateCode();
    virtual const char* GetIdentifierName() { return curr_decl->name->name; }
    
    VarDecl*  curr_decl;
    VarDecl*  decls;
    ExprList* exprs;
    u2        unpackCount;
    bool      isUnpack;
    bool      isCall;
};

// Stmt ////////////////////////////////////////////////////////////////////////////////////////////

struct Stmt : TreeNode
{
    enum Kind
    {
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
        STMT_ensureblock,
        STMT_with,
        STMT_case,
        STMT_assert,
        STMT_delete,
    };
    
    Stmt(Kind kind) : newline(false), kind(kind) {}
    
    virtual bool    IsLoop() const { return false; }
    virtual void    DoStmtResources(SymbolTable* st, CompileState& cs) = 0;
    virtual Instr*  DoStmtCodeGen() = 0;
    
    // Derived classes of stmt should NOT override CalculateResources, instead they
    // implement DoStmtResources.
    virtual void CalculateResources(SymbolTable* st, CompileState& cs)
    {
        newline = cs.UpdateLineInfo(line);
        this->DoStmtResources(st, cs); // Allow the derived class a chance calculate its resources.
    }
    
    virtual Instr* GenerateCode();
    
    u1 newline;
    Kind kind;
};

struct AssertStmt : Stmt
{
    AssertStmt(Expr* e) : Stmt(Stmt::STMT_assert), expr(e) {}
    
    virtual void DoStmtResources(SymbolTable* st, CompileState& cs);
    virtual Instr* DoStmtCodeGen();
    
    Expr* expr;
};


// FinallyStmt ////////////////////////////////////////////////////////////////////////////////

struct FinallyStmt : Stmt
{
    FinallyStmt(Stmt* block, Stmt* ensured_block);
    virtual ~FinallyStmt();
    
    virtual void DoStmtResources(SymbolTable* st, CompileState& cs);
    virtual Instr* DoStmtCodeGen();
    
    Stmt* block;
    SymbolTable* symtab;
    Stmt* ensured_block;
};

// RaiseStmt //////////////////////////////////////////////////////////////////////////////////

struct RaiseStmt : Stmt
{
    RaiseStmt(Expr* expr)
            : Stmt(Stmt::STMT_raise),
            reraiseOffset((u2) - 1), expr(expr)
    {}
    
    virtual void DoStmtResources(SymbolTable* st, CompileState& cs);
    virtual Instr* DoStmtCodeGen();
    
    u2 reraiseOffset;
    Expr* expr;
};

// TryStmt ////////////////////////////////////////////////////////////////////////////////////

struct CatchIsBlock : TreeNode, TLinked<CatchIsBlock>
{
    CatchIsBlock(Id* c, Expr* e, Stmt* b) :
            catchvar(c),
            isexpr(e),
            symbol(0),
            block(b),
            symtab(0)
    {}
    
    virtual ~CatchIsBlock();
    virtual void CalculateResources(SymbolTable*, CompileState&);
    
    Id*     catchvar;
    Expr*   isexpr;
    Symbol* symbol;
    Stmt*   block;
    SymbolTable* symtab;
};

struct TryStmt : Stmt
{
    TryStmt(Stmt* tryBlock, Stmt* catchBlock, Id* caughtVar, CatchIsBlock* cisb, Stmt* elsebody)
            : Stmt(Stmt::STMT_try),
            tryBlock(tryBlock),
            catchBlock(catchBlock),
            caughtVar(caughtVar),
            symbol(0),
            tryTab(0),
            catchTab(0),
            catchis(cisb),
            elseblock(elsebody) {}
            
    virtual ~TryStmt();
    
    virtual void DoStmtResources(SymbolTable* st, CompileState& cs);
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
    LabeledStmt(Id* id, Stmt* stmt)
            : Stmt(Stmt::STMT_labeled),
            id(id),
            label(0),
            stmt(stmt)
    {}
    
    virtual void DoStmtResources(SymbolTable* st, CompileState& cs);
    virtual Instr* DoStmtCodeGen();
    
    Id* id;
    Symbol* label;
    Stmt* stmt;
};

// LoopingStmt /////////////////////////////////////////////////////////////////////////////////////

struct LoopingStmt : Stmt
{
    LoopingStmt(Stmt::Kind k) : Stmt(k), label(0) {}
    
    virtual bool IsLoop() const { return true; }
    
    Symbol* label;
};

// BreakStmt ///////////////////////////////////////////////////////////////////////////////////////

struct BreakStmt : Stmt
{
    BreakStmt(Id *id)
            : Stmt(Stmt::STMT_break),
            label(0),
            id(id)
    {}
    
    virtual void DoStmtResources(SymbolTable* st, CompileState& cs);
    virtual Instr* DoStmtCodeGen();
    
    Symbol* label;
    Id* id;
};

// ContinueStmt ////////////////////////////////////////////////////////////////////////////////////

struct ContinueStmt : Stmt
{
    ContinueStmt(Id *id)
            : Stmt(Stmt::STMT_continue),
            label(0),
            id(id)
    {}
    
    virtual void DoStmtResources(SymbolTable* st, CompileState& cs);
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
    };
    
    Expr(Kind kind) : kind(kind) {}
    
    Kind kind;
};

struct ParenExpr : Expr
{
    ParenExpr(Expr* e) : Expr(Expr::EXPR_paren), expr(e) {}
    virtual ~ParenExpr();
    
    virtual void   CalculateResources(SymbolTable* st, CompileState& cs);
    virtual Instr* GenerateCode();
    
    Expr* expr;
};

// PropertyDecl ////////////////////////////////////////////////////////////////////////////////////

struct PropertyDecl : NamedTarget
{
    PropertyDecl(NameNode* name, Expr* get_expr, Expr* set_expr, StorageKind sk)
            : NamedTarget(name, Decl::DECL_property, sk),
            name_expr(0),
            getter(get_expr),
            setter(set_expr)
    {}
    
    virtual ~PropertyDecl();
    
    virtual void CalculateResources(SymbolTable* st, CompileState& cs);
    
    
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
    
    LoadExpr(LoadKind k) : Expr(Expr::EXPR_load), loadkind(k) {}
    
    virtual void CalculateResources(SymbolTable* st, CompileState& cs);
    virtual Instr* GenerateCode();
    
    LoadKind loadkind;
};

////////////////////////////////////////////// CallExpr ////////////////////////////////////////////

struct CallExpr : Expr
{
    CallExpr(Expr *left, ExprList *args)
            : Expr(Expr::EXPR_call),
            left(left),
            args(args), retc(1), redirectedcall(0) {}
            
    virtual void CalculateResources(SymbolTable* st, CompileState& cs);
    virtual Instr* GenerateCode();
    
    bool IsNewCall();
    
    Expr* left;
    ExprList* args;
    u2 retc;
    u1 redirectedcall;
};


////////////////////////////////////////////// CondExpr ////////////////////////////////////////////

struct CondExpr : Expr
{
    CondExpr(Expr* cond, Expr* a, Expr* b, bool unless = false)
            : Expr(Expr::EXPR_conditional),
            cond(cond),
            exprA(a),
            exprB(b),
            unless(unless) {}
            
    virtual void CalculateResources(SymbolTable* st, CompileState& cs);
    virtual Instr* GenerateCode();
    
    Expr* cond;
    Expr* exprA;
    Expr* exprB;
    bool  unless;
};

/////////////////////////////////////////// NullSelectExpr /////////////////////////////////////////

struct NullSelectExpr : Expr
{
    NullSelectExpr(Expr* a, Expr* b) : Expr(Expr::EXPR_nullselect), exprA(a), exprB(b) {}
    
    virtual void CalculateResources(SymbolTable* st, CompileState& cs);
    virtual Instr* GenerateCode();
    
    Expr* exprA;
    Expr* exprB;
};

///////////////////////////////////////////// SliceExpr ////////////////////////////////////////////

struct SliceExpr : Expr
{
    SliceExpr(Expr* expr, Expr* from, Expr* to)
            : Expr(Expr::EXPR_slice),
            expr(expr),
            from(from),
            to(to),
            slicefun(0)
    {}
    
    virtual ~SliceExpr();
    
    virtual void CalculateResources(SymbolTable* st, CompileState& cs);
    virtual Instr* GenerateCode();
    
    Expr* expr;
    Expr* from;
    Expr* to;
    MemberExpr* slicefun;
};

////////////////////////////////////////////// StmtList ////////////////////////////////////////////

struct StmtList : Stmt
{
    StmtList(Stmt *first, Stmt *second)
            : Stmt(Stmt::STMT_list),
            first(first),
            second(second) {}
            
    virtual void DoStmtResources(SymbolTable* st, CompileState& cs)
    {
        if (first) first->CalculateResources(st, cs);
        if (second) second->CalculateResources(st, cs);
    }
    
    virtual Instr* DoStmtCodeGen();
    
    Stmt* first;
    Stmt* second;
};

///////////////////////////////////////////// EmptyStmt ////////////////////////////////////////////

struct EmptyStmt : Stmt
{
    EmptyStmt() : Stmt(Stmt::STMT_empty) {}
    
    virtual void DoStmtResources(SymbolTable* st, CompileState& cs) {}
    Instr* DoStmtCodeGen();
};

////////////////////////////////////////////// ExprStmt ////////////////////////////////////////////

struct ExprStmt : Stmt
{
    ExprStmt(ExprList* expr) : Stmt(Stmt::STMT_expr), exprList(expr), autopop(true) {}
    
    virtual void DoStmtResources(SymbolTable* st, CompileState& cs);
    virtual Instr* DoStmtCodeGen();
    
    ExprList* exprList;
    bool autopop;
};

////////////////////////////////////////////// RetStmt /////////////////////////////////////////////

struct RetStmt : Stmt
{
    RetStmt(ExprList* el) : Stmt(Stmt::STMT_return), exprs(el), count(0), isInTry(false) {}
    
    virtual void    DoStmtResources(SymbolTable* st, CompileState& cs);
    virtual Instr*  DoStmtCodeGen();
    
    ExprList* exprs;
    size_t    count;
    bool isInTry;
};

///////////////////////////////////////////// YieldStmt ////////////////////////////////////////////

struct YieldStmt : Stmt
{
    YieldStmt(ExprList* el) : Stmt(Stmt::STMT_yield), exprs(el), count(0) {}
    
    virtual void DoStmtResources(SymbolTable* st, CompileState& cs);
    virtual Instr* DoStmtCodeGen();
    
    ExprList* exprs;
    size_t    count;
};

////////////////////////////////////////////// LoopStmt ////////////////////////////////////////////

struct LoopStmt : LoopingStmt
{
    LoopStmt(Stmt *body) : LoopingStmt(Stmt::STMT_loop), body(body) {}
    
    virtual void DoStmtResources(SymbolTable* st, CompileState& cs);
    virtual Instr* DoStmtCodeGen();
    
    Stmt* body;
};

//////////////////////////////////////////// CondLoopStmt //////////////////////////////////////////

struct CondLoopStmt : LoopingStmt
{
    CondLoopStmt(Expr* cond, Stmt* body, bool repeat, bool until)
            : LoopingStmt(Stmt::STMT_while),
            cond(cond),
            body(body),
            repeat(repeat),
            until(until) {}
            
    virtual void DoStmtResources(SymbolTable* st, CompileState& cs);
    virtual Instr* DoStmtCodeGen();
    
    Expr* cond;
    Stmt* body;
    bool repeat;
    bool until;
};

///////////////////////////////////////////// ForToStmt ////////////////////////////////////////////

struct ForToStmt : LoopingStmt
{
    ForToStmt(Id* id, Expr* from, Expr* to, Expr* step, Stmt* body, bool down)
            : LoopingStmt(Stmt::STMT_for),
            id(id),
            from(from),
            to(to),
            step(step),
            body(body),
            down(down),
            symbol(0),
            symtab(0) {}
    
    virtual ~ForToStmt();
    
    virtual void   DoStmtResources(SymbolTable* st, CompileState& cs);
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
    ForeachStmt(Id* id, Expr* type_expr, Expr* in, Stmt* body)
            : LoopingStmt(Stmt::STMT_foreach),
            id(id),
            iterVar(0),
            type_expr(type_expr),
            in(in),
            body(body),
            idexpr(0),
            enum_offset(0),
            symtab(0) {}
    
    virtual ~ForeachStmt();
    
    virtual void   DoStmtResources(SymbolTable* st, CompileState& cs);
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
    IfStmt(Expr* cond, Stmt* then_part, bool unless = false)
            : Stmt(Stmt::STMT_if),
            next(0),
            cond(cond),
            then_part(then_part),
            is_unless(unless) {}
            
    virtual void DoStmtResources(SymbolTable* st, CompileState& cs);
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
    ConditionalStmt(IfStmt* ifpart, Stmt* elsepart)
            : Stmt(Stmt::STMT_if),
            ifPart(ifpart),
            elsePart(elsepart) {}
            
    virtual void DoStmtResources(SymbolTable* st, CompileState& cs)
    {
        if (ifPart) ifPart->CalculateResources(st, cs);
        if (elsePart) elsePart->CalculateResources(st, cs);
    }
    
    virtual Instr* DoStmtCodeGen();
    
    IfStmt* ifPart;
    Stmt* elsePart;
};

///////////////////////////////////////////// BlockStmt ////////////////////////////////////////////

struct BlockStmt : Stmt
{
    BlockStmt(Stmt* stmts)
            : Stmt(Stmt::STMT_block),
            stmts(stmts),
            symtab(0) {}
            
    virtual ~BlockStmt();
    virtual void DoStmtResources(SymbolTable* st, CompileState& cs);
    virtual Instr* DoStmtCodeGen();
    
    Stmt* stmts;
    SymbolTable* symtab;
};

////////////////////////////////////////////// DeclStmt ////////////////////////////////////////////

struct DeclStmt : Stmt
{
    DeclStmt(Decl *decl);
    
    virtual void DoStmtResources(SymbolTable* st, CompileState& cs);
    
    virtual Instr* DoStmtCodeGen();
    
    Decl* decl;
};

///////////////////////////////////////////// UnaryExpr ////////////////////////////////////////////

struct UnaryExpr : Expr
{
    UnaryExpr(Expr::Kind k, Expr* expr)
            : Expr(k),
            expr(expr) {}
            
    virtual void   CalculateResources(SymbolTable* st, CompileState& cs);
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
    BinaryExpr(Expr::Kind k, Expr* left, Expr* right)
            : Expr(k),
            left(left),
            right(right)
    {}
    
    virtual void CalculateResources(SymbolTable* st, CompileState& cs);
    
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
    IdExpr(Id* id)
            : Expr(Expr::EXPR_identifier),
            depthindex(0),
            index(0),
            id(id),
            symbol(0),
            outer(false),
            inWithBlock(false), outerwith(false), newLocal(false)
    {}
    
    virtual void CalculateResources(SymbolTable* st, CompileState& cs);
    
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
    ConstantExpr(Expr::Kind k)
            : Expr(k),
            index(0) {}
            
    virtual Instr* GenerateCode();
    
    u2 index;
};

////////////////////////////////////////////// FunExpr /////////////////////////////////////////////

struct FunExpr : ConstantExpr
{
    FunExpr(ParamDecl* args, Stmt* body, size_t begtxt, size_t endtxt, Id* name = 0)
    : ConstantExpr(Expr::EXPR_function),
    args(args),
    name(name),
    body(body),
    def(0),
    symtab(0),
    scriptBeg(begtxt),
    scriptEnd(endtxt) {}
            
    virtual ~FunExpr();
    
    virtual void CalculateResources(SymbolTable* st, CompileState& cs);
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
    PropExpr(Expr* name, Expr* getfn, Expr* setfn)
    : Expr(Expr::EXPR_property),
    nameexpr(name),
    getter(getfn),
    setter(setfn) {}
    
    virtual void CalculateResources(SymbolTable* st, CompileState& cs);
    virtual Instr* GenerateCode();
    
    Expr* nameexpr;
    Expr* getter;
    Expr* setter;
};

///////////////////////////////////////////// MemberExpr ///////////////////////////////////////////

struct MemberExpr : ConstantExpr
{
    MemberExpr(Id *id)
            : ConstantExpr(Expr::EXPR_member),
            id(id) {}
            
    virtual void CalculateResources(SymbolTable* st, CompileState& cs);
    
    Id* id;
};

///////////////////////////////////////////// StringExpr ///////////////////////////////////////////

struct StringExpr : ConstantExpr
{
    StringExpr(char *string, size_t len)
            : ConstantExpr(Expr::EXPR_string),
            string(string),
            length(len) {}
            
    virtual ~StringExpr();
    virtual void CalculateResources(SymbolTable* st, CompileState& cs);
    
    char* string;
    size_t length;
};

//////////////////////////////////////////// IntegerExpr ///////////////////////////////////////////

struct IntegerExpr : ConstantExpr
{
    IntegerExpr(pint_t integer)
            : ConstantExpr(Expr::EXPR_integer),
            integer(integer) {}
            
    virtual void CalculateResources(SymbolTable* st, CompileState& cs);
    
    pint_t integer;
};

////////////////////////////////////////////// RealExpr ////////////////////////////////////////////

struct RealExpr : ConstantExpr
{
    RealExpr(preal_t real)
            : ConstantExpr(Expr::EXPR_real),
            real(real) {}
            
    virtual void CalculateResources(SymbolTable* st, CompileState& cs);
    
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
    DotExpr(Expr*, Expr*);
    
    virtual void    CalculateResources(SymbolTable* st, CompileState& cs);
    virtual Instr*  GenerateCode();
    virtual Instr*  GenerateCodeSet();
    virtual Opcode  GetOpcode() const { return OP_dotget; }
};

/** A binary expression where the right hand expression (rhe) is enclosed in square brackets and placed in a post-fix position
  * relative to the left hand expression (lhe). The rhe specifies the value to get/set from the lhe.
  *     ie  foo['bar']
  *         foo.bar['bazz']
  */
struct IndexExpr : DotExpr
{
    IndexExpr(Expr* l, Expr* r) : DotExpr(l, r)
    {
    }
    virtual void    CalculateResources(SymbolTable* st, CompileState& cs);
    virtual Instr*  GenerateCode();
    virtual Instr*  GenerateCodeSet();    
    virtual Opcode GetOpcode() const { return OP_subget; }
};

/** A dot expression in which the value being retrieved and the object it belongs to are bound together as one value.
  *     ie  bind foo.bar
  *         bind foo.bar.bazz
  */
struct DotBindExpr : DotExpr
{
    DotBindExpr(Expr *l, Expr *r) : DotExpr(l, r) { kind = Expr::EXPR_dotbind; }
    
    virtual Instr* GenerateCode();
    virtual Instr* GenerateCodeSet();
};

/** A list of (key, value) pairs used as the elements of an object literal. */
struct FieldList : TreeNode
{
    FieldList(Expr* name, Expr* value) : dtarget(0), name(name), value(value), next(0) {}
    
    FieldList(DeclarationTarget* dt) : dtarget(dt), name(0), value(0), next(0) {}
    
    virtual void CalculateResources(SymbolTable* st, CompileState& cs)
    {
        if (dtarget)
        {
            dtarget->CalculateResources(st, cs);
        }
        else
        {
            if (name) name ->CalculateResources(st, cs);
            if (value) value->CalculateResources(st, cs);
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
    DictionaryExpr(FieldList* fields) : Expr(Expr::EXPR_dictionary), type_expr(0), fields(fields) {}
    
    virtual void    CalculateResources(SymbolTable* st, CompileState& cs);
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
    ArrayExpr(ExprList* elements) : Expr(Expr::EXPR_array), numElements(0), elements(elements) {}
    
    virtual void CalculateResources(SymbolTable* st, CompileState& cs);
    virtual Instr* GenerateCode();
    
    size_t      numElements;    //!< Length of the array.
    ExprList*   elements;       //!< elements of the array.
};

// ExprList ///////////////////////////////////////////////////////////////////////////////////////

struct ExprList : TreeNode
{
    ExprList(Expr* e) : expr(e), next(0) {}
    
    virtual void CalculateResources(SymbolTable* st, CompileState& cs);
    
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
        MOD_ASSIGN_STMT,
        CONCAT_SPACE_ASSIGN_STMT,
        CONCAT_ASSIGN_STMT,
    };
    
    AssignmentStmt(ExprList* l, ExprList* r);
    
    virtual void DoStmtResources(SymbolTable* st, CompileState& cs);
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

/////////////////////////////////////////// WithStatement //////////////////////////////////////////

struct WithStatement : Stmt
{
    WithStatement(Expr* e, Stmt* b);
    
    virtual ~WithStatement();
    
    virtual void DoStmtResources(SymbolTable* st, CompileState& cs);
    virtual Instr* DoHeader();
    virtual Instr* DoStmtCodeGen();
    
    Expr*           with;
    Stmt*           block;
    SymbolTable*    symtab;
};

////////////////////////////////////////////// PkgDecl /////////////////////////////////////////////

struct PkgDecl : NamedTarget
{
    PkgDecl(NameNode*, Stmt* body, StorageKind sto);
    
    virtual ~PkgDecl();
    virtual void CalculateResources(SymbolTable* st, CompileState& cs);
    virtual Instr* GenerateCode();
    
    SymbolTable*    symtab;
    StringExpr*     id;
    Stmt*           body;
};

////////////////////////////////////////////// CaseList ////////////////////////////////////////////

struct CaseList : TreeNode, TLinked<CaseList>
{
    CaseList(ExprList* matches, Stmt* body);
    
    virtual void DoResources(SymbolTable* st, CompileState& cs);
    
    ExprList*   matches;
    Stmt*       body;
};

////////////////////////////////////////////// CaseStmt ////////////////////////////////////////////

struct CaseStmt : Stmt
{
    CaseStmt(Expr* selector, CaseList* cases, Stmt* elsebody);
    
    virtual void    DoStmtResources(SymbolTable* st, CompileState& cs);    
    virtual Instr*  DoStmtCodeGen();
    
    Expr*       selector;
    CaseList*   cases;
    Stmt*       elsebody;
};

////////////////////////////////////////////// NameNode ////////////////////////////////////////////

struct NameNode : TreeNode
{
    NameNode(IdExpr* idexpr) : idexpr(idexpr), dotexpr(0) {}
    
    NameNode(DotExpr* dotexpr) : idexpr(0), dotexpr(dotexpr) {}
    
    virtual ~NameNode() {}
    
    virtual void    CalculateResources(SymbolTable* st, CompileState& cs);
    virtual Instr*  GenerateCode();
    virtual Instr*  GenerateCodeSet();
    const char*     GetName();
    
    Expr* GetSelfExpr() { return dotexpr ? dotexpr->left : 0; }
    
    IdExpr*     idexpr;
    DotExpr*    dotexpr;
};

struct DeleteStmt : Stmt, TLinked<DeleteStmt>
{
    DeleteStmt(DotExpr* e) : Stmt(Stmt::STMT_delete), expr(e) {}
    
    virtual void DoStmtResources(SymbolTable* st, CompileState& cs)
    {
        if (expr)
        {
            expr->CalculateResources(st, cs);
        }
        if (next) next->CalculateResources(st, cs);
    }
    
    virtual Instr* DoStmtCodeGen();
    
    DotExpr* expr;
};

struct ClassDecl : NamedTarget
{
    ClassDecl(NameNode* id, Expr* super, Stmt* stmts, StorageKind sk)
            : NamedTarget(id, Decl::DECL_class, sk),
            stringid(0),
            super(super),
            stmts(stmts),
            symtab(0)
    {}
    
    virtual ~ClassDecl();
    
    virtual void    CalculateResources(SymbolTable* st, CompileState& cs);
    virtual Instr*  GenerateCode();
    
    StringExpr*     stringid;
    Expr*           super;
    Stmt*           stmts;
    SymbolTable*    symtab;
};

}// pika

#endif
