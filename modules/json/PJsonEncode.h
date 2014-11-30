#ifndef PIKA_JSONENCODE_HEADER
#define PIKA_JSONENCODE_HEADER

namespace pika {

struct JsonEncoder {
    JsonEncoder(Engine*);
    virtual ~JsonEncoder();
    
    String* Encode(Value&);
protected:
    void EncodeArray(Array*);
    void EncodeDictionary(Dictionary*);
    void EncodeString(String*);
    void EncodeInteger(pint_t);
    void EncodeReal(preal_t);
    void EncodeNull();
    void EncodeTrue();
    void EncodeFalse();
    void EncodeKey(Value& key);
    void EncodeElement(Value& elem);
    
    void EncodeCString(const char* str, size_t len, bool escape=false);
    
    void InvalidKey(Value& key);
    void InvalidElement(Value& elem);
    void MarkCollection(Object*);
    
    Engine* engine;
    Table marked;
    Buffer<char> buffer;
};

}// pika

#endif
