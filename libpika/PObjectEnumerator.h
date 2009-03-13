/*
 *  PObjectEnumerator.h
 *  See Copyright Notice in Pika.h
 */
#ifndef POBJECTENUMERATOR_H
#define POBJECTENUMERATOR_H

#ifndef PIKA_ENUMERATOR_HEADER
#   include "PEnumerator.h"
#endif

namespace pika {
class ObjectEnumerator : public Enumerator
{
public:
    ObjectEnumerator(Engine* eng, bool values, Basic* obj, Table& tab)
            : Enumerator(eng), started(false), bValues(values), owner(obj), table(tab) {res.SetNull();}
            
    virtual ~ObjectEnumerator() {}
    
    virtual bool FilterValue(Value& key, Basic* obj) { return true; }
        
    virtual bool Rewind()
    {
        started = true;
        Slot* curr = 0;
        size_t bin = 0;
        size_t sz  = table.size; // size of table.slots[]
        size_t count = table.count; // # of elements in table
        if (owner && sz)
        {
            // Build an array of keys so that any changes made to the slot table
            // do not effect us.
            
            slots.Resize(count);
            size_t s = 0;
            for (bin = 0; bin < sz; ++bin)
            {
                curr = table.slots[bin];
                
                while (curr)
                {
                    if (!(curr->attr & Slot::ATTR_noenum))
                    {
                        ASSERT(s < count);
                        slots[s++] = curr->key;
                    }
                    curr = curr->next;
                }
            }
            if (s < count)
                slots.Resize(s);
            currSlot = slots.Begin();
            MoveCurrent();
            return true;
        }
        else
        {
            currSlot = slots.End();
        }
        return false;
    }
    
    virtual bool IsValid()
    {
        return started && (currSlot < slots.End());
    }
    
    virtual void GetCurrent(Value& c)
    {
        c = res;
    }
    
    virtual void MoveCurrent()
    {
        if (IsValid())
        {            
            if (table.Get(*currSlot, res) && FilterValue(res, owner))
            {
                if (!bValues)
                {
                    res = *currSlot;
                }
                return;
            }
            else
            {
                // Key was deleted while we were enumerating, move on to the next one.
                Advance();
                if (IsValid())
                {
                    return MoveCurrent();
                }
                else
                {                    
                    return;
                }
            }
        }
        return;
    }
    
    virtual void Advance()
    {
        res.SetNull();
        if (owner)
        {
            if (!started)
            {
                return;
            }
            
            if (!(++currSlot < slots.End()))
            {
                currSlot = slots.End();
            }
            MoveCurrent();
        }
    }
    
    virtual void MarkRefs(Collector* c)
    {
        if (owner)
            owner->Mark(c);
            
        Buffer<Value>::Iterator end = slots.End();
        
        for (Buffer<Value>::Iterator i = slots.Begin(); i != end; ++i)
        {
            MarkValue(c, *i);
        }
    }
private:    
    bool started;
    bool bValues;
    Value res;
    Basic* owner;
    Buffer<Value> slots;
    Buffer<Value>::Iterator currSlot;
    Table& table;
};

}// pika

#endif
