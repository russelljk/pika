/*
 *  PByteArray.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_BYTEARRAY_HEADER
#define PIKA_BYTEARRAY_HEADER

namespace pika {
/** 
 * A mutable binary string object.
 *
 * Unlike the string objects bytes objects can be resized and modified without forcing a new object
 * to be created.
 *
 * Like the string objects, bytes objects can include arbitrary binary data including nulls.
 * The member variable 'byteorder' specifies the order that data is read and written.
 * 'byteorder' defaults to whatever the system byteorder is.
 *
 */
class ByteArray : public Object
{
    PIKA_DECL(ByteArray, Object)
    
    enum EByteOrder
    {
        BO_little,
        BO_big,
    };
protected:
    ByteArray(Engine*, Type*, u1*, size_t);
    ByteArray(const ByteArray*);
    
    friend class ByteArrayEnumerator;
public:
    virtual ~ByteArray();
    
    virtual Object*     Clone();
    virtual Enumerator* GetEnumerator(String*);
    virtual String*     ToString();
    
    virtual bool   SetSlot(const Value& key, Value& value, u4 attr = 0);
    virtual bool   GetSlot(const Value& key, Value& res);
    
    virtual void   Rewind();
    virtual void   SetPosition(pint_t);
    virtual pint_t GetPosition();
    virtual pint_t GetLength();
    virtual void   SetLength(ssize_t);
    
    virtual String* ReadString(Context*);
    virtual String* ReadStringLength(pint_t len);
    virtual String* ReadStringAll();
    virtual preal_t ReadReal();    
    virtual bool    ReadBoolean();
    virtual pint_t  ReadInteger();        
    virtual u1      ReadByte();
    virtual u2      ReadWord();
    virtual u4      ReadDword();
    virtual u8      ReadQword();
    
    virtual void  WriteString(String* s, bool resize);    
    virtual void  WriteReal(preal_t);
    virtual void  WriteBoolean(bool);
    virtual void  Write(Value);
    virtual void  WriteInteger(pint_t);        
    virtual void  WriteByte(u1);
    virtual void  WriteWord(u2);
    virtual void  WriteDword(u4);
    virtual void  WriteQword(u8);
    virtual void  WriteByteAt(u1 u, size_t at);
        
    virtual void    Init(Context*);
    virtual Object* Slice(pint_t, pint_t);

    INLINE void SetEndian(pint_t endian)
    {
        if ((endian != (pint_t)BO_little) && (endian != (pint_t)BO_big))
        {
            RaiseException("invalid byte order specified");
        }
        byteOrder = (EByteOrder)endian;
    }
    
    INLINE pint_t  GetEndian()      const { return byteOrder; }
    INLINE bool   IsLittleEndian() const { return byteOrder == BO_little; }
    INLINE bool   IsBigEndian()    const { return byteOrder == BO_big;    }
    
    static ByteArray* Create(Engine*, Type*, u1*, size_t);
    static void Constructor(Engine* eng, Type* obj_type, Value& res);
    static void StaticInitType(Engine* eng);
protected:
    virtual void  InitializeWith(u1*, size_t);
    virtual void  SmartResize(size_t sizeneeded);
    
    EByteOrder  byteOrder;
    size_t      pos;
    Buffer<u1>  buffer;
};

}// pika

#endif
