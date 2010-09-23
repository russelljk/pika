/*
 *  PSymbolTable.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_SYMBOLTABLE_HEADER
#define PIKA_SYMBOLTABLE_HEADER

namespace pika {
class Engine;
class SymbolTable;

struct Symbol
{
    Symbol(const char* n, SymbolTable* tab, bool g, bool  w)
            : name(n), 
            table(tab), 
            isGlobal(g), 
            isWith(w), 
            offset(-1), 
            next(0)
    {}

    ~Symbol() {}

    const char*  name;  //!< Variable name
    SymbolTable* table; //!< SymbolTable we are defined in.
    bool isGlobal;      //!< Global symbol.
    bool isWith;        //!< Member symbol.
    int offset;         //!< Local variable offset, -1 otherwise.
    Symbol* next;       //!< Next symbol in the SymbolTable.
};

enum SymbolType
{
    ST_main     = PIKA_BITFLAG(0),
    ST_package  = PIKA_BITFLAG(1),
    ST_function = PIKA_BITFLAG(2),
    ST_using    = PIKA_BITFLAG(3),
    ST_noinherit= PIKA_BITFLAG(4),
};

class SymbolTable
{
public:
    enum { HASHSIZE = 256 };

    SymbolTable(SymbolTable* parent,
                u4 flags = 0); // Inherits parent's scope (currently false only for catch & finally blocks).
    
    ~SymbolTable();
    
    Symbol* Shadow(const char* name);
    Symbol* Put(const char* name);
    Symbol* Get(const char* name);

    bool IsDefined(const char* name);
    
    bool VarsAreGlobal() const { return fPackage || fUsing; }
    bool DefaultsToGlobal() const { return fMain || fPackage || fUsing; }
    
    bool GetWith() { return fUsing; }

    bool IsWithBlock() const
    {
        if (fUsing || fNoInherit)
            return fUsing;
        return parent ? parent->IsWithBlock() : false;
    }

    int GetDepth() const { return depth; }

    int IncrementDepth();

    void Dump();
    void Dump(pika::Engine*, class Table* tab);
private:
    int depth;        
    SymbolTable* parent;
    Symbol* table[HASHSIZE];
    u4 fMain:1;
    u4 fPackage:1;
    u4 fFunction:1;
    u4 fUsing:1;
    u4 fNoInherit:1;
};
}// pika

#endif
