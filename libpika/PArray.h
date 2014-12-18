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

/** A User-defined binary-comparison Functor. */
struct PIKA_API ValueComp
{    
    ValueComp(Context* ctx, Value fn);
    
    ValueComp(const ValueComp& rhs);
    
    ~ValueComp();
    
    ValueComp& operator=(const ValueComp& rhs);
    
    bool operator()(const Value& l, const Value& r);
private:
    Context* context;       //!< Context to use for function calls.
    Value comparison_func;  //!< Comparison Function
};

/** A resizable array object. Also used for variable arguments and array literals. 
  * Most member functions can be used directly by C++ code as well as scripts.
  */
class PIKA_API Array : public Object
{
    PIKA_DECL(Array, Object)
protected:
    friend class ArrayIterator;
    
    explicit Array(Engine* eng, Type*, size_t, Value const*);
    explicit Array(const Array*);
    
    bool GetIndexOf(const Value& key, size_t &index);
public:
    virtual ~Array();
    virtual bool Finalize();
    
    virtual bool BracketRead(const Value&, Value&);
    virtual bool BracketWrite(const Value&, Value&, u4 attr = 0);
    
    virtual void        MarkRefs(Collector*);
    virtual Object*     Clone();
    virtual Iterator*   Iterate(String*);
    virtual void        Init(Context*);
    
    /** Converts the array into a string, formatted as an array literal. */
    virtual String* ToString();
    
    INLINE Value& operator[](size_t i) { return elements[i]; }
    INLINE const Value& operator[](size_t i) const { return elements[i]; }
    
    /** Returns a subset from this array. */
    Array* Slice(pint_t from, pint_t to);
    
    /** Performs the given function on each element.
      * @note The map function takes a single arguments (an element), the return
      *       value of fn will take the place of the element.
      * @param fn The map function or functor. 
      * @result This Array after map is performed.
      */
    Array* Map(Value fn);
    
    /** Sort the array with given comparison function. 
      * @note The comparison function should accept two arguments and return a boolean
      *       value based on the comparison method used.
      * @param fn The comparison function or functor. 
      * @result This Array sorted.
      */
    Array* Sort(Value fn);
    
    /** Returns an array where all the elements are filtered through a function. 
      * @note The filter function should accept a single arguments and should return
      *       a Boolean value which determines if the value should be kept or discarded.
      * @param fn The filter function or functor. 
      * @result An Array filled with the filtered values.
      */
    Array* Filter(Value fn);
    
    /** Fold all the elements of the array from left to right. */
    Value Fold(const Value& start_val, const Value& fn);    
    
    /** Fold all the elements of the array from right to left. */
    Value Foldr(const Value& start_val, const Value& fn);

    /** Appends an array to the end of this array. */
    Array* Append(Array*);

    /** Returns the element at the given index. */    
    Value& At(pint_t idx);
    
    Array* TakeWhile(Value fn);
    Array* DropWhile(Value fn);

    /** Add an element to the front of the array. */
    Array* Unshift(Value& v);

    /** Removes and returns the first element. */
    Value Shift();
    
    /** Add an element to the back of the array. */
    Array* Push(Value& v);
    
    /** Removes and returns the last element. */
    Value Pop();
    
    Value Zip(Array* rhs);
    
    /** Set the new length of the array. 
      * If the new size is larger than the current one the new elements are set to null.
      */
    void SetLength(ssize_t);
    
    /** Concat with this array on the right hand side.
      * @param lhs The left hand side operand of the concat operation.
      */
    Value CatRhs(Value& lhs);
    
    /** Concat with this on the left hand side. 
      * @param rhs The right hand side operand of the concat operation.
      */
    Value CatLhs(Value& rhs);
    
    /** Reverse the elements in the array. 
      * @result A pointer to this Array, with the elements reversed.
      */
    Array* Reverse();
    
    bool    Empty() const;
    virtual bool ToBoolean() { return !Empty(); }
    
    Value   GetFront();
    Value   GetBack();
    
    void    SetFront(const Value& v);
    void    SetBack(const Value& v);
    
    INLINE size_t GetLength() { return elements.GetSize();  }   //!< Retrieves the size of the array.
    INLINE Value* GetAt(s4 idx) { return elements.GetAt(idx); } //!< Retrieves a pointer to the element at the given index.
    
    static Array*   Cat(Array* lhs, Array* rhs);
    static Array*   Create(Engine*, Type*, size_t length, Value const* elems);
    static size_t   GetMax();
    
    static void Constructor(Engine* eng, Type* obj_type, Value& res);
    static void StaticInitType(Engine* eng);
    
    Buffer<Value>& GetElements() { return elements; }
protected:
    Buffer<Value> elements;
};

}// pika

#endif


