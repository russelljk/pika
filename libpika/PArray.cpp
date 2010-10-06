/*
 *  PArray.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PikaSort.h"
#include "PDictionary.h"
#include <algorithm>

namespace pika {

PIKA_IMPL(Array)

class ArrayEnumerator : public Enumerator
{
public:
    ArrayEnumerator(Engine* eng, bool indexes, Array* owner)
            : Enumerator(eng),
            started(false),
            indexes(indexes),
            owner(owner),
            curr(0) {}
    
    virtual ~ArrayEnumerator() {}
    
    virtual bool Rewind()
    {
        started = true;
        curr = 0;
        if (owner && (owner->elements.GetSize() > 0))
            return true;
        return false;
    }
    
    virtual void Advance()
    {
        if (curr < owner->elements.GetSize())
        {
            ++curr;
        }
    }
    
    virtual bool IsValid()
    {
        if (owner)
        {
            if (!started)
                return Rewind();
                
            if (curr < owner->elements.GetSize())
                return true;
        }
        return false;
    }
    
    virtual void GetCurrent(Value& c)
    {
        if (curr < owner->elements.GetSize())
        {
            if (indexes)
                c.Set((pint_t)curr);
            else
                c = owner->elements[curr];
        }
    }
    
    virtual void MarkRefs(Collector* c)
    {
        if (owner)
            owner->Mark(c);
    }
    
    bool started;
    bool indexes;
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

Array* Array::Create(Engine* eng, Type* type, size_t length, Value* elems)
{
    Array* v = 0;
    type = type ? type : eng->Array_Type;
    PIKA_NEW(Array, v, (eng, type, length, elems));
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
        RaiseException("Array size too large.");
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
        RaiseException("Cannot add more elements to array.");
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
        RaiseException("Max size reached. Cannot add more elements to the array.");
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
        RaiseException("Cannot remove elements from an empty array");
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

Array::Array(Engine* eng, Type* arrType, size_t length, Value* elems)
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

Enumerator* Array::GetEnumerator(String *enumtype)
{
    if (enumtype == engine->elements_String ||
        enumtype == engine->indices_String  ||
        enumtype == engine->emptyString)
    {
        Enumerator* e = 0;
        PIKA_NEW(ArrayEnumerator, e, (engine, (enumtype == engine->indices_String), this));
        engine->AddToGC(e);
        return e;
    }
    return ThisSuper::GetEnumerator(enumtype);
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
            RaiseException("Cannot create a Array with negative length\n");
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

Value& Array::At(pint_t idx)
{
    if (idx < 0 || (size_t)idx >= elements.GetSize())
    {
        RaiseException("Attempt to access element "PINT_FMT".", idx);
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
        RaiseException("Attempt to retrieve the first element of an empty array.\n");
    }
    return elements.Front();
}

Value Array::GetBack()
{
    if (elements.GetSize() == 0)
    {
        RaiseException("Attempt to retrieve the last element of an empty array.\n");
    }
    return elements.Back();
}

void Array::SetFront(const Value& v)
{
    if (elements.GetSize() == 0)
    {
        RaiseException("Attempt to set the first element of an empty array.\n");
    }
    elements.Front() = v;
}

void Array::SetBack(const Value& v)
{
    if (elements.GetSize() == 0)
    {
        RaiseException("Attempt to set the last element of an empty array.\n");
    }
    elements.Back() = v;
}

void Array::Constructor(Engine* eng, Type* obj_type, Value& res)
{
    Object* obj = Array::Create(eng, obj_type, 0, 0);
    res.Set(obj);
}

void Array::StaticInitType(Engine* eng)
{   
    pint_t Array_MAX = Array::GetMax();
    
    SlotBinder<Array>(eng, eng->Array_Type)
    .Method(&Array::Empty,      "empty?")
    .Method(&Array::Append,     "append")
    .Method(&Array::At,         "at")
    .Method(&Array::Reverse,    "reverse")
    .Method(&Array::Push,       "push")
    .Method(&Array::Push,       "opLsh")
    .Method(&Array::Push,       "opLsh_r")
    .Method(&Array::Unshift,    "opRsh")
    .Method(&Array::Unshift,    "opRsh_r")
    .Method(&Array::Pop,        "pop")
    .Method(&Array::Shift,      "shift")
    .Method(&Array::Unshift,    "unshift")
    .Method(&Array::Map,        "map")
    .Method(&Array::Filter,     "filter")
    .Method(&Array::TakeWhile,  "takeWhile")
    .Method(&Array::DropWhile,  "dropWhile")
    .Method(&Array::Foldr,      "foldr")
    .Method(&Array::Fold,       "fold")
    .Method(&Array::Sort,       "sort")
    .Method(&Array::CatLhs,     "opCat")
    .Method(&Array::CatRhs,     "opCat_r")
    .Method(&Array::Slice,      OPSLICE_STR)
    .Method(&Array::ToString,   "toString")
    .Method(&Array::Zip,        "zip")    
    .StaticMethod(&Array::Cat,  "cat")
    .PropertyRW("length",
                &Array::GetLength,  "getLength",
                &Array::SetLength,  "setLength")
    .PropertyRW("front",
                &Array::GetFront,   "getFront",
                &Array::SetFront,   "setFront")
    .PropertyRW("back",
                &Array::GetBack,    "getBack",
                &Array::SetBack,    "setBack")
    .Constant(Array_MAX, "MAX")
    ;
    eng->GetWorld()->SetSlot(eng->Array_String, eng->Array_Type);
}

}// pika
