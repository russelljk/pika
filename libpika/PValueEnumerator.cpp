/*
 *  PValueEnumerator.cpp
 *  See Copyright Notice in Pika.h
 */

#include "PBasic.h"
#include "PEnumerator.h"
#include "PValueEnumerator.h"
#include "PEngine.h"
namespace pika {

class RangeEnumerator : public Enumerator
{
public:
    RangeEnumerator(Engine* eng, pint_t fromi, pint_t toi, pint_t stepi = 0)
            : Enumerator(eng),
            from(fromi),
            to(toi),
            curr(0),
            step(stepi)
    {
        if (!step)
        {
            step = (to < from) ? -1 : 1;
        }
        else if ((from > to && step > 0) ||
                 (from < to && step < 0))
        {
            RaiseException("invalid range and step");
        }
    }

    virtual ~RangeEnumerator() { }

    bool Rewind()
    {
        curr = from;
        return IsValid();
    }

    bool IsValid() { return(to > from) ? curr <= to : curr >= to; }

    void GetCurrent(Value& v) { v.Set(curr); }

    void Advance() { curr += step; }

    virtual Enumerator* GetEnumerator(String*) { return this; }
private:
    pint_t from;
    pint_t to;
    pint_t curr;
    pint_t step;
};

typedef pint_t (*IntegerEnumModifier)(pint_t);

class IntegerEnumerator : public Enumerator
{
public:
    IntegerEnumerator(Engine* eng, pint_t val, pint_t stepin = 0, IntegerEnumModifier modfn = 0)
            : Enumerator(eng),
            modifier(modfn),
            val(val),
            curr(0),
            step(stepin)
    {
        step = (!step) ? ((val < 0) ? -1 : 1) : step;
    }

    virtual ~IntegerEnumerator() { }

    bool Rewind()
    {
        curr = 0;
        return IsValid();
    }

    bool IsValid()
    {
        return (val < 0) ? curr >= val : curr <= val;
    }

    void GetCurrent(Value& v)
    {
        if (IsValid())
        {
            v.Set(modifier ? modifier(curr) : curr);
            return;
        }
        return;
    }

    void Advance() { curr += step; }
private:
    IntegerEnumModifier modifier;
    pint_t val;
    pint_t curr;
    pint_t step;
};

ValueEnumerator::ValueEnumerator(Engine* eng, const Value& prim)
        : Enumerator(eng),
        is_valid(false),
        value(prim)
{
}

ValueEnumerator::~ValueEnumerator() {}
void ValueEnumerator::MarkRefs(Collector* c) { MarkValue(c, value); }
bool ValueEnumerator::Rewind() { is_valid = true; return true; }
bool ValueEnumerator::IsValid() { return is_valid; }
void ValueEnumerator::GetCurrent(Value& v) { if (is_valid) { v = value; } }
void ValueEnumerator::Advance() { is_valid = false; }

pint_t IntegerSqMod(pint_t v)
{
    return v * v;
}

Enumerator* ValueEnumerator::Create(Engine* eng, const Value& prim, String* enum_type)
{
    GCPAUSE_NORUN(eng);
    //if (!enum_type || (enum_type != eng->emptyString && enum_type != eng->values_String && enum_type != eng->keys_String))
        //return DummyEnumerator::Create(eng);

    if (prim.IsInteger())
    {
        IntegerEnumModifier modfn = 0;
        if (enum_type == eng->AllocString("squares"))
            modfn = IntegerSqMod;
        else if (enum_type && enum_type != eng->emptyString)
            return DummyEnumerator::Create(eng);            
        
        IntegerEnumerator* re;
        PIKA_NEW(IntegerEnumerator, re, (eng, prim.val.integer, 0, modfn));
        eng->AddToGC(re);
        return re;
    }
    else if (prim.IsNull())
    {
        return DummyEnumerator::Create(eng);
    }
    else
    {
        ValueEnumerator* pe;
        PIKA_NEW(ValueEnumerator, pe, (eng, prim));
        eng->AddToGC(pe);
        return pe;
    }
}

Enumerator* CreateRangeEnumerator(Engine* eng, pint_t from, pint_t to, pint_t step)
{
    RangeEnumerator* re;
    PIKA_NEW(RangeEnumerator, re, (eng, from, to, step));
    eng->AddToGC(re);
    return re;
}

}// pika
