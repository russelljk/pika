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
    
/** @brief String is a immutable array of characters.
  * Unlike C strings in Pika strings can contain arbitrary data including null characters.
  * To create a string call Engine::AllocString or one of its variants.
  */
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
    
    /** Do not call this directly unless you know what your doing and how strings behave in Pika. */
    static String* Create(Engine*, const char*, size_t, bool=false);
    
    /** String finalization is handled by the StringTable. */
    virtual bool Finalize();
    
    /** Returns a StringEnumerator that can enumerate over elements, indices, and empty the string. */
    virtual Enumerator* GetEnumerator(String*);
    
    virtual Type* GetType() const;
    virtual bool  GetSlot(const Value& key, Value& result);
    virtual bool  BracketRead(const Value& key, Value& result);
    virtual bool  SetSlot(const Value& key, Value& value, u4 attr = 0);
    virtual bool  CanSetSlot(const Value& key);
    virtual bool  DeleteSlot(const Value& key);
    
    INLINE size_t      GetLength()   const { return length;   }
    INLINE size_t      GetHashCode() const { return hashcode; }
    INLINE const char* GetBuffer()   const { return buffer;   }
    
    // ------------------------------------------------------------------------
    /** Concat two String's together. */
    static String* Concat(String* a, String* b);

    /** Concat two String's together with character separating them. */
    static String* ConcatSep(String* a, String* b, char sep);

    /** Concat two String's together with a space separating them. */    
    static String* ConcatSpace(String* a, String* b);
    
    /** Duplicate the String a given number of times. */
    static String* Multiply(String*, pint_t);
    
    /** Compare this String to another one. 
      * @result  
      *   0 If the two Strings are equal
      * < 0 If this String is less than the rhs String.
      * > 0 If this String is greather than the rhs String.
      */
    int Compare(const String* rhs) const;
    
    /** Splits this String by the delimiters and places them into an Array. */        
    Array* Split(String* delimiters);
    
    /** Returns a String with all letters converted to lower-case. */    
    String* ToLower();
    
    /** Returns a String with all letters converted to upper-case. */    
    String* ToUpper();
    
    /** Converts the String into a real or an integer. 
      * If the number cannot be converted null is returned. */
    Value ToNumber();
    
    /** Convets the String to an Integer. 
      * @result true is the conversion is successful false otherwise. */
    bool ToInteger(pint_t& res);
    
    /** Convets the String to a Real. 
      * @result true is the conversion is successful false otherwise. */
    bool ToReal(preal_t& res);
    
    /** Returns the sequence of elements that lie within the given range. */
    String* Slice(size_t from, size_t to);
    
    /** Returns this string with all the characters reversed. */
    String* Reverse();
    
    // ------------------------------------------------------------------------
    
    /** Determines if the String has embedded '\0' characters. */
    INLINE bool HasNulls() const { return strlen(buffer) != length; }
    
    static void StaticInitType(Engine* eng);    
protected:
    String*      next;      //!< Next String in the StringTable. <b>Do Not Modify</b>.
    size_t const length;    //!< Length of the String including all null characters except the terminating null.
    size_t const hashcode;  //!< Computed hashcode.
    char         buffer[1]; //!< Start of the buffer. The rest of the string is stored after the object.
};

}// pika

#endif
