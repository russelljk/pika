/*
 *  PStringtable.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PStringTable.h"
#include "PPlatform.h"

namespace pika
{

PIKA_FORCE_INLINE bool Pika_strsame(const char* a, size_t lena, const char* b, size_t lenb)
{
    return lena == lenb && StrCmpWithSize(a, b, lena) == 0;
}

StringTable::StringTable(Engine* eng) : engine(eng)
{
    Clear();
}

StringTable::~StringTable()
{
    SweepAll();
}

void StringTable::Clear()
{
    Pika_memzero(Slots, sizeof(String*) * ENTRY_SIZE);
}

void StringTable::SweepAll()
{
    for (size_t i = 0; i < ENTRY_SIZE; i++)
    {
        String* curr = 0;
        String* next = 0;

        curr = Slots[i];

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
    for (size_t i = 0; i < ENTRY_SIZE; i++)
    {
        String*  curr =  Slots[i];
        String** ptrto = &Slots[i];

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
    return Get( cstr, strlen( cstr ) );
}

String* StringTable::Get(const char* cstr, size_t len)
{
    if ( len > PIKA_STRING_MAX_LEN )
    {
        RaiseException("Attempt to create a string of length "SIZE_T_FMT" (max string length %d).", len, PIKA_STRING_MAX_LEN);
    }

    size_t i;
    String* s;
    i = Pika_strhash(cstr, len) & (ENTRY_SIZE - 1);

    for (s = Slots[i]; s; s = (String*)s->next)
    {
        if (Pika_strsame(cstr, len, s->buffer, s->length))
        {
            if (s->gcflags & GCObject::ReadyToCollect) // Is it a dead string?
                s->gcflags = s->gcflags & ~GCObject::ReadyToCollect; // ... Then revive it.
            return s;
        }
    }

    s = String::Create(engine, cstr, len);
    s->next = Slots[i];
    Slots[i] = s;
    return s;
}

}//pika
