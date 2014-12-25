/*
 *  PJsonEncode.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PJson.h"
#include "PJsonEncode.h"

namespace pika {
    
JsonEncoder::JsonEncoder(Engine* eng) : engine(eng), depth(0), tabsize(4)
{
}

JsonEncoder::~JsonEncoder()
{
}

void JsonEncoder::MarkCollection(Object* obj)
{
    Value val(obj);
    if (marked.Exists(val))
    {
        RaiseException(JsonEncodeError::StaticGetClass(), "Attempt to encode an Object more than once.");
    }
    marked.Set(val, val);
}

void JsonEncoder::EncodeArray(Array* array)
{
    buffer.Push('[');
    ++depth;
    this->Newline();    
    Buffer<Value>& elements = array->GetElements();
    Buffer<Value>::Iterator iter = elements.Begin();
    while(iter != elements.End())
    {
        this->EncodeElement(*iter);
        ++iter;
        if (iter != elements.End())
        {
            buffer.Push(',');
            this->Newline();
        }
    }
    --depth;
    this->Newline();
    buffer.Push(']'); 
}

void JsonEncoder::EncodeDictionary(Dictionary* dict)
{
    buffer.Push('{');
    ++depth;
    this->Newline();
    Table& elements = dict->Elements();
    for (Table::Iterator iter = elements.GetIterator(); iter; )
    {
        this->EncodeKey(iter->key);
        buffer.Push(':'); buffer.Push(' ');
        this->EncodeElement(iter->val);
                
        if (++iter)
        {
            buffer.Push(',');
            this->Newline();
        }
    }
    --depth;
    this->Newline();
    buffer.Push('}');
}

void JsonEncoder::Newline()
{
    buffer.Push('\n');
    for (size_t i =0; i < depth * tabsize; ++i)
    {
        buffer.Push(' ');
    }
}

void JsonEncoder::EncodeKey(Value& key)
{
    if (key.IsString())
    {
        this->EncodeString(key.val.str);
        return;
    }
    this->InvalidKey(key);
}

void JsonEncoder::EncodeElement(Value& elem)
{
    switch(elem.tag)
    {
    case TAG_null:
        this->EncodeNull();
        return;
   
    case TAG_boolean:
        if (elem.val.index)
        {
            this->EncodeTrue();
        }
        else
        {
            this->EncodeFalse();
        }
        return;
    
    case TAG_integer:
        this->EncodeInteger(elem.val.integer);
        return;
    
    case TAG_real:
        this->EncodeReal(elem.val.real);
        return;
    
    case TAG_string:
        this->EncodeString(elem.val.str);
        return;
    // case TAG_property:
    // TODO: call property
    
    case TAG_object:
        if (elem.IsDerivedFrom(Array::StaticGetClass()))
        {
            Array* array = static_cast<Array*>(elem.val.object);
            this->EncodeArray(array);
            return;
        }
        else if (elem.IsDerivedFrom(Dictionary::StaticGetClass()))
        {
            Dictionary* dict = static_cast<Dictionary*>(elem.val.object);
            this->EncodeDictionary(dict);
            return;
        }
    }
    this->InvalidElement(elem);
}

void JsonEncoder::InvalidKey(Value& key)
{
    String* type = engine->GetTypenameOf(key);
    RaiseException(Exception::ERROR_type, "Attempt to encode key of type '%s'.", type->GetBuffer());
}

void JsonEncoder::InvalidElement(Value& elem)
{
    String* type = engine->GetTypenameOf(elem);
    RaiseException(Exception::ERROR_type, "Attempt to encode element of type '%s'.", type->GetBuffer());    
}

void JsonEncoder::EncodeInteger(pint_t i)
{
    Value v(i);
    String* str = Engine::NumberToString(engine, v);
    this->EncodeCString(str->GetBuffer(), str->GetLength());
}

void JsonEncoder::EncodeReal(preal_t r)
{
    Value v(r);
    String* str = Engine::NumberToString(engine, v);    
    this->EncodeCString(str->GetBuffer(), str->GetLength());
}

void JsonEncoder::EncodeString(String* str)
{
    buffer.Push('\"');
    this->EncodeCString(str->GetBuffer(), str->GetLength(), true);
    buffer.Push('\"');
}

void JsonEncoder::BadUnicodeLiteral()
{
    RaiseException(JsonEncodeError::StaticGetClass(), "Attempt to encode string with an invalid utf-8 character sequence or non-ascii characters.");
}

void JsonEncoder::EncodeCString(const char* str, size_t len, bool escape)
{
    size_t idx = buffer.GetSize();
    size_t const newSize = idx + len;
    if (escape)
    {
        /* If this was originally a Pika String instance, we will need to escape it
         * before writing the string.
         */
        buffer.SetCapacity(newSize);
        for (size_t i = 0; i < len; ++i)
        {
            int x = str[i];
            static const char* HEX_TO_CHAR = "0123456789ABCDEF";
            
            if (x >= 0x80)
            {
                trace0("utf8");
                /* Not an ascii character.
                 * This means it's either a utf-8 sequence or the string contains
                 * arbitrary data.
                 *
                 * If the later, it can't be converted to JSON anyway. So we will
                 * attempt to interpret it as a utf-8 and convert to an escaped utf-32 
                 * character.
                 */
                size_t uch = this->ConvertFromUTF8(str + i, len - i);
                
                buffer.Push('\\');
                buffer.Push('u');
                
                u4 ch1 = (uch & 0xff);
                u4 ch2 = ((uch >>  8) & 0xff);
                u4 ch3 = ((uch >> 16) & 0xff);
                u4 ch4 = ((uch >> 24) & 0xff);
                
                buffer.Push(HEX_TO_CHAR[ch4 % 16]);
                buffer.Push(HEX_TO_CHAR[ch3 % 16]);
                buffer.Push(HEX_TO_CHAR[ch2 % 16]);
                buffer.Push(HEX_TO_CHAR[ch1 % 16]);
            }
            else
            {
                switch(x)
                {
                case '\b':
                    buffer.Push('\\');
                    buffer.Push('b');
                    break;
                case '\f':
                    buffer.Push('\\');
                    buffer.Push('f');
                    break;
                case '\n':
                    buffer.Push('\\');
                    buffer.Push('n');
                    break;
                case '\r':
                    buffer.Push('\\');
                    buffer.Push('r');
                    break;
                case '\t':
                    buffer.Push('\\');
                    buffer.Push('t');
                    break;
                case '\"':
                    buffer.Push('\\');
                    buffer.Push('\"');
                    break;
                case '\\':
                    buffer.Push('\\');
                    buffer.Push('\\');
                    break;                
                case '/':
                    buffer.Push('\\');
                    buffer.Push('/');
                    break;
                default:
                    buffer.Push(x);
                }

            }
        }
    }
    else
    {
        /* An integer or real or keyword converted. */
        buffer.Resize(newSize);
        Pika_memcpy(buffer.GetAt(idx), str, len);
    }
}

void JsonEncoder::EncodeNull()
{
    this->EncodeCString("null", 4);
}

void JsonEncoder::EncodeTrue()
{
    this->EncodeCString("true", 4);
}

void JsonEncoder::EncodeFalse()
{
    this->EncodeCString("false", 4);
}

String* JsonEncoder::Encode(Value& val)
{
    if (val.IsDerivedFrom(Array::StaticGetClass()))
    {
        Array* array = static_cast<Array*>(val.val.object);
        this->EncodeArray(array);
    }
    else if (val.IsDerivedFrom(Dictionary::StaticGetClass()))
    {
        Dictionary* dict = static_cast<Dictionary*>(val.val.object);
        this->EncodeDictionary(dict);
    }
    size_t length = buffer.GetSize();
    buffer.Push('\0');
    String* res = engine->GetString(buffer.GetAt(0), length);
    return res;
}

size_t JsonEncoder::ConvertFromUTF8(const char* utf8, size_t length)
{
    int ch1 = utf8[0];
    if (ch1 < 0xC2)
    {
        this->BadUnicodeLiteral();
    }
    else if (ch1 < 0xE0)
    {
        /* 2-byte utf8 character code. */
        if (length < 2) 
        {
            this->BadUnicodeLiteral();
        }
    
        int ch2 = utf8[1];
    
        if ((ch2 & 0xC0) != 0x80)
        {
            this->BadUnicodeLiteral();
        }
    
        return (ch1 << 6) + ch2 - 0x3080;
    }
    else if (ch1 < 0xF0)
    {
        /* 3-byte utf8 character code. */
        if (length < 3)
        {
            this->BadUnicodeLiteral();
        }
    
        int ch2 = utf8[1];
        int ch3 = utf8[2];
    
        if (((ch2 & 0xC0) != 0x80) || 
            (ch1 == 0xE0 && ch2 < 0xA0) || 
            ((ch3 & 0xC0) != 0x80))
        {
            this->BadUnicodeLiteral();
        }
        return (ch1 << 12) + (ch2 << 6) + ch3 - 0xE2080;
    }
    else if (ch1 < 0xF5)
    {
        /* 4-byte utf8 character code. */
        int ch2 = utf8[1];
        int ch3 = utf8[2];
        int ch4 = utf8[3];
        
        if (((ch2 & 0xC0) != 0x80) ||
            (ch1 == 0xF0 && ch2 < 0x90) ||
            (ch1 == 0xF4 && ch2 >= 0x90) ||
            ((ch3 & 0xC0) != 0x80) ||
            ((ch4 & 0xC0) != 0x80))
        {
            this->BadUnicodeLiteral();
        }

        return (ch1 << 18) + (ch2 << 12) + (ch3 << 6) + ch4 - 0x3C82080;
    }
    this->BadUnicodeLiteral();
    return 0; // Compiler Warning
}

}// pika
