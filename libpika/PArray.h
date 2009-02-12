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

namespace pika 
{
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
    
    Array*  Slice(pint_t from, pint_t to);   //!< Returns a subset from this array.

    Array*  Map(Value fn);                 //!< Performs the given function on each element.
    Array*  Sort(Value fn);                //!< Sort the array with given comparison function.
    Array*  Filter(Value);                 //!< Returns an array where all the elements are filtered through a function.
    Value   Foldr(const Value&, const Value& fn);    //!< Fold the array from right to left.
    Value   Fold(const Value&, const Value& fn);     //!< Fold the array from left to right.
    
    Array*  Unshift(Value& v);            //!< Add an element to the front of the array.
    Value   Shift();                      //!< Removes and returns the first element.

    Array*  Push(Value& v);               //!< Add an element to the back of the array.
    Value   Pop();                        //!< Removes and returns the last element.

    void    SetLength(ssize_t);           //!< Set the new size of the array. 

    Array*  Append(Array*);               //!< Appends an array to the end of this array.

    Value&  At(pint_t idx);                //!< Returns the element at the given index.

    Value   CatRhs(Value& lhs);           //!< Concat with this on the right-hand-side.
    Value   CatLhs(Value& rhs);           //!< Concat with this on the left-hand-side.
    
    Array*  Reverse();                    //!< Reverse the elements in the array.

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


