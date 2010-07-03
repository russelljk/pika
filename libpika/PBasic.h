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

namespace pika {

class Engine;
class Enumerator;
class Type;

struct NamedConstant 
{
    const char* name; // Name of the constant.
    pint_t value;     // Integer value the constant represents.
};

#define PIKA_CONST(N, C) {  N, (pint_t)C },
#define PIKA_ENUM(C)     { #C, (pint_t)C },

/** A Basic script object that can have an associated type, slot table and 
  * can be enumerated. By default Basic has none of these but instead provides
  * the interface to access them. 
  */
class PIKA_API Basic : public GCObject
{
    PIKA_REG(Basic)
public:
    INLINE Basic(Engine* eng) : engine(eng) {}

    virtual ~Basic() {}
    
    INLINE bool IsDerivedFrom(const ClassInfo *other) { return GetClassInfo()->IsDerivedFrom(other); }
           
    virtual ClassInfo* GetClassInfo();
    static  ClassInfo* StaticCreateClass();
    
    static  void EnterConstants(Basic*, NamedConstant*, size_t);
    
    /** Returns the Type for this Basic Object.
      * @note   Derived classes must override.
      */
    virtual Type* GetType() const = 0;
    
    virtual void AddFunction(Function* f);    
    virtual void AddProperty(Property* p); 
    virtual bool GetSlot(const Value& key, Value& result) { return false; }
    virtual bool SetSlot(const Value& key, Value& value, u4 attr = 0) { return false; }
    
    virtual bool BracketRead(const Value& key, Value& result) { return false; }
    virtual bool BracketWrite(const Value& key, Value& value, u4 attr = 0) { return false; }
    
    virtual bool CanSetSlot(const Value& key) { return true;  }
    virtual bool HasSlot(const Value& key)    { return false; }
    virtual bool DeleteSlot(const Value& key) { return false; }

    virtual Enumerator* GetEnumerator(class String* enum_kind);
    
    INLINE Engine* GetEngine() const { return engine; }

    /** Perform a write barrier between this and the passed object.
      * Any time a GCObject obtains a <b><i>new</i></b> reference to another GCObject it
      * must perform a WriteBarrier to mantain the invarant that no black-list object holds a 
      * reference to a white-list object.
      *
      * Most of the time this is handled for you <i>if</i> you use the SetSlot methods or
      * any access methods like SetXXXX where XXXX is the name of a attribute.
      */
    virtual void WriteBarrier(GCObject*);

    /** Perform a write barrier between this and the passed value.
      */
    virtual void WriteBarrier(const Value&);
    
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

}// pika

#endif
