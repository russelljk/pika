/*
 *  PTable.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_TABLE_HEADER
#define PIKA_TABLE_HEADER

#ifndef PIKA_VALUE_HEADER
#   include "PValue.h"
#endif

namespace pika {

struct Slot
{
    enum Attributes
    {
        ATTR_none       = 0x0,  // no attributes
        ATTR_nodelete   = 0x1,  // cannot be removed
        ATTR_final      = 0x2,  // cannot change values
        ATTR_noenum     = 0x4,  // is not enumerated by an Enumerator or foreach statement
        ATTR_forcewrite = 0x8,  // forces a final Slot to be overwritten.
        ATTR_virtual    = 0x10,
        ATTR_protected  = (ATTR_final | ATTR_nodelete),
        ATTR_internal   = (ATTR_final | ATTR_noenum | ATTR_nodelete),
    };

    INLINE Slot(const Value& k, Value& v, u4 a)
            : key(k),
            val(v),
            next(0),
            attr(a)
    {}

    Value key;  //!< Lookup key or name of the slot.
    Value val;  //!< Value of the slot.
    Slot* next; //!< Next chained slot.
    u4    attr; //!< Bitwise or'ed attributes of the slot.
};

class PIKA_API Table 
{
public:
    /*
     * Simple forward iterator that allows you to move throught each slot in a table.
     * Warning: Do not change the key of the slot!
     */
    struct Iterator 
    {
        Iterator(Table* t)
                : owner(t)
        {
            Reset();
        }

        Iterator(const Iterator& i) : index(i.index), pos(i.pos), owner(i.owner) {}

        void Reset()
        {
            for (index = 0; index < owner->size; ++index) 
            {
                pos = owner->slots[index];
                if (pos) 
                {
                    break;
                }
            }
        }

        INLINE operator bool()  const { return (index < owner->size) && pos;  }
        INLINE bool operator!() const { return !((index < owner->size) && pos); }

        INLINE Iterator& operator++()
        {
            if (pos && pos->next) 
            {
                pos = pos->next;
            }
            else 
            {
                ++index;
                for (; index < owner->size; ++index) 
                {
                    pos = owner->slots[index];
                    if (pos)
                        break;
                }
            }
            return (*this);
        }
        
        INLINE Iterator operator++(int) { Iterator tmp(*this); ++*this; return tmp; }

        INLINE Slot& operator*()  { return *pos; }
        INLINE Slot* operator->() { return  pos; }

        size_t index;
        Slot*  pos;
        Table* owner;
    };
private:
    void Grow();

public:
    Table();

    Table(const Table& other);

    ~Table();
    
    bool Exists(const Value& key);
    bool Set(const Value& key, Value& value, u4 attrs = 0);
    bool Get(const Value& key, Value& res);
    bool CanSet(const Value& key);
    bool CanInherit(const Value& key);
    bool Remove(const Value& key);

    Iterator GetIterator() { return Iterator(this); }

    void DoMark(class Collector*);
    void Clear();
    
    Slot** slots; //!< Linear array of slots. Each position in the array may have more than 1 chained slot.
    size_t count; //!< The number of elements in the table.
    size_t size;  //!< The length of the slots member variable.

    static size_t const MAX_TABLE_SLOTS;
    static size_t const MAX_TABLE_SIZE;
};

}// pika

#endif
