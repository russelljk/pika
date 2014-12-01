#include "PJson.h"
#include "PJsonDecode.h"

namespace pika {

struct JsonKeywordDef {
    const char* keyword;
    size_t length;
    int kind;
};

static JsonKeywordDef JsonKeywords[] = {
    { "null",  4, JsonTokNull  },
    { "true",  4, JsonTokTrue  },
    { "false", 5, JsonTokFalse },
};

const char* JsonTokenToString(int x)
{
    switch(x)
    {
    case JsonTokInteger: return "integer";
    case JsonTokNumber: return "number";
    case JsonTokString: return "string literal";
    case JsonTokTrue:   return "true";
    case JsonTokFalse:  return "false";
    case JsonTokNull:   return "null";
    case JsonTokBadIdentifier: return "unknown keyword or identifier";
    };
    return "unkown";
}

static size_t const JsonKeywordCount = sizeof(JsonKeywords);

JsonStringStream::JsonStringStream(const char* cstr, size_t length): pos(0)
{
    buffer.Resize(length);
    Pika_memcpy(buffer.GetAt(0), cstr, length);
}
    
JsonStringStream::~JsonStringStream()
{
}

int JsonStringStream::Get()
{ 
    return pos < buffer.GetSize() ? buffer[pos++] : EOF; 
}

int JsonStringStream::Peek()
{
    return pos < buffer.GetSize() ? buffer[pos] : EOF;
}

bool JsonStringStream::IsEof()
{
    return pos > buffer.GetSize();
}

JsonTokenizer::JsonTokenizer(String* str): 
    look(EOF), line(0), col(0),
    stream(str->GetBuffer(), str->GetLength())
{
    Pika_memzero(&token, sizeof(JsonToken));
    this->GetLook();
}

JsonTokenizer::~JsonTokenizer()
{
    for (size_t i = 0; i < tokens.GetSize(); ++i)
    {
        if (tokens[i].kind == JsonTokString)
        {
            Pika_free(tokens[i].val.str.buffer);
        }
    }
}

void JsonTokenizer::GetLook()
{
    if (stream.IsEof())
    {
        this->look = EOF;
    }
    else
    {
        this->look = stream.Get();
        col++;
        this->jsonBuffer.Push((char)this->look);   
        if (this->look == '\n')
        {
            line++;
            col = 0;
        }
    }
}

bool JsonTokenizer::IsEof()
{
    return this->look == EOF;
}

void JsonTokenizer::SyntaxError(const char* msg)
{
    RaiseException(Exception::ERROR_syntax, 
        "%s at line %d col %d, while parsing JSON.", 
        msg, line, col);
}

void JsonTokenizer::EatWhitespace()
{
    if (IsSpace(this->look))
    {
        do {
            this->GetLook();
            if (this->IsEof()) {
                return;
            }
        }
        while (IsSpace(this->look));
    }
}

void JsonTokenizer::GetNext()
{
    this->EatWhitespace();
    if (this->look != EOF)
    {
        if (this->look == '\"')
        {
            /* A Double Quoted String (the only type of string allowed in JSON). */
            Buffer<char> str;
            int kind = JsonTokString;
            do
            {
                this->GetLook();
                
                if (this->IsEof())
                {
                    /* If we reached the end of stream, then the string literal was unclosed. */
                    this->SyntaxError("Unclosed string literal");
                    break;
                }
                else if (this->look == '\\')
                {
                    /* Make sure the escape code is valid. */
                    this->GetLook();
                    switch(this->look)
                    {
                    case 'b':  str.Push('\b'); break;
                    case 'f':  str.Push('\f'); break;
                    case 'n':  str.Push('\n'); break;
                    case 'r':  str.Push('\r'); break;
                    case 't':  str.Push('\t'); break;
                    case '\"': str.Push('\"'); break;
                    case '\\': str.Push('\\'); break;
                    case '/':  str.Push('/');  break;                        
                    case 'u':
                    {
                        /* JSON Unicode literal
                         * 
                         * Always 4 characters long.
                         * Represents a utf-32 character code.
                         *
                         * We want to turn it into a utf-8 sequence so that it is compatilible with
                         * Pika's string format.
                         */
                        size_t u = 0;
                        size_t digits = 0;
                        do
                        {
                            this->GetLook();
                            if (this->IsEof())
                            {
                                this->SyntaxError("Bad unicode literal in string literal.");
                            }
                            if (IsDigit(this->look))
                            {
                                digits = digits * 16 + (this->look - '0');
                            }
                            else if (IsLetter(this->look))
                            {
                                int lower = ToLower(this->look);
                                digits = digits * 16 + (lower - 'a' + 10);
                            }
                            else
                            {
                                this->SyntaxError("Bad unicode literal in string literal.");
                            }
                        }
                        while(++u < 4);
                        
                        // Convert the 32 bit unicode char to utf-8.
                        if (digits < 0x80)
                        {
                            str.Push(digits);
                        }
                        else if (digits <= 0x7FF)
                        {
                            str.Push((digits >> 6)   + 0xC0);
                            str.Push((digits & 0x3F) + 0x80);
                        }
                        else if (digits <= 0xFFFF)
                        {
                            str.Push((digits  >> 12)  + 0xE0);
                            str.Push(((digits >> 6)   & 0x3F) + 0x80);
                            str.Push((digits  & 0x3F) + 0x80);
                        }
                        else if (digits <= 0x10FFFF)
                        {
                            str.Push((digits  >> 18) + 0xF0);
                            str.Push(((digits >> 12) & 0x3F) + 0x80);
                            str.Push(((digits >> 6)  & 0x3F) + 0x80);
                            str.Push((digits & 0x3F) + 0x80);
                        }
                        else
                        {
                            this->SyntaxError("Bad unicode literal in string literal.");
                        }
                        break;
                    }
                    default: 
                        this->SyntaxError("Bad escape code.");
                    };
                }
                else if (this->look == '\"')
                {
                    break;
                }
                else
                {
                    str.Push((char)this->look);
                }
            }
            while (true);
                                
            // Move past the closing quote.
            if (this->look == '\"')
            {
                this->GetLook();
            }
            size_t length = str.GetSize();
            str.Push('\0');
            
            token.kind = JsonTokString;
            token.col = col;
            token.line = line;
            token.val.str.buffer = (char*)Pika_calloc(length + 1, sizeof(char));
            token.val.str.length = length;
            Pika_memcpy(token.val.str.buffer, str.GetAt(0), length);
            tokens.Push(token);
        }
        else if (IsDigit(this->look) || this->look == '-')
        {
            /* A Number. */
            u8 digits = 0;
            u8 integer = 0;
            bool isfraction = false;
            bool isexp = false;
            int sign = 1;
            int expsign = 1;
            int kind = JsonTokInteger;
            s8 fract_part = 0;
            
            if (this->look == '-')
            {
                sign = -1;
                this->GetLook();
                if (!IsDigit(this->look))
                {
                    this->SyntaxError("Expected number after negative sign.");
                }
            }            
            do {
                digits *= 10;
                digits += look - '0';
                
                this->GetLook();
                if (isfraction) {
                    ++fract_part;
                }
                if (this->IsEof())
                {
                    break;
                }
                else if (this->look == '.')
                {
                    if (isexp)
                    {
                        /* Fraction part cannot come after the exponent. */
                        this->SyntaxError("Bad number.");
                    }
                    integer = digits;
                    isfraction = true;
                    
                    this->GetLook();
                    kind = JsonTokNumber;
                    
                    if (!IsDigit(this->look))
                    {
                        this->SyntaxError("Bad number.");
                    }                 
                }
                else if (this->look == 'e' || this->look == 'E')
                {
                    this->GetLook();
                    integer = digits;
                    digits = 0;
                    expsign = 1;
                    kind = JsonTokNumber;
                    isexp = true;
                    
                    /* Look for the exponent sign. */
                    if (this->look == '+')
                    {
                        this->GetLook();
                    }
                    else if (this->look == '-')
                    {
                        expsign = -1;
                        this->GetLook();
                    }
                    
                    /* Make sure a digit comes after the exponent. */
                    if (!IsDigit(this->look))
                    {
                        this->SyntaxError("Bad exponent.");
                    }
                }
            }
            while (IsDigit(this->look));
                        
            token.kind = kind;
            token.col = col;
            token.line = line;
            
            if (kind == JsonTokInteger)
            {
                /* Add the sign to the integer. */
                token.val.integer = digits * sign;
            } 
            else if (kind == JsonTokNumber)
            {
                /* Build the JSON Number from the specified parts.. */
                s8 exp_part = 0;
                if (isexp)
                {
                    exp_part = digits;
                    digits = integer;
                }
                
                double number = (double)digits * sign;                
                s8 exponent = (s8)exp_part * expsign;
                exponent -= fract_part;
                
                number *= pow(10.0, (double)exponent);  
                token.val.number = number;              
            }
            tokens.Push(token);
        }
        else if (IsLetter(this->look))
        {
            /* The JSON specification doesn't allow identifiers, all object properties must be
             * double quoted strings.
             * 
             * Instead we are looking for the 3 keywords: null, true and false. 
             */
            size_t start = this->jsonBuffer.GetSize() - 1;
            do
            {
                this->GetLook();
                if (this->IsEof())
                {
                    break;
                }
            }
            while (IsLetter(this->look));
            
            size_t end = this->jsonBuffer.GetSize() - 1;            
            const char* identifier = this->jsonBuffer.GetAt(start);
            size_t length = end - start;
            int kind = JsonTokBadIdentifier;
            
            for (size_t i = 0; i < JsonKeywordCount; ++i)
            {
                JsonKeywordDef& def = JsonKeywords[i];
                if (def.length != length)
                {
                    continue;
                }
                else if(StrCmpWithSize(identifier, def.keyword, length) == 0)
                {
                    kind = def.kind;
                }
            }
            
            token.kind = kind;
            token.col = col;
            token.line = line;
            token.val.str.buffer = 0;
            token.val.str.length = 0;
            tokens.Push(token);
        }
        else
        {
            token.kind = look;
            token.col = col;
            token.line = line;
            token.val.ch = this->look; 
            tokens.Push(token);
            this->GetLook();
        }
    }
    else
    {
        token.kind = look;
        token.col = col;
        token.line = line;
        token.val.ch = this->look; 
        tokens.Push(token);
        this->GetLook();
    }
}

Value JsonParser::Parse()
{
    this->tokenizer->GetNext();
    Value res(NULL_VALUE);
    if (IsTokenKind('{')) {
        Dictionary* dict = DoObject();
        res.Set(dict);
    } else if (IsTokenKind('[')) {
        Array* array = DoArray();
        res.Set(array);
    } else {
        this->Unexpected();
    }
    return res;
}

JsonParser::JsonParser(Engine* eng, JsonTokenizer* tokenizer):
    engine(eng), tokenizer(tokenizer)
{
}

JsonParser::~JsonParser()
{
}

int JsonParser::GetTokenKind()
{
    return tokenizer->token.kind;
}

bool JsonParser::IsTokenKind(int x)
{
    return GetTokenKind() == x;
}

Dictionary* JsonParser::DoObject()
{
    Match('{');
    Dictionary* dict = Dictionary::Create(engine, engine->Dictionary_Type);
    String* key = 0;
    if (IsTokenKind(JsonTokString)) {
        do {
            String* key = DoString(false);
            Match(':');
            Value value = DoValue();
            dict->BracketWrite(key, value);
        } while (Optional(','));
    }
    Match('}');
    return dict; 
}

Array* JsonParser::DoArray()
{
    Match('[');

    Array* array = Array::Create(engine, engine->Array_Type, 0, 0);
    if (!IsTokenKind(']'))
    {
        do {
            Value value = DoValue();
            array->Push(value);
        }
        while (Optional(','));
    }
    Match(']');
    return array;
}

void JsonParser::GetNext()
{
    if (GetTokenKind() != EOF)
    {
        tokenizer->GetNext();
    }
}

Value JsonParser::DoValue()
{
    Value res(NULL_VALUE);
    switch(GetTokenKind())
    {
    case '{':
        res.Set(DoObject());
    break;
    case '[':
        res.Set(DoArray());
    break;
    case JsonTokInteger:
        res.Set((pint_t)this->tokenizer->token.val.integer);
        this->GetNext();
        break;
    case JsonTokNumber:
        res.Set((preal_t)this->tokenizer->token.val.number);
        this->GetNext();
        break;
    case JsonTokString:
        res.Set(DoString(false));
        break;
    case JsonTokTrue:
        res.SetTrue();
        this->GetNext();
        break;
    case JsonTokFalse:
        res.SetFalse();
        this->GetNext();
        break;
    case JsonTokNull:
        res.SetNull();
        this->GetNext();
        break;
    default:
        this->Unexpected();
    }
    return res; 
}

String* JsonParser::DoString(bool optional)
{
    if (IsTokenKind(JsonTokString))
    {
        JsonToken& token = tokenizer->token;
        String* res = engine->GetString(token.val.str.buffer, token.val.str.length);
        Match(JsonTokString);
        return res;
    }
    else if (!optional)
    {
        Expected(JsonTokString, false);
    }
    return 0;
}

void JsonParser::Unexpected()
{
    this->Expected(GetTokenKind(), true);
}

void JsonParser::Expected(int x, bool unexpected)
{
    int line = tokenizer->line;
    int col  = tokenizer->col;
    const char * expected = unexpected ? "Unexpected" : "Expected";
    
    if (x > JsonTokMin && x <= JsonTokMax)
    {
        const char* tokenName = JsonTokenToString(x);
        RaiseException(Exception::ERROR_syntax, 
            "%s token '%s', at line %d col %d, while parsing JSON.", 
            expected,
            tokenName, line, col);        
    }
    else if (x < JsonTokMin)
    {
        RaiseException(Exception::ERROR_syntax, 
            "%s token '%c', at line %d col %d, while parsing JSON.", 
            expected,
            (char)x, line, col);
    }
    else
    {
        RaiseException(Exception::ERROR_syntax, 
            "%s token %d, at line %d col %d, while parsing JSON.", 
            expected,
            x, line, col);
    }
}

void JsonParser::Match(int tok)
{
    if (GetTokenKind() == tok)
    {
        tokenizer->GetNext();
    }
    else
    {
        Expected(tok);
    }
}

bool JsonParser::Optional(int tok)
{
    if (GetTokenKind() == tok)
    {
        GetNext();
        return true;
    }
    return false;
}

}// pika
