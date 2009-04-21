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

        const char*  name;
        SymbolTable* table;
        bool         isGlobal;
        bool         isWith;
        int          offset;
        Symbol*      next;
    };

    class SymbolTable
    {
    public:
        enum { HASHSIZE = 256 };

        SymbolTable(SymbolTable* parent,
                    bool global  = false, // Variables default to global scope
                    bool with    = false, // Variables default to slot_table
                    bool func    = false, // Default for functions
                    bool inherit = true); // Inherits parent's scope (currently unset only for catch|ensure blocks)

        ~SymbolTable();

        Symbol* Shadow(const char* name);
        Symbol* Put(const char* name);
        Symbol* Get(const char* name);

        bool IsDefined(const char* name);

        bool DefaultsToGlobal() const { return defaultGlobal || isWith; }

        bool GetWith() { return isWith; }

        bool IsWithBlock() const
        {
            if (isFunction)
                return false;
            else if (isWith || !inherit)
                return isWith;
            return parent ? parent->IsWithBlock() : false;
        }

        int GetDepth() const { return depth; }

        int IncrementDepth();

        void Dump();
        void Dump(pika::Engine*, class Table* tab);
        
        int          depth;
        bool         defaultGlobal;
        bool         isWith;
        bool         isFunction;
        bool         inherit;
        SymbolTable* parent;
        Symbol*      table[HASHSIZE];
    };
}// pika

using pika::SymbolTable;
using pika::Symbol;

#endif
