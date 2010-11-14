/*
 *  PByteArray.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PByteArray.h"

namespace pika {

// ByteArrayIterator ////////////////////////////////////////////////////////////////////////////////

class ByteArrayIterator : public Iterator
{
public:
    ByteArrayIterator(Engine* eng, Type* batype, ByteArray* byte_array) : 
        Iterator(eng, batype),
        valid(false),
        pos(0),
        byte_array(byte_array) 
    {
        valid = Rewind();
    }
    
    virtual ~ByteArrayIterator() {}
    
    bool ValidObject() const { return byte_array != 0; }
    
    bool IsAtEnd() const { return !(ValidObject() && (pos < byte_array->buffer.GetSize())); }
    
    virtual bool Rewind()
    {
        if (!ValidObject())
            return false;
        pos = 0;
        return true;
    }
    
    virtual bool ToBoolean() { return valid; }
    
    virtual int Next(Context* ctx)
    {
        if (pos < byte_array->buffer.GetSize())
        {
            u1 ch = byte_array->buffer[pos++];
            ctx->Push((pint_t)ch);
            valid = true;
            return 1;
        }
        else
        {
            valid = false;
        }
        return 0;
    }
    
    virtual void Advance()
    {
        if ( !IsAtEnd() )
        {
            ++pos;
        }
    }
    
    bool valid;
    size_t pos;
    ByteArray* byte_array;
};

// ByteArray //////////////////////////////////////////////////////////////////////////////////////////

PIKA_IMPL(ByteArray)

ByteArray::ByteArray(Engine* eng, Type* obj_type, u1* v, size_t len)
        : Object(eng, obj_type),
#if defined(PIKA_BIG_ENDIAN)
        byteOrder(BO_big),
#else
        byteOrder(BO_little),
#endif
        pos(0)
{
    if (v && len)
        InitializeWith(v, len);
}

ByteArray::ByteArray(const ByteArray* rhs) :
    ThisSuper(rhs),
    byteOrder(rhs->byteOrder),
    pos(rhs->pos),
    buffer(rhs->buffer)
{
}

Object* ByteArray::Clone()
{
    ByteArray* b = 0;
    GCNEW(engine, ByteArray, b, (this));
    return b;
}

Iterator* ByteArray::Iterate(String* iter_type)
{
    if (iter_type == engine->emptyString)
    {
        Iterator* e = 0;
        PIKA_NEW(ByteArrayIterator, e, (engine, engine->Iterator_Type, this));
        engine->AddToGC(e);
        return e;
    }
    return ThisSuper::Iterate(iter_type);
}

ByteArray::~ByteArray() {}

void ByteArray::Rewind() { pos = 0; }

void ByteArray::SetPosition(pint_t p)
{
    if (p >= 0 && p <= (pint_t)buffer.GetSize())
    {
        pos = p;
    }
    else
    {
        RaiseException("Attempt to set position beyond ByteArray' buffer");
    }
}

pint_t ByteArray::GetPosition() { return(pint_t)(pos); }
pint_t ByteArray::GetLength()   { return(pint_t)buffer.GetSize(); }

String* ByteArray::ToString()
{
    if ( buffer.GetSize() )
    {
        return engine->AllocString( (const char*)buffer.GetAt(0), buffer.GetSize() );
    }
    return engine->emptyString;
}

bool ByteArray::GetSlot(const Value& key, Value& res)
{
    if (key.IsInteger())
    {
        pint_t index = key.val.integer;
        if (index >= 0 && index < (pint_t)buffer.GetSize())
        {
            res.Set((pint_t)buffer[index]);
            return true;
        }
        else
        {
            return false;
        }
    }
    return ThisSuper::GetSlot(key, res);
}

bool ByteArray::SetSlot(const Value& key, Value& value, u4 attr)
{
    if (key.IsInteger())
    {
        switch (value.tag)
        {
        case TAG_null:
        {
            pint_t index = key.val.integer;
            SetPosition(index);
            WriteByte(0x00);
            return true;
        }
        case TAG_boolean:
        {
            pint_t index = key.val.integer;
            SetPosition(index);
            WriteByte(value.val.index ? 0x01 : 0x00);
            return true;
        }
        case TAG_integer:
        {
            pint_t index = key.val.integer;
            SetPosition(index);
            WriteByte((u1)(value.val.integer & 0xFF));
            return true;
        }
        case TAG_string:
        {
            pint_t index = key.val.integer;
            SetPosition(index);
            WriteString(value.val.str, false);
            return true;
        }
        default:
            return false;
        }
    }
    return ThisSuper::SetSlot(key, value, attr);
}

String* ByteArray::ReadStringLength(pint_t len)
{        
    if (len == 0)
    {
        return engine->emptyString;
    }
    else if (len < 0)
    {
        RaiseException("Cannot read string of negative length.");
    }
    
    if ((pos + (size_t)len) > (size_t)GetLength())
    {
        RaiseException("Attempt to read beyond length of a byte-array's buffer.");
    }
    String* res = engine->AllocString((const char*)buffer.GetAt(pos), len);
    pos += len;
    return res;
}

String* ByteArray::ReadStringAll()
{
    size_t end = buffer.GetSize();
    size_t amt = end - pos;
    ASSERT(end >= pos);
    String* res = 0;
    if (amt)
    {
        res = engine->AllocString((const char*)buffer.GetAt(pos), amt);
        pos = end;
    }
    else
    {
        res = engine->emptyString;
    }
    return res;
}

String* ByteArray::ReadString(Context* ctx)
{
    u2 argc = ctx->GetArgCount();
    
    
    if (argc == 0)
    {
        return ReadStringAll();
    }
    else if (argc == 1)
    {
        pint_t len = ctx->GetIntArg(0);
        return ReadStringLength(len);
    }
    else
    {
        ctx->WrongArgCount();
    }
    return engine->emptyString;
}

bool ByteArray::ReadBoolean()
{
    bool res = (ReadByte() != 0);
    return res;
}

u1 ByteArray::ReadByte()
{
    if (pos >= buffer.GetSize())
    {
        RaiseException("Attempt to read beyond byte-array's buffer.");
    }
    u1 res = buffer[pos++];
    return res;
}

u2 ByteArray::ReadWord()
{
    u1 a, b;
    
    if (IsBigEndian())
    {
        a = ReadByte();
        b = ReadByte();
    }
    else
    {
        b = ReadByte();
        a = ReadByte();
    }
    
    u2 res = ((u2)a << 8) | (u2)b;
    return res;
}

u4 ByteArray::ReadDword()
{
    u2 a, b;
    
    if (IsBigEndian())
    {
        a = ReadWord();
        b = ReadWord();
    }
    else
    {
        b = ReadWord();
        a = ReadWord();
    }
    
    u4 res = ((u4)a << 16) | (u4)b;
    return res;
}

u8 ByteArray::ReadQword()
{
    u4 a, b;
    
    if (IsBigEndian())
    {
        a = ReadDword();
        b = ReadDword();
    }
    else
    {
        b = ReadDword();
        a = ReadDword();
    }
    u8 res = ((u8)a << 32) | (u8)b;
    return res;
}

void ByteArray::InitializeWith(u1* v, size_t l)
{
    ASSERT(v);
    buffer.Resize(l);
    Pika_memcpy(buffer.GetAt(0), v, l);
    pos = 0;
}

void ByteArray::SetLength(ssize_t s)
{
    if (s == GetLength())
    {
        return;
    }
    else if (s < 0)
    {
        RaiseException("Attempt to resize byte-array with negative length.");
    }
    else if (s > PIKA_STRING_MAX_LEN)
    {
        RaiseException("Byte Array's length is too large.");
    }
    else if (s == 0)
    {
        buffer.Clear();
        pos = 0;
        return;
    }
    
    size_t sz  = s;
    size_t len = buffer.GetSize();
    buffer.SmartResize(sz);
    
    if (sz > len)
    {
        size_t diff = sz - len;
        Pika_memzero(buffer.GetAt(len), diff);
    }
    pos = Clamp<size_t>(pos, 0, buffer.GetSize() - 1);
}

ByteArray* ByteArray::Create(Engine* eng, Type* type, u1* v, size_t len)
{
    ByteArray* byte_array = 0;
    GCNEW(eng, ByteArray, byte_array, (eng, type, v, len));
    return byte_array;
}

// -----------------------------------------------------------------------------

#if defined(PIKA_64BIT_INT) 

void ByteArray::WriteInteger(pint_t x) { WriteQword(x); }

pint_t  ByteArray::ReadInteger()
{
    union 
    {
        u8 u;
        s8 s;
    } us;
    us.u = ReadQword();
    return us.s;
}

#else

void ByteArray::WriteInteger(pint_t x) { WriteDword(x); }

pint_t  ByteArray::ReadInteger()
{
    union 
    {
        u4 u;
        s4 s;
    } us;
    us.u = ReadDword();
    return us.s;
}

#endif /* PIKA_64BIT_INT */

// -----------------------------------------------------------------------------

#if defined(PIKA_64BIT_REAL) // 64 bit real numbers

void ByteArray::WriteReal(preal_t x)
{
    union RealToInt 
    {
        u8 i;
        preal_t r;
    } rti;
    rti.r = x;
    WriteQword(rti.i);
}

preal_t ByteArray::ReadReal()
{
    union 
    {
        u8 u;
        preal_t f;
    } us;
    
    us.u = ReadQword();
    return us.f;
}

#else

void ByteArray::WriteReal(preal_t x)
{
    union RealToInt 
    {
        u4 i;
        preal_t r;
    } rti;
    rti.r = x;
    WriteDword(rti.i);
}

preal_t ByteArray::ReadReal()
{
    union 
    {
        u4 u;
        preal_t f;
    } us;
    us.u = ReadDword();
    return us.f;
}

#endif /* PIKA_64BIT_REAL */

void ByteArray::Write(Value v)
{
    switch (v.tag)
    {
    case TAG_null:      WriteByte(0);                    break;
    case TAG_boolean:   WriteBoolean(v.val.index != 0);  break;
    case TAG_integer:   WriteInteger(v.val.integer);     break;
    case TAG_real:      WriteReal(v.val.real);           break;
    case TAG_string:    WriteString(v.val.str, true); break;
    default:
        RaiseException("Attempt to write unsupported type");
    }
}

void ByteArray::SmartResize(size_t sizeneeded)
{
    if (sizeneeded > buffer.GetSize())
    {
        size_t cap = buffer.GetCapacity();
        
        if (sizeneeded > cap)
        {
            size_t cap_2 = cap * 2;
            size_t newcap = Max<size_t>(cap_2, sizeneeded);
            if (newcap >= PIKA_BUFFER_MAX_LEN)
                RaiseException("Maximum ByteArray length reached.");
            buffer.SetCapacity(newcap);
        }
    }
    SetLength((s4)sizeneeded);
}

void ByteArray::WriteString(String* str, bool resize)
{
    size_t sizeneeded = pos + str->GetLength();
    
    if (sizeneeded > buffer.GetSize())
    {
        if (resize)
        {
            SetLength(sizeneeded);
        }
        else
        {
            RaiseException("not enough room to write string");
        }
    }
    // c strings are always stored in the same byteorder so we can just
    // copy it directly to the buffer.
    Pika_memcpy(buffer.GetAt(pos), str->GetBuffer(), str->GetLength());
    pos = sizeneeded;
}

void ByteArray::WriteBoolean(bool b) { WriteByte(b ? 0x01 : 0x00); }

void ByteArray::WriteByteAt(u1 u, size_t at)
{
    if (at >= buffer.GetSize())
    {
        RaiseException("cannot write beyond ByteArray buffer");
    }
    buffer[at] = u;
}

void ByteArray::WriteByte(u1 u)
{
    if (pos == buffer.GetSize())
    {
        buffer.Push(u);
    }
    buffer[pos++] = u;
}

void ByteArray::WriteWord(u2 u)
{
    if (IsBigEndian())
    {
        WriteByte((u1)((u >> 8) & 0xFF));
        WriteByte((u1)((u)      & 0xFF));
    }
    else
    {
        WriteByte((u1)((u)      & 0xFF));
        WriteByte((u1)((u >> 8) & 0xFF));
    }
}

void ByteArray::WriteDword(u4 u)
{
    if (IsBigEndian())
    {
        WriteWord((u2)((u >> 16) & 0xFFFF));
        WriteWord((u2)((u)       & 0xFFFF));
    }
    else
    {
        WriteWord((u2)((u)       & 0xFFFF));
        WriteWord((u2)((u >> 16) & 0xFFFF));
    }
}

void ByteArray::WriteQword(u8 u)
{
    if (IsBigEndian())
    {
        WriteDword((u4)((u >> 32) & 0xFFFFFFFF));
        WriteDword((u4)((u)       & 0xFFFFFFFF));
    }
    else
    {
        WriteDword((u4)((u)       & 0xFFFFFFFF));
        WriteDword((u4)((u >> 32) & 0xFFFFFFFF));
    }
}

void ByteArray::Init(Context* ctx)
{
    u2 argc = ctx->GetArgCount();
    if ( argc == 1 )
    {
        Value& arg0 = ctx->GetArg(0);        
        if (arg0.IsInteger())
        {
            pint_t i = arg0.val.integer;
            SetLength(i);
        }
        else
        {
            String* str = ctx->GetStringArg(0);
            InitializeWith((u1*)str->GetBuffer(), str->GetLength());
        }
    }
    else if (argc != 0)
    {
        ctx->WrongArgCount();
    }
}

Object* ByteArray::Slice(pint_t from, pint_t to)
{
    // TODO: handle negative range by reversing elements.
    ByteArray* byte_array = 0;
    
    if (from > to || from < 0 || to < 0 || to > (pint_t)buffer.GetSize())
    {
        return 0;
    }
    
    size_t amt = to - from;
    if (!amt)
    {
        byte_array = ByteArray::Create(engine, GetType(), 0, 0);
    }
    else
    {
        byte_array = ByteArray::Create(engine, GetType(), buffer.GetAt(from), amt);
    }
    return byte_array;
}

int ByteArray_nextBytes(Context* ctx, Value& self)
{
    ByteArray* ba = static_cast<ByteArray*>(self.val.object);
    pint_t nbytes = ctx->GetIntArg(0);
    if (nbytes > PIKA_MAX_RETC)
    {
        RaiseException("Attempt to return too many values; limit of %u values exceeded", PIKA_MAX_RETC);
    }
    else if (nbytes < 0)
    {
        RaiseException("Cannot return negative number ("PINT_FMT") of values.", nbytes);
    }
    else if (nbytes == 0)
    {
        return 0;
    }
    else if ((ba->GetPosition() + nbytes) >= ba->GetLength())
    {
        RaiseException("Attempt to read ("PINT_FMT") bytes beyond the end of the byte-array.", ((ba->GetPosition() + nbytes) - ba->GetLength()) + 1);
    }
    else
    {
        u4 num = static_cast<u4>(nbytes);
        for (u4 a = 0; a < num; ++a)
        {
            u1 byte = ba->ReadByte();
            ctx->Push((pint_t)byte);
        }
        return nbytes;
    }
    return 0;
}

void ByteArray::Constructor(Engine* eng, Type* type, Value& res)
{
    Object* ba = ByteArray::Create(eng, type, 0, 0);
    res.Set(ba);
}

void ByteArray::StaticInitType(Engine* eng)
{
    Package* Pkg_World = eng->GetWorld();
    String* ByteArray_String = eng->AllocString("ByteArray");
    eng->ByteArray_Type = Type::Create(eng, ByteArray_String, eng->Object_Type, ByteArray::Constructor, Pkg_World);
    
    SlotBinder<ByteArray>(eng, eng->ByteArray_Type)
    .Method(&ByteArray::Rewind,             "rewind")
    .Method(&ByteArray::Write,              "write")
    .Method(&ByteArray::ReadBoolean,        "readBoolean")
    .Method(&ByteArray::ReadInteger,        "readInteger")
    .Method(&ByteArray::ReadReal,           "readReal")
    .Method(&ByteArray::ReadByte,           "readByte")
    .Method(&ByteArray::ReadWord,           "readWord")
    .Method(&ByteArray::ReadDword,          "readDword")
    .RegisterMethod(ByteArray_nextBytes,    "nextBytes")
    .MethodVA(&ByteArray::ReadString,       "readString")
    .Method(&ByteArray::Slice,              OPSLICE_STR)
    .Constant((pint_t)ByteArray::BO_big,    "BIG")
    .Constant((pint_t)ByteArray::BO_little, "LITTLE")
    .PropertyRW("endian",
                &ByteArray::GetEndian,      "getEndian",
                &ByteArray::SetEndian,      "setEndian")
    .PropertyRW("position",
                &ByteArray::GetPosition,    "GetPosition",
                &ByteArray::SetPosition,    "SetPosition")
    .PropertyRW("length",
                &ByteArray::GetLength,      "getLength",
                &ByteArray::SetLength,      "setLength");
                
    Pkg_World->SetSlot(ByteArray_String, eng->ByteArray_Type);
}

}// pika
