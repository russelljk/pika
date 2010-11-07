/*
 *  PObjectIterator.h
 *  See Copyright Notice in Pika.h
 */
#ifndef POBJECTITERATOR_H
#define POBJECTITERATOR_H

namespace pika {

class ObjectIterator : public Iterator
{
public:
    ObjectIterator(Engine* eng, Type* typ, IterateKind k, Object* obj, Table* tab)
            : Iterator(eng, typ), kind(k), valid(false), started(false), owner(obj), table(tab)
    {
        key.SetNull();
        val.SetNull();
        Rewind();
    }
    
    virtual ~ObjectIterator() {}
    
    virtual bool FilterValue(Value& key, Object* obj)
    {
        return true;
    }
    
    virtual bool Rewind()
    {
        if (!table)
            return false;
        started = true;
        Slot* curr = 0;
        size_t bin = 0;
        size_t sz  = table->size; // size of table.slots[]
        size_t count = table->count; // # of elements in table
        if (owner && sz)
        {
            // Build an array of keys so that any changes made to the slot table
            // do not effect us.
            
            slots.Resize(count);
            size_t s = 0;
            for (bin = 0; bin < sz; ++bin)
            {
                curr = table->rows[bin];
                
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
            valid = true;
            MoveCurrent();
            return true;
        }
        else
        {
            currSlot = slots.End();
        }
        return false;
    }
    
    virtual bool ToBoolean()
    {
        return valid;
    }
    
    virtual int Next(Context* ctx)
    {
        if (started && (currSlot < slots.End()))
        {
            valid = true;
            Advance();
              
            switch (kind) {
            case IK_values:
                ctx->Push(val);
                return 1;
            case IK_keys:
                ctx->Push(key);
                return 1;
            case IK_both:
                ctx->Push(key);
                ctx->Push(val);
                return 2;
            case IK_default:
            default:
                if (ctx->GetRetCount() == 2)
                {
                    ctx->Push(key);
                    ctx->Push(val);
                    return 2;
                }
                else
                {
                    ctx->Push(val);
                    return 1;
                }
            }
        }
        else
        {
            valid = false;
            ctx->PushNull();
        }
        return 1;
    }
    
    virtual void MoveCurrent()
    {
        if (started && (currSlot < slots.End()))
        {            
            if (table->Get(*currSlot, val) && FilterValue(val, owner))
            {
                key = *currSlot;
                return;
            }
            else
            {
                // Key was deleted while we were iterating, move on to the next one.
                Advance();
                if (ToBoolean())
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
        key.SetNull();
        val.SetNull();
        if (owner)
        {
            if (!started)
            {
                return;
            }
            
            MoveCurrent();
            if (currSlot < slots.End())
            {
                ++currSlot;
            }
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
        MarkValue(c, key);
        MarkValue(c, val);
    }
private:
    IterateKind kind;
    bool valid;
    bool started;
    bool bValues;
    Value key;
    Value val;
    Object* owner;
    Buffer<Value> slots;
    Buffer<Value>::Iterator currSlot;
    Table* table;
};

}// pika

#endif
