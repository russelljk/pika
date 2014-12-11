/*
 *  PJsonDecode.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_JSONDECODE_HEADER
#define PIKA_JSONDECODE_HEADER

namespace pika {

enum JsonTokenKind {
    JsonTokMin = 257, 
    JsonTokInteger,
    JsonTokNumber,
    JsonTokString,
    JsonTokTrue,
    JsonTokFalse,
    JsonTokNull,
    JsonTokBadIdentifier,
    JsonTokMax
};

struct JsonToken {
    union {
        struct {
            char*  buffer;
            size_t length;
        }      str;
        s8     integer;
        double number;
        int    ch;
    }   val;
    int kind;
    int line;
    int col;
};

struct JsonStringStream {
    JsonStringStream(const char* cstr, size_t length);
    
    virtual ~JsonStringStream();
    
    virtual int  Get();
    virtual int  Peek();
    virtual bool IsEof();

private:
    Buffer<char> buffer;
    size_t pos;
};

struct JsonTokenizer {
    JsonTokenizer(String* str);
    
    virtual ~JsonTokenizer();
    
    void GetNext();
    void GetLook();
    bool IsEof();
    void SyntaxError(const char* msg);
    void EatWhitespace();
    
    int look;
    int line;
    int col;
    JsonStringStream stream;
    Buffer<char> jsonBuffer;
    Buffer<JsonToken> tokens;
    JsonToken token;
};

struct JsonParser {
    JsonParser(Engine* eng, JsonTokenizer* tokenizer);
    virtual ~JsonParser();
    
    Value       Parse();
protected:    
    Array*      DoArray();
    Dictionary* DoObject();
    String*     DoString(bool optional=false);
    Value       DoValue();
    Value       DoNumber();
    
    int     GetTokenKind();
    bool    IsTokenKind(int x);
    void    Unexpected();
    void    Expected(int x, bool unexpected=false);
    void    Match(int);
    bool    Optional(int);
    void    GetNext();
    
    Engine* engine;
    JsonTokenizer* tokenizer;
};

}// pika

#endif
