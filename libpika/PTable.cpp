/*
 *  PTable.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PConfig.h"
#include "PUtil.h"
#include "PMemory.h"
#include "PValue.h"
#include "PBasic.h"
#include "PString.h"
#include "PTable.h"

#ifdef  PIKA_USE_SLOT_POOL
#define PIKA_SLOT_POOL_SIZE 2048

#include "PMemPool.h"

namespace pika {
static MemObjPool<Slot> PropertyPool(PIKA_SLOT_POOL_SIZE);
}// pika

#endif

namespace pika {
#if defined(PIKA_32)
INLINE size_t Pika_Hash64(const u8 X) { return (size_t)X + ((size_t)(X >> 32) * 23); }
INLINE size_t Pika_Hash32(const u4 X) { return (size_t)X; }
#else
INLINE size_t Pika_Hash64(const u8 X) { return (size_t)X; }
INLINE size_t Pika_Hash32(const u4 X) { return (size_t)X; }
#endif

INLINE size_t Pika_HashValue(const Value& v)
{
    switch (v.tag)
    {
    case TAG_null:      return v.val.index;
    case TAG_boolean:   return v.val.index;

#if defined(PIKA_64BIT_INT)
    case TAG_integer:   Pika_Hash64(*((u8*)&v.val.integer));
#else
    case TAG_integer:   Pika_Hash32(*((u4*)&v.val.integer));
#endif

#if defined(PIKA_64BIT_REAL)
    case TAG_real:      Pika_Hash64(*((u8*)&v.val.real));
#else
    case TAG_real:      Pika_Hash32(*((u4*)&v.val.real));
#endif

    case TAG_index:     return v.val.index;
    case TAG_string:    return v.val.str->GetHashCode();
    }
    return v.val.index >> 3;
}

size_t const Table::MAX_TABLE_SLOTS = GetMaxSize<Slot>();
size_t const Table::MAX_TABLE_SIZE  = GetMaxSize<Slot>() / 2;

Table::Table()
        : slots(0),
        count(0),
        size(8)
{
    slots = (Slot**)Pika_calloc(size, sizeof(Slot*));
    count = 0;
}

Table::Table(const Table& other)
        : slots(0),
        count(0),
        size(other.size)
{
    slots = (Slot**)Pika_calloc(size, sizeof(Slot*));
    count = other.count;

    for (size_t i = 0; i < size; ++i)
    {
        Slot* p = other.slots[i];

        while (p)
        {
#ifdef PIKA_USE_SLOT_POOL
            void* v = PropertyPool.RawAlloc();
            Slot* temp = new(v) Slot(p->key, p->val, p->attr);
#else
            Slot *temp = 0;
            PIKA_NEW(Slot, temp, (p->key, p->val, p->attr));
#endif
            temp->next = slots[i];
            slots[i] = temp;

            p = p->next;
        }
    }
}

Table::~Table()
{
    Clear();
    Pika_free(slots);
}

void Table::Grow()
{
    size_t nsize = size << 1;

    if (nsize < size ||             // Addition over-flow or
        nsize >= MAX_TABLE_SIZE)    // Bigger than max size allowed
    {
        nsize = MAX_TABLE_SIZE;
    }

    size_t oldSize  = size;
    Slot** oldNodes = slots;

    size  = nsize;
    slots = (Slot**)Pika_calloc(size, sizeof(Slot*));

    if (!slots)
    {
        size  = oldSize;
        slots = oldNodes;

        RaiseException("Table::Grow memory allocation failed.");
    }

    for (size_t i = 0; i < oldSize; i++)
    {
        Slot* current = oldNodes[i];

        while (current)
        {

            Slot* next = current->next;

            size_t hashcode = Pika_HashValue(current->key) & (size - 1);

            current->next = slots[hashcode];
            slots[hashcode] = current;

            current = next;
        }
    }
    Pika_free(oldNodes);
}

bool Table::Set(const Value& key, Value& value, u4 attr)
{
    size_t hashcode = Pika_HashValue(key) & (size - 1);
    Slot*  current  = slots[hashcode];

    while (current)
    {
        if (key == current->key)
        {
            current->val = value;
            return true;
        }
        current = current->next;
    }

    /* If adding another element will exceed the max size allowed. */

    if ((count + 1) >= MAX_TABLE_SLOTS)
    {
        RaiseException("Table::Set max number of slots reached.");
    }

    /* If the load ratio is greater than 2:1 and
       the table size is not maxed out. */
    if ((count > (size << 1)) && (size  < MAX_TABLE_SIZE))
    {
        Grow();
        hashcode = Pika_HashValue(key) & (size - 1);
    }
#ifdef PIKA_USE_SLOT_POOL
    void* v = PropertyPool.RawAlloc();
    Slot* temp = new(v) Slot(key, value, attr);
#else
    Slot* temp = 0;
    PIKA_NEW(Slot, temp, (key, value, attr));
#endif

    temp->next = slots[hashcode];
    slots[hashcode] = temp;
    ++count;

    return true;
}

Table::ESlotState Table::CanSet(const Value& key)
{
    size_t hashcode = Pika_HashValue(key) & (size - 1);
    Slot* current = slots[hashcode];

    while (current)
    {
        if (key == current->key)
        {
            return (current->val.tag != TAG_property) && !(current->attr & Slot::ATTR_final) ? SS_yes : SS_no;
        }
        current = current->next;
    }
    return SS_nil;
}

bool Table::CanInherit(const Value& key)
{
    size_t hashcode = Pika_HashValue(key) & (size - 1);
    Slot* current = slots[hashcode];

    while (current)
    {
        if (key == current->key)
        {
            return (current->val.tag != TAG_property) && (!(current->attr & Slot::ATTR_final) || (current->attr & Slot::ATTR_virtual));
        }
        current = current->next;
    }
    return true;
}

bool Table::Exists(const Value& key)
{
    size_t hashcode = Pika_HashValue(key) & (size - 1);
    Slot *current = slots[hashcode];

    while (current)
    {
        if (key == current->key)
            return true;
        current = current->next;
    }
    return false;
}

bool Table::Get(const Value& key, Value& res)
{
    size_t hashcode = Pika_HashValue(key) & (size - 1);
    const Slot* current = slots[hashcode];

    while (current)
    {
        if (key == current->key)
        {
            res = current->val;
            return true;
        }
        current = current->next;
    }

    return false;
}

bool Table::Remove(const Value& key)
{
    size_t hashcode = Pika_HashValue(key) & (size - 1);
    Slot** ptr_to   = &(slots[hashcode]);
    Slot*  current  = slots[hashcode];

    while (current)
    {
        if (key == current->key)
        {
            if (current->attr & Slot::ATTR_nodelete)
                return false;
            *ptr_to = current->next;

#ifdef PIKA_USE_SLOT_POOL
            PropertyPool.Delete(current);
#else
            Pika_delete<Slot>(current);
#endif

            count--;
            return true;
        }
        ptr_to = &(current->next);
        current = current->next;
    }
    return false;
}

void Table::DoMark(Collector* c)
{
    if (count == 0) return;

    for (size_t b = 0; b < size ; ++b)
    {
        Slot* e = slots[b];

        while (e)
        {
            MarkValue(c, e->key);
            MarkValue(c, e->val);
            e = e->next;
        }
    }
}

void Table::Clear()
{
    for (size_t i = 0; i < size; ++i)
    {
        Slot* curr = slots[i];

        while (curr)
        {
            Slot* next = curr->next;

#ifdef PIKA_USE_SLOT_POOL
            PropertyPool.Delete(curr);
#else
            Pika_delete<Slot>(curr);
#endif
            curr = next;
        }
        slots[i] = 0;
    }
    count = 0;
}

}// pika
