/*
 *  PBasic.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_BASIC_HEADER
#define PIKA_BASIC_HEADER

#ifndef PIKA_VALUE_HEADER
#include "PValue.h"
#endif

#ifndef PIKA_COLLECTOR_HEADER
#include "PCollector.h"
#endif

#include "PClassInfo.h"

namespace pika 
{

class Engine;
class Enumerator;
class Type;

struct NamedConstant 
{
    const char* name;
    pint_t      value;
};

#define PIKA_CONST(N, C) {  N, (pint_t)C },
#define PIKA_ENUM(C)     { #C, (pint_t)C },

class PIKA_API Basic : public GCObject
{
    PIKA_REG(Basic)
public:
    INLINE Basic(Engine* eng) : engine(eng) {}

    virtual ~Basic() {}
    
    INLINE bool IsDerivedFrom(const ClassInfo *other) { return GetClassInfo()->IsDerivedFrom(other); }
    
    virtual Type* GetType() const = 0;
    virtual ClassInfo* GetClassInfo();
    static  ClassInfo* StaticCreateClass();
    
    virtual bool GetSlot(const Value& key, Value& result) { return false; }
    virtual bool SetSlot(const Value& key, Value& value, u4 attr = 0) { return false; }

    virtual bool CanSetSlot(const Value& key) { return true;  }
    virtual bool HasSlot(const Value& key)    { return false; }
    virtual bool DeleteSlot(const Value& key) { return false; }

    virtual Enumerator* GetEnumerator(class String* enum_kind);
    
    static void EnterConstants(Basic*, NamedConstant*, size_t);

    INLINE Engine* GetEngine() const { return engine; }

    // Perform a write barrier between this and the passed object.
    virtual void WriteBarrier(GCObject*);
    virtual void WriteBarrier(const Value&);
    
    // Create a new instance in response to 'new'.
    virtual void CreateInstance(Value& v) {v.SetNull();}

    // specializations
    template<typename TK>
    INLINE bool GetSlot(TK key, Value& result)
    {
        Value vkey(key);
        return GetSlot(vkey, result);
    }

    template<typename T>
    INLINE bool SetSlot(const char* key, T t, u4 attr = 0)
    {
        Value v(t);
        return SetSlot(key, v, attr);
    }

    template<typename TK, typename TV>
    INLINE bool SetSlot(TK key, TV t, u4 attr = 0)
    {
        Value vkey(key);
        Value v(t);
        return SetSlot(vkey, v, attr);
    }

    bool SetSlot(const char* key, Value& value, u4 attr = 0);
protected:
    Engine* engine;
};

INLINE void MarkValue(Collector* c, Value& v)
{
    ASSERT(v.tag < MAX_TAG);

    if (v.tag >= TAG_gcobj && v.val.gcobj)
        v.val.gcobj->Mark(c);
}

}// pika

#endif
