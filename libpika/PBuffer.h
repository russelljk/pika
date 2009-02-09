/*
 *  PBuffer.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_BUFFER_HEADER
#define PIKA_BUFFER_HEADER

#ifndef PIKA_CONFIG_HEADER
#   include "PConfig.h"
#endif
#ifndef PIKA_UTIL_HEADER
#   include "PUtil.h"
#endif
#ifndef PIKA_MEMORY_HEADER
#   include "PMemory.h"
#endif
#ifndef PIKA_ERROR_HEADER
#   include "PError.h"
#endif

#include <iterator>

namespace pika
{

// XXX: Export exception handling methods so that PikaError does not need to be included.
// XXX: Constructor that takes an initial size.

////////////////////////////////////////////// Buffer //////////////////////////////////////////////
/**
 *  Resizable template Buffer class.
 */
template<typename T>
class Buffer
{
public:    
    /** Access the Buffer with an index. This access is slower than using a pointer based Iterator but is
      * much safer because the size and location of the Buffer's elements can change and the Indexer is still valid.
      */
    class Indexer
    {
        size_t           index;
        Buffer<T>*       owner;
    public:
        friend           class Buffer<T>;
        
        INLINE           Indexer(size_t idx, Buffer<T>* b) : index(idx), owner(b) { }
        INLINE           Indexer() : index(0), owner(0) { }
        
        INLINE           Indexer(const Indexer& rhs) : index(rhs.index), owner(rhs.owner) { }
        
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef ptrdiff_t                       difference_type;
        typedef size_t                          size_type;
        typedef T&                              reference;
		typedef const T&						const_reference;
        typedef T*                              pointer;
        typedef T                               value_type;
        
        INLINE void CheckValid() const
        {
            if (index >= owner->GetSize())
            {
                RaiseException("Invalid indexer. %d", index);
            }
        }
        
        INLINE Indexer& operator= (const Indexer& x) {  index = x.index; owner = x.owner; return *this; }
        
        INLINE Indexer& operator++() { ++index; return(*this); }
        INLINE Indexer  operator++(int)
        { 
            Indexer tmp(*this);
            ++*this;
            return tmp;
        }
        INLINE Indexer& operator--()    { --index; return(*this); }
        INLINE Indexer  operator--(int) { Indexer tmp(*this); --*this; return tmp; }
        INLINE Indexer  operator+ (size_type off)
        {
            Indexer tmp(*this);
            tmp.index += off;
            return tmp;
        }
        INLINE Indexer& operator+=(size_type off) { index += off; return *this; }
        INLINE Indexer  operator- (size_type off)
        {
            Indexer tmp(*this);
            tmp.index -= off;
            return tmp;
        }
        INLINE friend difference_type operator-(const Indexer& lhs, const Indexer& rhs)
        {
            return lhs.index - rhs.index;
        }
        INLINE Indexer& operator-=(size_type off) { index -= off; return *this; }
        
        INLINE reference       operator* ()        { CheckValid(); return (*owner)[index];   }
        INLINE const_reference operator* () const  { CheckValid(); return (*owner)[index];   }
        
        INLINE pointer       operator->()        { CheckValid(); return  owner->PointerAt(index);  }
        INLINE const pointer operator->() const  { CheckValid(); return  owner->PointerAt(index);  }
                
        INLINE reference operator[](size_type off) { CheckValid(); return (*owner)[index + off];  } // TODO: Is this correct?
        
        INLINE bool	operator==(const Indexer& rhs) const { return index == rhs.index; }
        INLINE bool	operator!=(const Indexer& rhs) const { return index != rhs.index; }
        INLINE bool	operator<=(const Indexer& rhs) const { return index <= rhs.index; }
        INLINE bool	operator>=(const Indexer& rhs) const { return index >= rhs.index; }
        INLINE bool	operator< (const Indexer& rhs) const { return index <  rhs.index; }
        INLINE bool	operator> (const Indexer& rhs) const { return index >  rhs.index; }
    };
    
    /** Basic iterator that has partial compatibility with the C++ STL. 
      */
    class Iterator
    {
        T* myPtr;
    public:
        friend class Buffer<T>;
        
        typedef std::random_access_iterator_tag iterator_category;
        typedef ptrdiff_t                       difference_type;
        typedef size_t                          size_type;
        typedef T&                              reference;
		typedef const T&						const_reference;
        typedef T*                              pointer;
        typedef T                               value_type;
        
        INLINE Iterator(T* ptr = 0) : myPtr(ptr) { }
        INLINE Iterator(const Iterator& rhs) : myPtr(rhs.myPtr) { }
        
        INLINE Iterator& operator= (const Iterator& x) { myPtr = x.myPtr; return *this; }
        
        INLINE Iterator& operator++()        { ++myPtr; return(*this); }
        INLINE Iterator  operator++(int)
        {
            Iterator tmp(*this);
            ++*this;
            return tmp;
        }
        INLINE Iterator& operator--()        { --myPtr; return(*this); }
        INLINE Iterator  operator--(int)     { Iterator tmp(*this); --*this; return tmp; }
        INLINE Iterator  operator+ (size_type off)
        {
            Iterator tmp(*this);
            tmp.myPtr += off;
            return tmp;
        }
        INLINE Iterator& operator+=(size_type off) { myPtr += off; return *this; }
        INLINE Iterator  operator- (size_type off)
        {
            Iterator tmp(*this);
            tmp.myPtr -= off;
            return tmp;
        }
        INLINE friend difference_type operator-(const Iterator& lhs, const Iterator& rhs)
        {
            return lhs.myPtr - rhs.myPtr;
        }
        INLINE Iterator& operator-=(size_type off) { myPtr -= off; return *this; }
        
        INLINE reference operator* () { return *myPtr; }
        INLINE pointer   operator->() { return  myPtr; }
        
        INLINE const_reference operator* () const  { return *myPtr; }
        INLINE const pointer   operator->() const  { return  myPtr; }
                
        INLINE reference operator[](size_type off) { return  *(myPtr + off); }
        
        INLINE bool	operator==(const Iterator& rhs) const { return myPtr == rhs.myPtr; }
        INLINE bool	operator!=(const Iterator& rhs) const { return myPtr != rhs.myPtr; }
        INLINE bool	operator<=(const Iterator& rhs) const { return myPtr <= rhs.myPtr; }
        INLINE bool	operator>=(const Iterator& rhs) const { return myPtr >= rhs.myPtr; }
        INLINE bool	operator< (const Iterator& rhs) const { return myPtr <  rhs.myPtr; }
        INLINE bool	operator> (const Iterator& rhs) const { return myPtr >  rhs.myPtr; }
    };
    
    /** Basic constant iterator that has 'partial' compatibility with the C++ STL.  
      */
    class ConstIterator
    {
        const T* myPtr;
    public:
        friend class Buffer<T>;
        
        typedef std::random_access_iterator_tag iterator_category;
        typedef ptrdiff_t                       difference_type;
        typedef size_t                          size_type;
        typedef const T&                        reference;
        typedef const T*                        pointer;
        typedef const T                         value_type;
        
        INLINE ConstIterator(const T* ptr = 0) : myPtr(ptr) { }
        INLINE ConstIterator(const ConstIterator & rhs) : myPtr(rhs.myPtr) { }
        
        INLINE ConstIterator&   operator= (const ConstIterator& x) { myPtr = x.myPtr; return *this; }
        
        INLINE ConstIterator&   operator++()    { ++myPtr; return(*this); }
        INLINE ConstIterator    operator++(int) { ConstIterator tmp(*this);  ++*this; return tmp; }
        
        INLINE ConstIterator&   operator--()    { --myPtr; return(*this); }
        INLINE ConstIterator    operator--(int) { ConstIterator tmp(*this); --*this; return tmp; }
        
        INLINE ConstIterator    operator+ (size_type off) { ConstIterator tmp(*this); tmp.myPtr += off; return tmp; }
        INLINE ConstIterator&   operator+=(size_type off) { myPtr += off; return *this; }
        
        INLINE ConstIterator    operator- (size_type off) { ConstIterator tmp(*this); tmp.myPtr -= off; return tmp; }
        INLINE ConstIterator&   operator-=(size_type off) { myPtr -= off; return *this; }
        
        INLINE friend difference_type operator-(const ConstIterator& lhs, const ConstIterator& rhs)
        {
            return lhs.myPtr - rhs.myPtr;
        }
        
        INLINE reference    operator* () const { return *myPtr; }
        INLINE pointer      operator->() const { return myPtr; }
        INLINE reference    operator[](size_type off) const { return  *(myPtr + off); }
        
        INLINE bool	operator==(const ConstIterator& rhs) const { return myPtr == rhs.myPtr; }
        INLINE bool	operator!=(const ConstIterator& rhs) const { return myPtr != rhs.myPtr; }
        INLINE bool	operator<=(const ConstIterator& rhs) const { return myPtr <= rhs.myPtr; }
        INLINE bool	operator>=(const ConstIterator& rhs) const { return myPtr >= rhs.myPtr; }
        INLINE bool	operator< (const ConstIterator& rhs) const { return myPtr <  rhs.myPtr; }
        INLINE bool	operator> (const ConstIterator& rhs) const { return myPtr >  rhs.myPtr; }
    };
    
    INLINE Buffer() : elements(0), size(0), capacity(0) { }
    
    Buffer(const Buffer& a)
    {
        size     = 0;
        capacity = 0;
        elements = 0;
        
        size_t newSize = a.GetSize();
        
        if (newSize > 0)
        {
            SetCapacity(newSize);
            T* eThis = elements;
            T* eA    = a.elements;
            
            for (size_t i = 0; i < newSize; ++i)
            {
                *(eThis++) = *(eA++);
            }
        }
    }
    
    ~Buffer()
    {
        if (elements)
        {
            DestructRange(0, size);
            Pika_free((void*)elements);
        }
        elements = 0;
    }
    
    INLINE static size_t ResizeAmt(size_t oldcap)
    {
        return 2;
    }
    
    INLINE size_t NewCap(size_t oldcap, size_t sizeneeded) const
    {
        if (oldcap == 0)
        {
            return 16;
        }
                
        size_t nsize = (size_t)(oldcap * ResizeAmt(oldcap));
        
        if (nsize < sizeneeded)
        {
            nsize = sizeneeded;
        }
        return nsize;
    }
    
    void Push(const T& t)
    {
        if (size == capacity)
        {
            SetCapacity(NewCap(capacity, size + 1));
        }
        CopyConstrucor(&elements[size++], t);
    }
    
    void Pop()
    {
        if (size != 0)
        {
            Pika_destruct<T>(&elements[--size]);
        }
    }
    
    /** Removes any unused space so that the size and capacity match. */
    void ShrinkWrap()
    {
        if (size != capacity)
        {
            SetCapacity(size);
        }
    }
    
    void SetCapacity(size_t newSize)
    {
        if (newSize >= GetMaxSize<T>())
        {
            RaiseException("Internal class Buffer: max capacity reached.");
        }
        
        if (newSize < size)
        {
            DestructRange(newSize, size);
            size = newSize;
        }
        
        TNeedsDestructor<T> needsDestructor;        
        if (needsDestructor())
        {
            T* temp = elements;
            elements = (T*)Pika_malloc(newSize * sizeof(T));
            
            if (elements)
            {
                T* elementPtr = elements;
                T* tempPtr    = temp;
                
                for (size_t i = 0; i < size; ++i)
                {
                    *(elementPtr++) = *(tempPtr++);
                }
            }
            else
            {
                RaiseException("Internal class Buffer failed to allocate enough bytes.");
            }
            Pika_free((void*)temp);
        }
        else
        {
            elements = (T*)Pika_realloc(elements, newSize * sizeof(T));
            if (!elements)
            {
                RaiseException("Internal class Buffer failed to allocate enough bytes.");
            }
        }
        this->capacity = newSize;
    }
    
    INLINE size_t IsEmpty()     const { return size == 0; }
    INLINE size_t GetSize()     const { return size; }
    INLINE size_t GetCapacity() const { return capacity; }
    
    void Resize(size_t newSize)
    {
        size_t currentSize = GetSize();
        
        if (newSize < currentSize)
        {
            DestructRange(newSize, size);
            size = newSize;
        }
        else if (currentSize < newSize)
        {
            if (newSize > GetCapacity())
            {
                SetCapacity(newSize);
            }
            ConstructRange(size, newSize);
            size = newSize;
        }
    }
    
    void SmartResize(size_t newSize)
    {
        size_t currentSize = GetSize();
        
        if (newSize < currentSize)
        {
            DestructRange(newSize, size);
            size = newSize;
        }
        else if (currentSize < newSize)
        {
            if (newSize > capacity)
            {
                size_t newcap = NewCap(capacity, newSize);
                SetCapacity(newcap);
            }
            ConstructRange(size, newSize);
            size = newSize;
        }
    }
    
    void Clear() { Resize(0); }
    
    size_t      IndexOf(ConstIterator& iter)    const { return iter.myPtr - elements; }
    size_t      IndexOf(Iterator& iter)               { return iter.myPtr - elements; }
    
    const T*    PointerOf(ConstIterator& iter)  const { return iter.myPtr; } 
    T*          PointerOf(Iterator& iter)             { return iter.myPtr; }
    
    size_t      IndexOfPointer(const T* pT)     const { return pT - elements; } 
        
    /** When order doen't matter for the elements of an array you can use SwapAndPop to quickly erase an 
      * element. It works by swapping the element at the given position with the last element. Then the 
      * last element is popped of the back of the array.
      *
      * O(1)
      */
    bool SwapAndPop(Iterator& iter)
    {
        size_t iteratorIndex = iter.myPtr - elements;
        bool ret = SwapAndPop(iteratorIndex);
        iter.myPtr = elements + iteratorIndex;
        return ret;
    }
    
    bool SwapAndPop(size_t index)
    {
        if (!size || index >= size)
        {
            return false;
        }
        
        if (index != (size - 1))
        {
            Swap(elements[index], Back());
        }
        
        Pop();
        return true;
    }
    
    /** Erases the element at the given position. Since every element after that position must be
      * copied down it can be an expensive operation. If you do not care about the order of elements
      * you can call SwapAndPop.
      *
      * O(n)
      */
    bool Erase(Iterator& iter)
    {
        size_t iteratorIndex = iter.myPtr - elements;
        bool ret = Erase(iteratorIndex);
        iter.myPtr = elements + iteratorIndex;
        return ret;
    }
    
    bool Erase(size_t index)
    {
        if (!size)
        {
            return false;
        }
        if (index >= size)
        {
            return false;
        }
        if (index == size - 1)
        {
            //Easy case
            Pop();
            return true;
        }
        
        //Copy down.
        
        T* ePtr      = elements + index;
        T* ePtrPlus1 = elements + (index + 1);
        
        for (size_t i = index; i < size - 1; ++i)
        {
            *(ePtr++) = *(ePtrPlus1++);
        }
        
        Pika_destruct<T>(&elements[size - 1]);  //Destruct last element
        size--;
        return true;
    }
    
    void Insert(Iterator& iter, const T& t)
    {
        size_t iteratorIndex = iter.myPtr - elements; // Find 0-based index of the iterator
        Insert(iteratorIndex, t);
    }
    
    void Insert(size_t index, const T& t)
    {
        if (index > size)
        {
            return;
        }
        
        if ((GetSize() == 0) || (index == size))
        {
            Push(t);
            return;
        }
        
        Resize(size + 1);
        
        T* startPtr = elements + index;
        T* endPtr   = elements + size - 1;
        
        T* i = endPtr;
        T* j = endPtr - 1;
        
        for (; i != startPtr; --i, --j)
        {
            *i = *j;
        }
        *startPtr = t;
    }
    
    INLINE T& operator[](size_t i)
    {
        ASSERT(i < size);
        return elements[i];
    }
    
    INLINE const T& operator[](size_t i) const
    {
        ASSERT(i < size);
        return elements[i];
    }
    
    T*                      GetAt(size_t idx)       { return elements + idx; }
    const T*                GetAt(size_t idx) const { return elements + idx; }
    
    INLINE       T&         Back()       { return elements[size - 1]; }
    INLINE const T&         Back() const { return elements[size - 1]; }
    
    INLINE       T&         Front()       { return *Begin(); }
    INLINE const T&         Front() const { return *Begin(); }
    
    INLINE Iterator         At(size_t sz) { return Iterator(elements + sz); }
    
    INLINE Iterator         Begin() { return Iterator(elements); }
    INLINE Iterator         End()   { return Iterator(elements + size); }
    INLINE ConstIterator    BeginConst() { return ConstIterator(elements); }
    INLINE ConstIterator    EndConst()   { return ConstIterator(elements + size); }
        
    INLINE Indexer          iBegin() { return Indexer(0, this); }
    INLINE Indexer          iEnd()   { return Indexer(size, this); }
    
    INLINE T*               PointerAt(size_t sz) { return elements + sz; }
    
    INLINE T*               BeginPointer()       { return elements; }
    INLINE const T*         BeginPointer() const { return elements; }
    
    INLINE T*               EndPointer()       { return elements + size; }
    INLINE const T*         EndPointer() const { return elements + size; }
    
    INLINE ConstIterator    Begin() const { return ConstIterator(elements); }
    INLINE ConstIterator    End()   const { return ConstIterator(elements + size); }
protected:
    INLINE void CopyConstrucor(T *tPtr, const T &t) { *tPtr = t; }
    
    /** Calls the constructor for a given range of elements. */
    void ConstructRange(size_t start, size_t end)
    {
        ASSERT(start <= end);
        
        TNeedsDestructor<T> needsDestructor;
        
        if (needsDestructor())
        {
            T* startPtr = elements + start;
            T*   endPtr = elements + end;
            
            for (T* i = startPtr; i != endPtr; ++i)
            {
                Pika_construct<T>(i);
            }
        }
    }
    
    /** Calls the destructor for a given range of elements. */
    void DestructRange(size_t  start, size_t end)
    {
        ASSERT(start <= end);
        TNeedsDestructor<T> needsDestructor;
        
        if (needsDestructor())
        {
            T* startPtr = elements + start;
            T*   endPtr = elements + end;
            
            for (T* i = startPtr; i != endPtr; ++i)
            {
                Pika_destruct<T>(i);
            }
        }
    }
    
    T*      elements;
    size_t  size;
    size_t  capacity;
};

}//pika

#endif
