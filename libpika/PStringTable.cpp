/*
 *  PStringtable.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PStringTable.h"
#include "PPlatform.h"

namespace pika {

PIKA_FORCE_INLINE bool Pika_strsame(const char* a, size_t lena, const char* b, size_t lenb)
{
    return lena == lenb && StrCmpWithSize(a, b, lena) == 0;
}

StringTable::StringTable(Engine* eng) : size(ENTRY_SIZE), count(0), engine(eng)
{
    entries = (String**)Pika_calloc(size, sizeof(String*));    
}

StringTable::~StringTable()
{
    if (entries)
    {
        SweepAll();
        Pika_free(entries);
        entries = 0;
    }
}

void StringTable::Clear()
{
    Pika_memzero(entries, sizeof(String*) * size);
}

static size_t const MAX_STRTABLE_SIZE  = GetMaxSize<String*>() / 8;

void StringTable::Grow()
{
    size_t nsize = size << 1;
    
    if (nsize < size ||             // Addition over-flow or
        nsize >= MAX_STRTABLE_SIZE) // Bigger than max size allowed
    {
        nsize = MAX_STRTABLE_SIZE;
    }
    
    size_t oldSize  = size;
    String** oldEntries = entries;
    
    size  = nsize;
    entries = (String**)Pika_calloc(size, sizeof(String*));
    
    if (!entries)
    {
        size  = oldSize;
        entries = oldEntries;

        RaiseException("StringTable::Grow memory allocation failed.");
    }
    
    for (size_t i = 0; i < oldSize; i++)
    {
        String* current = oldEntries[i];
        
        while (current)
        {
            String* next = current->next;
            
            size_t hashcode = current->GetHashCode() & (size - 1);
            
            current->next = entries[hashcode];
            entries[hashcode] = current;
            
            current = next;
        }
    }
    Pika_free(oldEntries);
}

void StringTable::SweepAll()
{
    for (size_t i = 0; i < size; i++)
    {
        String* curr = 0;
        String* next = 0;
        
        curr = entries[i];
        
        while (curr)
        {
            next = curr->next;
            Pika_delete(curr);
            curr = next;
        }
    }
    Clear();
}

void StringTable::Sweep()
{
    for (size_t i = 0; i < size; i++)
    {
        String*  curr =  entries[i];
        String** ptrto = &entries[i];
        
        while (curr)
        {
            if (!(curr->gcflags & GCObject::Persistent) && (curr->gcflags & GCObject::ReadyToCollect))
            {
                *ptrto = curr->next;
                Pika_delete(curr);
                curr = *ptrto;
            }
            else
            {
                ptrto = &curr->next;
                curr = curr->next;
            }
        }
    }
}

String* StringTable::Get(const char *cstr)
{
    return Get(cstr, strlen( cstr ));
}

String* StringTable::Get(const char* cstr, size_t len)
{
    if ( len > PIKA_STRING_MAX_LEN )
    {
        RaiseException("Attempt to create a string of length "SIZE_T_FMT" (max string length %d).", len, PIKA_STRING_MAX_LEN);
    }
    size_t strhash = Pika_strhash(cstr, len);
    size_t hashcode = strhash & (size - 1);
    
    for (String* s = entries[hashcode]; s; s = (String*)s->next)
    {
        if (Pika_strsame(cstr, len, s->buffer, s->length))
        {
            if (s->gcflags & GCObject::ReadyToCollect)               // If its a dead string ...
                s->gcflags = s->gcflags & ~GCObject::ReadyToCollect; // ... then revive it.
            return s;
        }
    }
    
    if ((size < MAX_STRTABLE_SIZE) && (count > (size << 1)))
    {
        Grow();
        hashcode = strhash & (size - 1);
    }
    ++count;
    String* newstr = String::Create(engine, cstr, len);
    newstr->next = entries[hashcode];
    entries[hashcode] = newstr;
    return newstr;
}

}// pika
