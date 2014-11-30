#include "PJson.h"
#include "PJsonEncode.h"

namespace pika {

JsonEncoder::JsonEncoder(Engine* eng) : engine(eng)
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
        RaiseException(Exception::ERROR_runtime, "Attempt to encode Object more than once.");
    }
    marked.Set(val, val);
}

void JsonEncoder::EncodeArray(Array* array)
{
    buffer.Push('[');
    Buffer<Value>& elements = array->GetElements();
    Buffer<Value>::Iterator iter = elements.Begin();
    
    while(iter != elements.End())
    {
        this->EncodeElement(*iter);
        ++iter;
        if (iter != elements.End())
        {
            buffer.Push(',');
            buffer.Push('\n');
        }
    }
    buffer.Push(']'); 
}

void JsonEncoder::EncodeDictionary(Dictionary* dict)
{
    buffer.Push('{');
    buffer.Push('\n');
    
    Table& elements = dict->Elements();
    for (Table::Iterator iter = elements.GetIterator(); iter; )
    {
        this->EncodeKey(iter->key);
        buffer.Push(':');
        this->EncodeElement(iter->val);
                
        if (++iter)
        {
            buffer.Push(',');
            buffer.Push('\n');
        }
    }
    buffer.Push('}');
}

void JsonEncoder::EncodeKey(Value& key)
{
    if (key.IsString())
    {
        this->EncodeString(key.val.str);
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

void JsonEncoder::EncodeCString(const char* str, size_t len, bool escape)
{
    size_t idx = buffer.GetSize();
    size_t const newSize = idx + len;
    if (escape)
    {
        buffer.SetCapacity(newSize);
        for (size_t i = idx; i < newSize; ++i)
        {
            // TODO: handle non-ascii (unicode) characters.
            int x = str[i];
            switch(x)
            {
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
            case '\"':
            case '\\':
            case '/':
                buffer.Push('\\');
                break;
            }
            buffer.Push(x);
        }
    }
    else
    {
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

}// pika
