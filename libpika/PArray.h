/*
 *  PAray.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_ARRAY_HEADER
#define PIKA_ARRAY_HEADER

#ifndef PIKA_BUFFER_HEADER
#   include "PBuffer.h"
#endif

#ifndef PIKA_OBJECT_HEADER
#   include "PObject.h"
#endif

namespace pika {
#if defined(PIKA_DLL) && defined(_WIN32)
template class PIKA_API Buffer<Value>;
#endif

class PIKA_API Array : public Object
{
    PIKA_DECL(Array, Object)
protected:
    friend class ArrayEnumerator;
    
    Array(Engine* eng, Type*, size_t, Value*);
    
    bool GetIndexOf(const Value& key, size_t &index);
public:
    virtual ~Array() {}
    
    virtual bool GetSlot(const Value&, Value&);
    virtual bool SetSlot(const Value&, Value&, u4 attr = 0);
    
    virtual void        MarkRefs(Collector*);
    virtual Object*     Clone();
    virtual Enumerator* GetEnumerator(String*);
    virtual void        Init(Context*);
    
    virtual String*  ToString();
    
    Array*  Slice(pint_t from, pint_t to);
    Array*  Map(Value fn);    
    Array*  Sort(Value fn);    
    Array*  Filter(Value);    
    Value   Foldr(const Value&, const Value& fn);
    Value   Fold(const Value&, const Value& fn);    
    Array*  Append(Array*);
    Value&  At(pint_t idx);
    Array*  TakeWhile(Value fn);
    Array*  DropWhile(Value fn);
    Array*  Unshift(Value& v);
    Value   Shift();
    
    Array*  Push(Value& v);
    Value   Pop();
    
    void    SetLength(ssize_t);
        
    Value   CatRhs(Value& lhs);
    Value   CatLhs(Value& rhs);
    
    Array*  Reverse();
    
    bool    Empty() const;
    
    Value   GetFront();
    Value   GetBack();
    
    void    SetFront(const Value& v);
    void    SetBack(const Value& v);
    
    INLINE size_t GetLength() { return elements.GetSize();  }   //!< Retrieves the size of the array.
    INLINE Value* GetAt(s4 idx) { return elements.GetAt(idx); } //!< Retrieves a pointer to the element at the given index.

    static Array*   Cat(Array* lhs, Array* rhs);
    static Array*   Create(Engine*, Type*, size_t length, Value *elems);
    static size_t   GetMax();
protected:
    Buffer<Value> elements;
};

}// pika

#endif


