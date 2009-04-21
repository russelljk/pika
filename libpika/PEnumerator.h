/*
 *  PEnumerator.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_ENUMERATOR_HEADER
#define PIKA_ENUMERATOR_HEADER

namespace pika {

/* Enumerator //////////////////////////////////////////////////////////////////////////////////////
 * 
 * Abstract class that provides the interface used for enumeration. 
 */
class PIKA_API Enumerator : public Basic
{
    PIKA_DECL(Enumerator, Basic)
public:
    INLINE Enumerator(Engine* eng) : Basic(eng) { }

    virtual ~Enumerator() { }

    virtual Type* GetType() const;
        
    virtual bool GetSlot(const Value& key, Value& result);
        
    /** Start | restart the enumerator. */
    virtual bool Rewind() = 0;
    
    /** Is the enumerator currently valid. */
    virtual bool IsValid() = 0;
    
    /** Get the current enumeration. */
    virtual void GetCurrent(Value&) = 0;
    
    /** Move on to the next value. */
    virtual void Advance() = 0;

};

/* DummyEnumerator /////////////////////////////////////////////////////////////////////////////////
 *
 * An Enumerator that enumerates nothing. 
 */
class PIKA_API DummyEnumerator : public Enumerator
{
public:
    INLINE DummyEnumerator(Engine* eng) : Enumerator(eng) { }

    virtual ~DummyEnumerator() { }

    virtual bool Rewind() { return false; }
    virtual bool IsValid() { return false; }
    virtual void GetCurrent(Value&) { }
    virtual void Advance() { }

    static Enumerator* Create(Engine* eng);
};

}// pika


#endif
