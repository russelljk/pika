/*
 *  PSymboltable.cpp
 *  See Copyright Notice in Pika.h
 */

#include "Pika.h"                // TODO: remove me?
#include "PSymbolTable.h"
#include "PAst.h"
#include "PMemory.h"
#include "PPlatform.h"

namespace pika {

SymbolTable::SymbolTable(SymbolTable* parent, bool global, bool with, bool isfunc, bool inherit)
        : defaultGlobal(global),
        isWith(with),
        isFunction(isfunc),
        inherit(inherit),
        parent(parent)
{
    Pika_memzero(table, sizeof(Symbol*) * HASHSIZE);
    depth = (parent) ? parent->depth : 0;
}

SymbolTable::~SymbolTable()
{
    for (size_t i = 0; i < HASHSIZE; i++)
    {
        Symbol* curr = 0;
        Symbol* next = 0;

        curr = table[i];

        while (curr)
        {
            next = curr->next;
            Pika_delete<Symbol>(curr);
            curr = next;
        }
    }
}

Symbol* SymbolTable::Shadow(const char* name)
{
    size_t  i = Pika_strhash(name) & (HASHSIZE - 1);
    Symbol* s = 0;

    PIKA_NEW(Symbol, s, (name, this, defaultGlobal, IsWithBlock()));

    s->next  = table[i];
    table[i] = s;

    return s;
}

Symbol* SymbolTable::Put(const char* name)
{
    size_t  i = 0;
    Symbol* s = 0;

    i = Pika_strhash(name) & (HASHSIZE - 1);

    for (s = table[i]; s; s = s->next)
    {
        if (StrCmp(s->name, name) == 0)
            return s;
    }

    s = 0;
    PIKA_NEW(Symbol, s, (name, this, defaultGlobal, IsWithBlock()));

    s->next = table[i];
    table[i] = s;
    return s;
}

Symbol* SymbolTable::Get(const char* name)
{
    size_t  i = 0;
    Symbol* s = 0;

    i = Pika_strhash(name) & (HASHSIZE - 1);

    for (s = table[i]; s; s = s->next)
    {
        if (StrCmp(s->name, name) == 0)
        {            
            return s;
        }
    }

    return (parent) ? parent->Get(name) : 0;
}

bool SymbolTable::IsDefined(const char* name)
{
    size_t  i = 0;
    Symbol* s = 0;

    i = Pika_strhash(name) & (HASHSIZE - 1);

    for (s = table[i]; s; s = s->next)
    {
        if (StrCmp(s->name, name) == 0)
        {
            return true;
        }
    }
    return false;
}

void SymbolTable::Dump(Engine* eng, Table* tab)
{
    for (size_t i = 0; i < HASHSIZE; ++i)
    {
        Symbol* curr = table[i];

        while (curr)
        {
            if (curr->offset != -1)
            {
                String* strname = eng->AllocString(curr->name);
                pint_t    offset = curr->offset;

                Value vname(strname);
                Value voffset(offset);

                tab->Set(vname, voffset);
            }
            curr = curr->next;
        }
    }
}

void SymbolTable::Dump()
{
    fprintf(stderr, "\nSymbols:\n");
    fprintf(stderr, "******************************\n");

    for (size_t i = 0; i < HASHSIZE; ++i)
    {
        Symbol* curr = table[i];

        while (curr)
        {
            if (curr->offset != -1)
            {
                fprintf(stderr, "%s : %d\n", curr->name, curr->offset);
            }
            curr = curr->next;
        }
    }
    fprintf(stderr, "******************************\n");
}

int SymbolTable::IncrementDepth() { return ++depth; }

}// pika
