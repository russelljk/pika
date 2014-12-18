/*
 *  PArray.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PikaSort.h"
#include "PDictionary.h"
#include <algorithm>
#include "PIterator.h"

namespace pika {

PIKA_IMPL(Array)

class ArrayIterator : public Iterator
{
public:
    ArrayIterator(Engine* eng, Type* itype, IterateKind k, Array* owner)
            : Iterator(eng, itype ? itype : eng->Iterator_Type),
            valid(false),
            kind(k),
            owner(owner),
            curr(0) 
    {
        valid = Rewind();
    }
    
    virtual ~ArrayIterator() {}
    
    virtual bool Rewind()
    {        
        curr = 0;
        if (owner && (owner->elements.GetSize() > 0))
            return true;
        return false;
    }
    
    virtual bool ToBoolean()
    {
        return valid;
    }
    
    virtual int Next(Context* ctx)
    {
        if (curr < owner->elements.GetSize())
        {
            int retc = 0;
            switch (kind) {
            case IK_values:
                ctx->Push(owner->elements[curr]);
                retc = 1;
                break;
            case IK_keys:
                ctx->Push((pint_t)curr);
                retc = 1;
                break;
            case IK_both:
                ctx->Push((pint_t)curr);
                ctx->Push(owner->elements[curr]);
                retc = 2;
                break;
            case IK_default:
            default:
                if (ctx->GetRetCount() >= 2)
                {
                    ctx->Push((pint_t)curr);
                    ctx->Push(owner->elements[curr]);
                    retc = 2;
                }
                else
                {
                    ctx->Push(owner->elements[curr]);
                    retc = 1;
                }
                break;
            }
            ++curr;
            valid = true;
            return retc;

        }
        else
        {
            valid = false;
        }
        return 0;
    }
    
    virtual void MarkRefs(Collector* c)
    {
        if (owner)
            owner->Mark(c);
    }
    
    bool valid;
    IterateKind kind;
    Array* owner;
    size_t curr;
};

ValueComp::ValueComp(Context* ctx, Value fn)
    : context(ctx),
    comparison_func(fn) 
{}

ValueComp::ValueComp(const ValueComp& rhs)
    : context(rhs.context), comparison_func(rhs.comparison_func)
{}

ValueComp::~ValueComp() {}

ValueComp& ValueComp::operator=(const ValueComp& rhs)
{
    if (this != &rhs)
    {
        context = rhs.context;
        comparison_func  = rhs.comparison_func;
    }
    return *this;
}

bool ValueComp::operator()(const Value& l, const Value& r)
{
    Engine* eng = context->GetEngine();
    
    context->CheckStackSpace(4);    // 2 arguments + self + function = 4
    context->Push(l);               // Left  operand
    context->Push(r);               // Right operand
    context->PushNull();            // Self  object
    context->Push(comparison_func); // Comp  function
    
    if (context->SetupCall(2))
    {
        // call the function
        context->Run();
    }
    // convert the return value to boolean.
    
    Value& res   = context->Top();
    bool   bcomp = eng->ToBoolean(context, res);
    
    context->Pop();
    return bcomp;
}

bool Array::BracketRead(const Value& key, Value& res)
{
    size_t index = 0;    
    if (GetIndexOf(key, index))
    {
        res = elements[index];
        return true;
    }
    return false;
}

bool Array::BracketWrite(const Value& key, Value& value, u4 attr)
{
    size_t index = 0;    
    if (GetIndexOf(key, index))
    {
        elements[index] = value;
        WriteBarrier(value);
        return true;
    }
    return false;
}

bool Array::GetIndexOf(const Value& key, size_t &index)
{
    if (key.tag == TAG_integer)
    {
        if (key.val.integer < 0)
        {
            if (-index < (pint_t)elements.GetSize()) {
                index = elements.GetSize() + key.val.integer;
                return true;
            }
        }
        
        if ((key.val.integer >= 0) && (key.val.integer < (pint_t)elements.GetSize()))
        {
            index = (size_t)key.val.integer;
            return true;
        }
    }
    return false;
}

void Array::MarkRefs(Collector* c)
{
    ThisSuper::MarkRefs(c);
    
    size_t sz = elements.GetSize();
    
    for (size_t i = 0; i < sz; ++i)
    {
        MarkValue(c, elements[i]);
    }
}

Array* Array::Create(Engine* eng, Type* type, size_t length, Value const* elems)
{
    if (length > GetMax()) {
        RaiseException("Attempt to create an Array larger than the maximum size allowed "SIZE_T_FMT".", GetMax());
    }
    
    void* sp = eng->ArrayRawAlloc();
    type = type ? type : eng->Array_Type;
    Array* v = new (sp) Array(eng, type, length, elems);
    eng->AddToGC(v);
    return v;
}

Object* Array::Clone()
{
    Array* a = 0;
    GCNEW(engine, Array, a, (this));
    return a;
}

size_t Array::GetMax()
{
    return Min<size_t>(PIKA_BUFFER_MAX_LEN, GetMaxSize<Value>());
}

void Array::SetLength(ssize_t slen)
{
    if (slen < 0)
    {
        RaiseException("Negative array size specified.");
    }
    size_t len = static_cast<size_t>(slen);
    if (len >= GetMax())
    {
        RaiseException("Attempt to create an Array larger than the maximum size allowed "SIZE_T_FMT".", GetMax());
    }
    size_t oldlen = elements.GetSize();
    size_t amt    = (oldlen < len) ? (len - oldlen) : 0;
    
    elements.SmartResize(len);
    
    for (size_t i = 0 ; i < amt ; ++i)
        elements[oldlen + i].SetNull();
}

Array* Array::Unshift(Value& v)
{
    if (elements.GetSize() >= GetMax())
    {
        RaiseException("Max size of "SIZE_T_FMT" reached. Cannot add more elements to the array.", GetMax());
    }
    WriteBarrier(v);
    
    elements.SmartResize(elements.GetSize() + 1);
    Pika_memmove(elements.GetAt(1), elements.GetAt(0), (elements.GetSize() - 1) * sizeof(Value));
    elements[0] = v;
    return this;
}

String* Array::ToString()
{
    GCPAUSE_NORUN(engine);
    size_t    len = elements.GetSize();
    Context*  ctx = engine->GetActiveContextSafe();
    String* comma = engine->AllocString(",");
    String*  curr = engine->AllocString("[");
    
    for (size_t i = 0; i < len; ++i)
    {
        String* res = engine->SafeToString(ctx, elements[i]);
        
        if (res)
        {
            curr = String::ConcatSpace(curr, res);
        }

        // Add a comma if this is not the last element.
        if (i + 1 != len)
            curr = String::Concat(curr, comma);
    }
    String* end = engine->AllocString("]");
    curr = String::ConcatSpace(curr, end);
    return curr;
}

Value Array::Shift()
{
    size_t len = elements.GetSize();
    Value v;
    
    if (len > 0)
    {
        v = elements[0];
        
        if (len > 1)
        {
            Pika_memmove(elements.GetAt(0), elements.GetAt(1), (len - 1) * sizeof(Value));
        }
        elements.Pop();
    }
    else
    {
        v.SetNull();
    }
    return v;
}

bool Array::Empty() const { return elements.GetSize() == 0; }

Array* Array::Push(Value& v)
{
    if (elements.GetSize() >= GetMax())
    {
        RaiseException("Max size of "SIZE_T_FMT" reached. Cannot add more elements to the array.", GetMax());
    }
    WriteBarrier(v);
    elements.Push(v);
    return this;
}

Array* Array::Reverse()
{
    std::reverse(elements.Begin(), elements.End());
    return this;
}

Array::~Array() {}

bool Array::Finalize()
{
    engine->DelArray(this);
    return false;
}

Value Array::Pop()
{
    size_t len = elements.GetSize();
    Value v(NULL_VALUE);
    
    if (len > 0)
    {
        v = elements[len - 1];
        elements.Pop();
    }
    else
    {
        RaiseException("Attempt to remove elements from an empty array");
    }
    return v;
}

Value Array::Zip(Array* rhs)
{
    Value res(NULL_VALUE);
    Buffer<Value>& left = this->elements;
    Buffer<Value>& right = rhs->elements;
    
    size_t len = Min(left.GetSize(), right.GetSize());
    Dictionary* dict = Dictionary::Create(engine, engine->Dictionary_Type);
    
    for (size_t i = 0; i < len; ++i) {
        dict->BracketWrite(left[i], right[i]);
    }
    res.Set(dict);
    return res;
}

Array::Array(Engine* eng, Type* arrType, size_t length, Value const* elems)
        : ThisSuper(eng, arrType)
{
    elements.Resize(length);
    
    if (!elems)
    {
        for (size_t i = 0; i < length; ++i)
        {
            elements[i].SetNull();
        }
    }
    else
    {
        Pika_memcpy(elements.GetAt(0), elems, length * sizeof(Value));
    }
}

Array::Array(const Array* rhs) :
    ThisSuper(rhs),
    elements(rhs->elements)
{
}

Iterator* Array::Iterate(String *iter_type)
{
    IterateKind kind = IK_default;
    if (iter_type == engine->elements_String) {
        kind = IK_values;
    } else if (iter_type == engine->keys_String) {
        kind = IK_keys;
    } else if (iter_type != engine->emptyString) {
        return ThisSuper::Iterate(iter_type);
    }
    
    Iterator* e = 0;
    PIKA_NEW(ArrayIterator, e, (engine, engine->Iterator_Type, kind, this));
    engine->AddToGC(e);
    return e;
}

// Array functions /////////////////////////////////////////////////////////////////////////////////

void Array::Init(Context* ctx)
{
    u2 argc = ctx->GetArgCount();
    
    if (argc == 1)
    {
        pint_t len = ctx->GetIntArg(0);
        
        if (len >= 0)
        {
            SetLength(len);
        }
        else
        {
            RaiseException("Attempt to create an Array with a negative length.\n");
        }
    }
    else if (argc != 0)
    {
        ctx->WrongArgCount();
    }
}

Array* Array::Slice(pint_t from, pint_t to)
{
    Engine::CollectorPause gcp(engine);
    bool reverse = false;
    
    if (to < from)
    {
        Swap(to, from);
        reverse = true;
    }
    if (from < 0 || to > (pint_t)GetLength())
    {
        return 0;
    }
    
    pint_t len = to - from;
    Array* res = Array::Create(engine, 0, len, GetAt(from));
    
    if (reverse)
    {
        res->Reverse();
    }
    return res;
}

Array* Array::Map(Value m)
{
    Context* ctx = engine->GetActiveContextSafe();
    
    for (size_t i = 0; i < elements.GetSize(); ++i)
    {
        ctx->Push(elements[i]);
        ctx->PushNull();
        ctx->Push(m);
        
        if (ctx->SetupCall(1))
        {
            ctx->Run();
        }
        
        // user might resize the vector behind our backs!
        if (i < elements.GetSize())
        {             
            elements[i] = ctx->PopTop();
        }
    }
    return this;
}

Array* Array::Sort(Value fn)
{
    Context* ctx = engine->GetActiveContextSafe();
    
    // keep the value GC safe (since we don't know how this method is called).
    
    ctx->Push(fn);
    
    /* We sort using indexers so that modifications to the orignal
     * array will not cause an out of bounds read|write. This is slower
     * but we really don't have a choice if we want to keep pika_sort from
     * reading out of bounds.
     */
    pika_sort(elements.iBegin(),
              elements.iEnd(),
              ValueComp(ctx, fn));
              
    ctx->Pop();
    return this;
}

Array* Array::Filter(Value fn)
{
    Context* ctx = engine->GetActiveContextSafe();
    
    Array* v = Array::Create(engine, 0, 0, 0);
    SafeValue safe(ctx, v);
    for (size_t i = 0; i < elements.GetSize(); ++i)
    {
        Value val = elements[i];
        ctx->CheckStackSpace(3);
        ctx->Push(val);
        ctx->PushNull();
        ctx->Push(fn);
        
        if (ctx->SetupCall(1))
        {
            ctx->Run();
        }
        Value res = ctx->Top();
        
        if (engine->ToBoolean(ctx, res))
        {
            v->Push(val);
        }
        ctx->Pop();
    }
    return v;
}

Array* Array::TakeWhile(Value fn)
{   
    Context* ctx = engine->GetActiveContextSafe();
    
    for (size_t i = 0; i < elements.GetSize(); ++i)
    {
        Value val = elements[i];
        ctx->CheckStackSpace(3);
        ctx->Push(val);
        ctx->PushNull();
        ctx->Push(fn);
        
        if (ctx->SetupCall(1))
        {
            ctx->Run();
        }
        Value res = ctx->Top();
        
        if (!engine->ToBoolean(ctx, res))
        {
            ctx->Pop();
            return Array::Create(engine, 0, Min<size_t>(i, elements.GetSize()), elements.GetAt(0));
        }
        ctx->Pop();
    }
    return Array::Create(engine, 0, elements.GetSize(), elements.GetAt(0));
}

Array* Array::DropWhile(Value fn)
{
    Engine::CollectorPause pauser(engine);
    
    Context* ctx = engine->GetActiveContextSafe();    
    
    for (size_t i = 0; i < elements.GetSize(); ++i)
    {
        Value val = elements[i];
        ctx->CheckStackSpace(3);
        ctx->Push(val);
        ctx->PushNull();
        ctx->Push(fn);
        
        if (ctx->SetupCall(1))
        {
            ctx->Run();
        }
        Value res = ctx->Top();
        
        if (!engine->ToBoolean(ctx, res))
        {
            size_t amt = elements.GetSize() > i ? elements.GetSize() - i : elements.GetSize();
            Array* v = Array::Create(engine, engine->Array_Type, amt, this->elements.GetAt(i));
            return v;
        }
        
        ctx->Pop();
    }
    Array* nullv = Array::Create(engine, 0, 0, 0);
    return nullv;
}
    
Value Array::Fold(const Value& init, const Value& fn)
{
    Context*  ctx = engine->GetActiveContextSafe();
    ctx->Push(init);
    for (size_t i = 0; i < elements.GetSize(); ++i)
    {
        ctx->CheckStackSpace(3);
        // (init) is on the stack.
        ctx->Push(elements[i]);
        ctx->PushNull();
        ctx->Push(fn);
        
        if (ctx->SetupCall(2))
        {
            ctx->Run();
        }
        // result stays on the stack.
    }
    return ctx->PopTop();
}

Value Array::Foldr(const Value& init, const Value& fn)
{
    Context* ctx = engine->GetActiveContextSafe();
    ctx->Push(init);
    for (pint_t i = (pint_t)elements.GetSize() - 1; (i >= 0) && (i < (pint_t)elements.GetSize()); --i)
    {
        ctx->CheckStackSpace(3);
        // (init) is on the stack.
        ctx->Push(elements[i]);
        ctx->PushNull();
        ctx->Push(fn);
        
        if (ctx->SetupCall(2))
        {
            ctx->Run();
        }
        // result stays on the stack.
    }
    return ctx->PopTop();
}

Array* Array::Append(Array* other)
{
    for (size_t i = 0; i < other->elements.GetSize(); ++i)
    {
        WriteBarrier(other->elements[i]);
        elements.Push(other->elements[i]);
    }
    return this;
}

// TODO: Add a multi return value version of Array.at
Value& Array::At(pint_t idx)
{
    if (idx < 0 || (size_t)idx >= elements.GetSize())
    {
        if (idx < 0) {
            RaiseException(Exception::ERROR_index, "Attempt to access an element with an index of "PINT_FMT".", idx); // TODO: BoundsError
        } else {
            RaiseException(Exception::ERROR_index, "Attempt to access element "PINT_FMT" from an array of size "SIZE_T_FMT".", idx, elements.GetSize()); // TODO: BoundsError
        }        
    }
    return *elements.GetAt(idx);
}

Array* Array::Cat(Array* lhs, Array* rhs)
{
    Engine* eng  = lhs->GetEngine();
    size_t  lenl = lhs->GetLength();
    size_t  lenr = rhs->GetLength();
    size_t  len  = lenl + lenr;
    Array*  res  = Array::Create(eng, 0, len, 0);
    
    Pika_memcpy(res->GetAt(0), lhs->GetAt(0), lenl*sizeof(Value));
    Pika_memcpy(res->GetAt((pint_t)lenl), rhs->GetAt(0), lenr*sizeof(Value));
    
    return res;
}

Value Array::CatRhs(Value& lhs)
{
    GCPAUSE_NORUN(engine);
    Context* ctx = engine->GetActiveContextSafe();
    Value res(NULL_VALUE);
    if (engine->Array_Type->IsInstance(lhs))
    {
        Array* arr = Cat((Array*)lhs.val.object, this);
        res.Set(arr);
    }
    else 
    {
        String* str = this->ToString();
        String* lhstr = engine->emptyString;
        if (lhs.IsString())
        {
            lhstr = lhs.val.str;
        }
        else
        {
            lhstr = engine->ToString(ctx, lhs);
        }
        res.Set( String::Concat(lhstr, str) );
    }
    return res;
}

Value Array::CatLhs(Value& rhs)
{
    GCPAUSE_NORUN(engine);
    Context* ctx = engine->GetActiveContextSafe();
    Value res(NULL_VALUE);
    if (engine->Array_Type->IsInstance(rhs))
    {
        // Concatenate the 2 Arrays together.
        Array* arr = Cat(this, (Array*)rhs.val.object);
        res.Set(arr);
    }
    else
    {
        // Array concatenation with a non-Array object.
        // We convert both arguments to string and
        // concatenate the results.
        String* str = this->ToString();
        String* rhstr = engine->emptyString;
        if (rhs.IsString())
        {
            rhstr = rhs.val.str;
        }
        else
        {
            rhstr = engine->ToString(ctx, rhs);
        }
        res.Set( String::Concat(str, rhstr) );
    }
    return res;
}

Value Array::GetFront()
{
    if (elements.GetSize() == 0)
    {
        RaiseException(Exception::ERROR_index, "Attempt to retrieve the first element of an empty array.\n");
    }
    return elements.Front();
}

Value Array::GetBack()
{
    if (elements.GetSize() == 0)
    {
        RaiseException(Exception::ERROR_index, "Attempt to retrieve the last element of an empty array.\n");
    }
    return elements.Back();
}

void Array::SetFront(const Value& v)
{
    if (elements.GetSize() == 0)
    {
        RaiseException(Exception::ERROR_index, "Attempt to set the first element of an empty array.\n");
    }
    elements.Front() = v;
}

void Array::SetBack(const Value& v)
{
    if (elements.GetSize() == 0)
    {
        RaiseException(Exception::ERROR_index, "Attempt to set the last element of an empty array.\n");
    }
    elements.Back() = v;
}

void Array::Constructor(Engine* eng, Type* obj_type, Value& res)
{
    Object* obj = Array::Create(eng, obj_type, 0, 0);
    res.Set(obj);
}

#define RETURN_VAL_AFTER() "The return value is a reference to this array."

PIKA_DOC(Array_sort, "/(comp)\
\n\
Sort the array based on the comparison function |comp|. The algorithm used to \
sort elements is quicksort with insertion sort for small arrays. "
RETURN_VAL_AFTER()
"\
[[[# Sort from smallest to largest\n\
a = [21, 22, 3, -3, 99, 5, 7]\n\
a.sort(\\(x, y)=>x < y)\
]]]\
")

PIKA_DOC(Array_empty, "/()\
\n\
Returns true or false depending on whether the array is empty.\
")

PIKA_DOC(Array_append, "/(a)\
\n\
Appends the elements of array |a| onto the end of this array. \
"RETURN_VAL_AFTER())

PIKA_DOC(Array_zip, "/(a)\
\n\
Returns a [Dictionary] made with the keys taken from this array's elements and \
values taken from |a|'s elements. \
The size of the dictionary will be the equal to min(self.length, |a|.length).")

PIKA_DOC(Array_at, "/(idx)\
\n\
Return the element located at the given index |idx|. If |idx| is not an [Integer] or is \
out of bounds an exception will be raised.")

PIKA_DOC(Array_reverse, "/()\
\n\
Reverses the order of elements in this array. \
"RETURN_VAL_AFTER())

PIKA_DOC(Array_push, "/(val)\
\n\
Pushes |val| on to back of the array.\
"RETURN_VAL_AFTER())

PIKA_DOC(Array_pop, "/()\
\n\
Pops and returns the last element of the array. If the array is empty an exception will be raised.")

PIKA_DOC(Array_shift, "/()\
\n\
Pops and returns the first element of the array. If the array is empty an exception will be raised.")

PIKA_DOC(Array_unshift, "/(val)\
\n\
Pushes |val| on to the front of the array.\
"RETURN_VAL_AFTER())

PIKA_DOC(Array_map, "/(fn)\
\n\
For each element the function |fn| is called with that element as the argument. \
The element is then replaced with the return value from |fn|."
RETURN_VAL_AFTER()
"\
[[[\
a = [1, 2, 3]\n\
\n\
# Square each element\n\
a.map(\\(x)=> x * x )]]]\
")

PIKA_DOC(Array_filter, "/(fn)\
\n\
Creates and returns a new array with elements filtered through the function |fn|. \
The filter function will be called for each element in this array. When the filter returns true \
the element is added to the new array. Otherwise, the element will be discarded.\
\
[[[\
a = [1, 2, 3, 4, 5, 6]\n\
\n\
# Only let evens through\n\
a.filter(\\(x)=> x mod 2 == 0)]]]\
")

PIKA_DOC(Array_takeWhile, "/(cond)\
\n\
Creates and returns a new array costructed from elements from this array. \
Elements are taken until the function |cond| returns '''false'''. \
At which point the elements left are discarded.\
[[[\
a = [1, 2, 3, 4, 5]\n\
print(a.takeWhile(\\(x)=> x <= 2) #=> [1, 2]\
]]]\
")

PIKA_DOC(Array_dropWhile, "/(cond)\
\n\
Creates and returns a new array costructed from elements from this array. \
Elements are dropped while the function |cond| returns '''false'''. \
At which point the elements left are added to the new array.\
\
[[[\
a = [1, 2, 3, 4, 5]\n\
print(a.dropWhile(\\(x)=> x <= 2) #=> [3, 4, 5]\
]]]\
")

PIKA_DOC(Array_fold, "/(x, func)\
\n\
Folds this array into a single value by calling |func| with arguments |x| and the \
first element. |x| is then set equal to the return value of |func|. \
The operation is then performed again on the next element with the updated \
value of |x|. This will continue until each element in the array is exhausted.\
\n\
This is the reverse of [foldr]. \
\
[[[\
a = ['bob', 'says', 'hello']\n\
print a.fold('', \\(a, b) => a...b) #=> 'bob says hello'\
]]]\
")

PIKA_DOC(Array_foldr, "/(x, func)\
\n\
Folds this array into a single value by calling |func| with arguments |x| and the \
last element. |x| is then set equal to the return value of |func|. \
The operation is then performed again on the penultimate element with the updated \
value of |x|. This will continue until each element in the array is exhausted.\
\n\
This is the reverse of [fold]. \
\
[[[\
a = ['bob', 'says', 'hello']\n\
print a.foldr('', \\(a, b) => a...b) #=> 'hello says bob'\
]]]\
")

PIKA_DOC(Array_opCat, "/(right)\
\n\
An override of the concatenation operator which returns this array concatentated \
with the array |right|. \
\
[[[\
c = [2, 3]..[4, 5] #=> [2, 3, 4, 5]\
]]]\
")

PIKA_DOC(Array_opCat_r, "/(left)\
\n\
An override of the right-handed concatenation operator which returns the array \
|left| concatentated with this array. \
\
[[[\
c = [2, 3]..[4, 5] #=> [2, 3, 4, 5]\
]]]\
")

PIKA_DOC(Array_opSlice, "/(start, stop)\
\n\
Returns a subset of the array from |start| to |stop|. If |start| is greater \
than |stop| the elements will be reversed. The number of elements in the array \
will be |stop|-|start| with |start| being the first element taken.\
\
[[[\
a = [1, 2, 3, 4, 5]\n\
print a[1 to 3] #=> [2, 3]\n\
print a[3 to a.length] #=> [4, 5]\
]]]\
")

PIKA_DOC(Array_toString, "/()\
\n\
Returns a string representation of this array.")

PIKA_DOC(Array_cat, "/(left, right)\
\n\
Return the concatenation of arrays |left| and |right|.\
[[[\
a, b = [1, 2], [4, 5]\n\
print Array.cat(a, b) #=> [1, 2, 4, 5]\
]]]\
")

PIKA_DOC(Array_length, "The length of the array.")

PIKA_DOC(Array_getLength, "/()\
\n\
Return the length of the array.")

PIKA_DOC(Array_setLength, "/(len)\
\n\
Resizes the array to length |len|. If |len| is < 0 then an exception will be raised.")

PIKA_DOC(Array_front, "The first element of the array.")

PIKA_DOC(Array_getFront, "/()\
\n\
Returns the first element of the array. An exception will be \
raised if the array is empty.")

PIKA_DOC(Array_setFront, "/(x)\
\n\
Sets the first element of the array to |x|. An exception will be \
raised if the array is empty.")

PIKA_DOC(Array_back, "The last element of the array.")

PIKA_DOC(Array_getBack, "/()\
\n\
Returns the last element of the array. An exception will be \
raised if the array is empty.")

PIKA_DOC(Array_setBack, "/(x)\
\n\
Sets the last element of the array to |x|. An exception will be \
raised if the array is empty.")

PIKA_DOC(Array_Type, "A resizable array object. This type is also used for variable \
arguments and array literals.")

int Array_opSlice(Context* ctx, Value& self)
{
    pint_t from, to;
    GETSELF(Array, array, "Array");
    Array* res  = 0;
    
    if (ctx->IsArgNull(0)) {
        from = 0;
        to   = ctx->GetIntArg(1);
    } else if (ctx->IsArgNull(1)) {
        from = ctx->GetIntArg(0);
        to   = array->GetLength();
    } else {
        from = ctx->GetIntArg(0);
        to   = ctx->GetIntArg(1);
    }
    
    if (to >= 0 && from >= 0)
    {
        res = array->Slice(from, to);
    }
    
    if (res)
    {
        ctx->Push(res);
        return 1;
    }
    return 0;
}
    
void Array::StaticInitType(Engine* eng)
{
    pint_t Array_MAX = Array::GetMax();
    
    SlotBinder<Array>(eng, eng->Array_Type)
    .Method(&Array::Empty,      "empty?",   PIKA_GET_DOC(Array_empty))
    .Method(&Array::Append,     "append",   PIKA_GET_DOC(Array_append))
    .Method(&Array::At,         "at",       PIKA_GET_DOC(Array_at))
    .Method(&Array::Reverse,    "reverse",  PIKA_GET_DOC(Array_reverse))
    .Method(&Array::Push,       "push",     PIKA_GET_DOC(Array_push))
    .Method(&Array::Pop,        "pop",      PIKA_GET_DOC(Array_pop))
    .Method(&Array::Shift,      "shift",    PIKA_GET_DOC(Array_shift))
    .Method(&Array::Unshift,    "unshift",  PIKA_GET_DOC(Array_unshift))
    .Method(&Array::Map,        "map",      PIKA_GET_DOC(Array_map))
    .Method(&Array::Filter,     "filter",   PIKA_GET_DOC(Array_filter))
    .Method(&Array::TakeWhile,  "takeWhile",PIKA_GET_DOC(Array_takeWhile))
    .Method(&Array::DropWhile,  "dropWhile",PIKA_GET_DOC(Array_dropWhile))
    .Method(&Array::Foldr,      "foldr",    PIKA_GET_DOC(Array_foldr))
    .Method(&Array::Fold,       "fold",     PIKA_GET_DOC(Array_fold))
    .Method(&Array::Sort,       "sort",     PIKA_GET_DOC(Array_sort))
    .Method(&Array::CatLhs,     "opCat",    PIKA_GET_DOC(Array_opCat))
    .Method(&Array::CatRhs,     "opCat_r",  PIKA_GET_DOC(Array_opCat_r))
    .Method(&Array::ToString,   "toString", PIKA_GET_DOC(Array_toString))
    .Method(&Array::Zip,        "zip",      PIKA_GET_DOC(Array_zip))    
    .RegisterMethod(Array_opSlice, OPSLICE_STR, 2, false, true, PIKA_GET_DOC(Array_opSlice))
    .StaticMethod(&Array::Cat,  "cat",      PIKA_GET_DOC(Array_cat))    
    .PropertyRW("length",
                &Array::GetLength,  "getLength",
                &Array::SetLength,  "setLength", 
                PIKA_GET_DOC(Array_getLength), 
                PIKA_GET_DOC(Array_setLength),
                PIKA_GET_DOC(Array_length))    
    .PropertyRW("front",
                &Array::GetFront,   "getFront",
                &Array::SetFront,   "setFront", 
                PIKA_GET_DOC(Array_getFront), 
                PIKA_GET_DOC(Array_setFront),
                PIKA_GET_DOC(Array_front)) 
    .PropertyRW("back",
                &Array::GetBack,    "getBack",
                &Array::SetBack,    "setBack", 
                PIKA_GET_DOC(Array_getBack), 
                PIKA_GET_DOC(Array_setBack),
                PIKA_GET_DOC(Array_back)) 
    .Constant(Array_MAX, "MAX")
    ;
    eng->GetWorld()->SetSlot(eng->Array_String, eng->Array_Type);
    eng->Array_Type->SetDoc(eng->AllocString(PIKA_GET_DOC(Array_Type)));
}

}// pika
