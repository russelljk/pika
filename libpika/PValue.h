/*
 *  PValue.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_VALUE_HEADER
#define PIKA_VALUE_HEADER

#include "PConfig.h"
#include "PUtil.h"
#include "PError.h"

#if defined(PIKA_64)
#   if defined(PIKA_64BIT_INT) && defined (PIKA_64BIT_REAL)
#       define CLEAR_BITS()
#   else
#       define CLEAR_BITS() val.index = 0
#   endif
#else
#   define CLEAR_BITS()
#endif

namespace pika
{

class String;
class Def;
class Context;
class Enumerator;
class Object;
class Type;
class Package;
class Property;
class UserData;
class Basic;
class Function;
class Array;
class ClassInfo;
class GCObject;
struct UserDataInfo;

enum ValueTag {
/*  0  */ TAG_null,
/*  1  */ TAG_boolean,
/*  2  */ TAG_integer,
/*  3  */ TAG_real,
/*  4  */ TAG_index,
/*  5  */ TAG_gcobj,
/*  6  */ TAG_def,
/*  7  */ TAG_string,
/*  -  */ TAG_basic = TAG_string,
/*  8  */ TAG_enumerator,
/*  9  */ TAG_property,
/* 10  */ TAG_userdata,
/* 11  */ TAG_object,
/* 12  */ MAX_TAG,
};

PIKA_API extern const char* GetTypeString(u2 e);

enum NullEnum { NULL_VALUE = 0 };

class PIKA_API Value
{
public:
    INLINE  Value() {}
    INLINE  Value(NullEnum)         : tag(TAG_null)       { val.index = 0; }
    INLINE  Value(pint_t i)         : tag(TAG_integer)    { CLEAR_BITS(); val.integer = i; }
    INLINE  Value(preal_t r)        : tag(TAG_real)       { CLEAR_BITS(); val.real = r; }
    INLINE  Value(Object* c)        : tag(TAG_object)     { val.object = c; }
    INLINE  Value(String* s)        : tag(TAG_string)     { val.str = s; }
    INLINE  Value(bool b)           : tag(TAG_boolean)    { val.index = (b) ? 1 : 0; }
    INLINE  Value(size_t i)         : tag(TAG_index)      { val.index = i; }
    INLINE  Value(Def* f)           : tag(TAG_def)        { val.def = f; }
    INLINE  Value(Enumerator* e)    : tag(TAG_enumerator) { val.enumerator = e; }
    INLINE  Value(Property* p)      : tag(TAG_property)   { val.property = p; }
    INLINE  Value(UserData* u)      : tag(TAG_userdata)   { val.userdata = u; }
    
    INLINE bool IsBoolean()     const { return  tag == TAG_boolean; }
    INLINE bool IsNull()        const { return  tag == TAG_null; }
    INLINE bool IsInteger()     const { return  tag == TAG_integer; }
    INLINE bool IsReal()        const { return  tag == TAG_real; }
    INLINE bool IsCollectible() const { return  tag >= TAG_gcobj; } //!< Is the object collectible.
    INLINE bool IsString()      const { return  tag == TAG_string; }
    INLINE bool IsFunction()    const { return  tag == TAG_def; }
    INLINE bool IsEnumerator()  const { return  tag == TAG_enumerator; }
    INLINE bool IsProperty()    const { return  tag == TAG_property; }
    INLINE bool IsUserData()    const { return  tag == TAG_userdata; }
    INLINE bool IsObject()      const { return (tag == TAG_object) && val.object; }
    
    bool IsDerivedFrom(ClassInfo* c) const;
    
    INLINE pint_t      GetInteger()    const { return val.integer; }
    INLINE preal_t     GetReal()       const { return val.real; }
    INLINE bool        GetBoolean()    const { return val.index != 0; }
    INLINE Object*     GetObject()     const { return val.object; }
    INLINE String*     GetString()     const { return val.str; }
    INLINE Def*        GetFunction()   const { return val.def; }
    INLINE Enumerator* GetEnumerator() const { return val.enumerator; }
    INLINE size_t      GetIndex()      const { return val.index; }
    
    void* GetUserData(UserDataInfo* info) const;
    void* GetUserDataFast() const;
    bool  HasUserData(UserDataInfo* info) const;
    
    INLINE void Set(pint_t i)      { tag = TAG_integer; CLEAR_BITS(); val.integer = i; }
    INLINE void Set(preal_t r)     { tag = TAG_real;    CLEAR_BITS(); val.real    = r; }
    INLINE void Set(size_t i)      { tag = TAG_index;       val.index      = i; }
    INLINE void Set(Object* c)     { tag = TAG_object;      val.object     = c; }
    INLINE void Set(String* s)     { tag = TAG_string;      val.str        = s; }
    INLINE void Set(Def* f)        { tag = TAG_def;         val.def        = f; }
    INLINE void Set(Enumerator* e) { tag = TAG_enumerator;  val.enumerator = e; }
    INLINE void Set(Property* p)   { tag = TAG_property;    val.property   = p; }
    INLINE void Set(UserData* u)   { tag = TAG_userdata;    val.userdata   = u; }
    
    INLINE void SetNull()       { tag = TAG_null;    val.index = 0; }
    INLINE void SetTrue()       { tag = TAG_boolean; val.index = 1; }
    INLINE void SetFalse()      { tag = TAG_boolean; val.index = 0; }
    INLINE void SetBool(bool b) { tag = TAG_boolean; val.index = b; }
    
    INLINE int GetTag() const { return tag; }
    
    friend INLINE bool operator==(const Value& a, const Value& b)
    {
#if defined(PIKA_32) && !defined(PIKA_64) && defined(PIKA_64BIT_REAL)
        if (a.tag != b.tag)
        {
            return false;
        }
        if (a.tag == TAG_real)
        {
            return a.val.real == b.val.real;
        }
        return (a.val.index == b.val.index);
#else
        return (a.tag == b.tag) && (a.val.index == b.val.index);
#endif
    }
    
    union {
        pint_t      integer;
        preal_t     real;
        size_t      index;
        Def*        def;
        Basic*      basic;
        String*     str;
        Object*     object;
        Type*       type;
        Package*    package;
        Context*    context;
        Property*   property;
        UserData*   userdata;
        GCObject*   gcobj;
        Function*   function;
        Enumerator* enumerator;
    }
    val;
    int tag;
};

class PIKA_API ScriptException : public Exception
{
public:
    ScriptException(const Value& v)
        : Exception(Exception::ERROR_script), var(v) {}
        
    virtual ~ScriptException() {}
    
    virtual const char* GetMessage() const;
    
    Value var;
};

PIKA_NODESTRUCTOR(Value)

}// pika

#endif
