/*
 *  PValueEnumerator.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_VALUEENUMERATOR_HEADER
#define PIKA_VALUEENUMERATOR_HEADER

namespace pika
{

// ValueEnumerator /////////////////////////////////////////////////////////////////////////////////

class PIKA_API ValueEnumerator : public Enumerator
{
public:
    ValueEnumerator(Engine* eng, const Value& prim);

    virtual            ~ValueEnumerator();

    virtual void        MarkRefs(Collector* c);

    virtual bool        Rewind();
    virtual bool        IsValid();
    virtual void        GetCurrent(Value&);
    virtual void        Advance();

    static Enumerator*  Create(Engine* eng, const Value& prim, String* enum_type);
protected:
    bool                is_valid;
    Value               value;
};

PIKA_API Enumerator* CreateRangeEnumerator(Engine*, pint_t from, pint_t to, pint_t step);
}

#endif
