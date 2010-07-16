/*
 *  PString.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_STRING_HEADER
#define PIKA_STRING_HEADER

#ifndef PIKA_BASIC_HEADER
#   include "PBasic.h"
#endif

namespace pika {
class StringTable;
class StringApi;
class StringEnumerator;
class Array;

class PIKA_API String : public Basic
{
    PIKA_DECL(String, Basic)
protected:
    friend class StringTable;
    friend class StringApi;
    friend class StringEnumerator;
    
    String(Engine*, size_t, const char*);
public:
    virtual ~String();
    
    // Do not call this directly unless you know what your doing and how strings behave in Pika
    static String*      Create(Engine*, const char*, size_t, bool=false);
    
    virtual bool        Finalize();
    
    virtual Enumerator* GetEnumerator(String*);
    virtual Type*       GetType() const;
    virtual bool        GetSlot(const Value& key, Value& result);
    virtual bool        BracketRead(const Value& key, Value& result);
    virtual bool        SetSlot(const Value& key, Value& value, u4 attr = 0);
    virtual bool        CanSetSlot(const Value& key);
    virtual bool        DeleteSlot(const Value& key);
    
    INLINE size_t       GetLength()   const { return length;   }
    INLINE size_t       GetHashCode() const { return hashcode; }
    INLINE const char*  GetBuffer()   const { return buffer;   }
    
    // ----- api ---------------------------------------------------------------
    
    static String*      Concat(String* a, String* b);
    static String*      ConcatSep(String* a, String* b, char sep);
    static String*      ConcatSpace(String* a, String* b);
    static String*      Multiply(String*, pint_t);
        
    int                 Compare(const String* s) const;
    Array*              Split(String* delimiters);
    String*             ToLower();
    String*             ToUpper();
    Value               ToNumber();
    bool                ToInteger(pint_t& res);
    bool                ToReal(preal_t& res);
    String*             Slice(size_t from, size_t to);
    String*             Reverse();
    // -------------------------------------------------------------------------
    
    INLINE bool         HasNulls() const { return strlen(buffer) != length; }
protected:
    String*             next;
    size_t const        length;
    size_t const        hashcode;
    char                buffer[1];
};

}// pika

void InitStringAPI(Engine*);

#endif
