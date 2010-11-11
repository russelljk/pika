/*
 * PTokenizer.cpp
 * See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PTokenDef.h"
#include "PError.h"
#include "PMemory.h"
#include "PTokenizer.h"
#include "PAst.h"
#include "PPlatform.h"
#define PIKA_keyword(x) { x, Token2String::GetNames()[x - TOK_global], 0 }

namespace pika {

struct KeywordDescriptor
{
    ETokenType  ttype; // keyword's token type
    const char* name;  // The name of the token, (can be any valid identifier).
    size_t      length;// length of the name, filled in at runtime.
};

static KeywordDescriptor static_keywords[] =
{
    // Keywords.
    //
    // { ETokenType, const char*, size_t },    
    PIKA_keyword(TOK_global),
    PIKA_keyword(TOK_local),
    PIKA_keyword(TOK_member),
    PIKA_keyword(TOK_function),
    PIKA_keyword(TOK_begin),
    PIKA_keyword(TOK_end),
    PIKA_keyword(TOK_elseif),
    PIKA_keyword(TOK_loop),
    PIKA_keyword(TOK_return),
    PIKA_keyword(TOK_locals),
    PIKA_keyword(TOK_self),
    PIKA_keyword(TOK_super),
    PIKA_keyword(TOK_null),
    PIKA_keyword(TOK_if),
    PIKA_keyword(TOK_for),
    PIKA_keyword(TOK_else),
    PIKA_keyword(TOK_in),
    PIKA_keyword(TOK_of),
    PIKA_keyword(TOK_is),
    PIKA_keyword(TOK_has),
    PIKA_keyword(TOK_true),
    PIKA_keyword(TOK_false),
    PIKA_keyword(TOK_while),
    PIKA_keyword(TOK_try),
    PIKA_keyword(TOK_catch),
    PIKA_keyword(TOK_raise),
    PIKA_keyword(TOK_break),
    PIKA_keyword(TOK_continue),
    PIKA_keyword(TOK_yield),
    PIKA_keyword(TOK_mod),
    PIKA_keyword(TOK_and),
    PIKA_keyword(TOK_or),
    PIKA_keyword(TOK_xor),
    PIKA_keyword(TOK_not),
    PIKA_keyword(TOK_then),
    PIKA_keyword(TOK_do),
    PIKA_keyword(TOK_property),
    PIKA_keyword(TOK_finally),
    PIKA_keyword(TOK_package),
    PIKA_keyword(TOK_using),
    PIKA_keyword(TOK_when),
    PIKA_keyword(TOK_bind),
    PIKA_keyword(TOK_to),
    PIKA_keyword(TOK_downto),
    PIKA_keyword(TOK_by),
    PIKA_keyword(TOK_class),
};

#define NumKeywordDescriptors ((sizeof (static_keywords))/(sizeof (KeywordDescriptor)))

// Convert a character/digit into a radix digit (ie 'f' returns 15).
INLINE u4 RadixToNumber(int x)
{
    if (IsDigit(x))
    {
        return x - '0';
    }
    else if (IsLetter(x))
    {
        x = ToLower(x);
        return x - 'a' + 10;
    }
    else
    {
        return 0;
    }
}

template<typename T>
struct NumberParser
{
    NumberParser(T* p) : tokenType(0), pT(p)
    {}
    
    bool ParseOk() { return tokenType == TOK_integerliteral || tokenType == TOK_realliteral; }
    
    void ReadNumber()
    {
        u4   radix      = 10;
        bool seen_dot   = false;
        bool seen_exp   = false;
        bool has_radix  = false;
        u8   int_part   = 0;
        u8   exp_part   = 0;
        u4   fract_part = 0;
        int  sign       = 1;
        
        if (!(IsDigit(pT->Look()) || pT->Look() == '.'))
        {
            pT->NumberParseError("Expecting digit or '.' while reading number");
            return;
        }
        
        ReadDigits(10, int_part);
        
        if (IsLetter(pT->Look())) // possible radix literal
        {
            int x = ToLower(pT->Look());
            bool noRadixSpecified = (int_part == 0);
            bool changed_radix = true;
            
            if (((x == 'x') || (x == 'h')) && (noRadixSpecified || int_part == 16))
            { 
                // Hexidecimal
                pT->GetLook();
                radix = 16;
            }
            else if ((x == 'b') && (noRadixSpecified || int_part == 2))
            { 
                // Binary
                pT->GetLook();
                radix = 2;
            }
            else if ((x == 'o') && (noRadixSpecified || int_part == 8))
            { 
                // Octal
                pT->GetLook();
                radix = 8;
            }
            else if ((x == 'r'))
            {
                // Radix specification.
                if ((int_part <= 36) && (int_part >= 2))
                {
                    pT->GetLook();
                    radix = (u4)int_part;
                }
                else
                {
                    pT->NumberParseError("invalid radix specified. needs to be from 2 to 36");
                    return;
                }
            }
            else if (x != 'e')
            {
                pT->NumberParseError("unexpected letter incountered while parsing number.");
                return;
            }
            else
            {
                changed_radix = false; // exponent
            }
            
            if (!(IsLetterOrDigit(pT->Look()) || pT->Look() == '.'))
            {
                pT->NumberParseError("Expecting digit or '.' while reading number");
                return;
            }
            // Read the integer part if a radix was supplied.
            if (changed_radix)
            {
                int_part = 0;
                ReadDigits(radix, int_part);
                has_radix = true;
            }
        }
        
        // 3:
        // Read in the fraction part [if present].
        if (pT->Look() == '.' && IsLetterOrDigit(pT->Peek()) && (RadixToNumber(pT->Peek()) < radix))
        {
            seen_dot = true;
            pT->GetLook();
            
            ASSERT(IsLetterOrDigit(pT->Look()));
            fract_part = ReadDigits(radix, int_part);
        }
        
        // 4:
        // Read in the exponent part [if present].
        if (pT->Look() == 'e' || pT->Look() == 'E')
        {
            pT->GetLook();
            seen_exp = true;
            
            if (pT->Look() == '+')
            {
                pT->GetLook();
                sign = + 1;
            }
            else if (pT->Look() == '-')
            {
                pT->GetLook();
                sign = - 1;
            }
            else
            {
                sign = 1;
            }
            
            if (!IsLetterOrDigit(pT->Look()))
            {
                pT->NumberParseError("Expecting exponent.");
                return;
            }
            ReadDigits(radix, exp_part);            
        }
        
        if (!seen_exp && !seen_dot)
        {
        
            tokenVal.integer = (pint_t)int_part;
            tokenType = TOK_integerliteral;
        }
        else
        {
            double val = (double)int_part;
            
            if (val != 0.0)
            {
                s8 exponent = (s8)exp_part * sign;
                exponent -= fract_part;
                
                val *= pow((double)radix, (double)exponent);
            }
            
            tokenVal.real = (preal_t)val;
            tokenType = TOK_realliteral;
        }
    }
    
    u4 ReadDigits(u4 radix, u8& int_part)
    {
        u4 count = 0;
        
        while (IsLetterOrDigit(pT->Look()) || (pT->Look() == '_'))
        {
        
            if (pT->Look() != '_')
            {
                unsigned int currdig = RadixToNumber(pT->Look());
                
                if (currdig >= radix)
                {
                    return count;
                }
                else
                {
                    count++;
                    int_part = (int_part * radix) + currdig;
                }
            }
            pT->GetLook();
        }
        return count;
    }
    
    YYSTYPE tokenVal;
    int tokenType;
    T* pT;
};

///////////////////////////////////////////StringToNumber///////////////////////////////////////////

StringToNumber::StringToNumber(const char* str,
                               size_t      len,
                               bool        eatwsp,
                               bool        use_html_hex,
                               bool        must_consume)
        : look(EOF),
        curr(str),
        buffer(str),
        end(str + len),        
        is_number(false),
        is_real(false),
        is_integer(false),
        eat_white(eatwsp),
        html_hex(use_html_hex)
{
    ReadNumber();
    
    if (is_number)
    {
        if (eat_white)
        {
            EatWhiteSpace();
        }
        is_number = !must_consume || (curr >= end);
    }
}

StringToNumber::StringToNumber(const char* str, const char* endstr, bool must_consume)
        : look(EOF),
        curr(str),
        buffer(str),
        end(endstr),        
        is_number(true),
        is_real(false),
        is_integer(false),
        eat_white(false),
        html_hex(false)
{
    ReadNumber();
    
    if (is_number)
    {
        if (eat_white)
        {
            EatWhiteSpace();
        }
        is_number = !must_consume || (curr >= end);
    }
}

void StringToNumber::EatWhiteSpace()
{
    while (IsSpace(look) && look != EOF)
    {
        GetLook();
    }
}

void StringToNumber::GetLook()
{
    if (curr < end)
    {
        look = *curr++;
    }
    else
    {
        look = EOF;
    }
}

void StringToNumber::ReadNumber()
{
    GetLook();
    EatWhiteSpace();
    
    NumberParser<StringToNumber> numparse(this);
    numparse.ReadNumber();
    
    is_integer = is_real = 0;
    if (numparse.ParseOk())
    {
        if (numparse.tokenType == TOK_realliteral)
        {
            is_real    = 1;
            is_integer = 0;
            is_number  = 1;
            real = numparse.tokenVal.real;
        }
        else if (numparse.tokenType == TOK_integerliteral)
        {
            is_real    = 0;
            is_integer = 1;
            is_number  = 1;
            integer = numparse.tokenVal.integer;
        }
    }
    else
    {
        is_number = 0;
    }
}

///////////////////////////////////////////IScriptStream////////////////////////////////////////////

IScriptStream::~IScriptStream()
{
}

//////////////////////////////////////////////Tokenizer/////////////////////////////////////////////

Tokenizer::Tokenizer(CompileState* s, std::ifstream* fs)
        : is_str_ctor(0),
        tokenBegin(0),
        tokenEnd(0),
        state(s),
        tabIndentSize(4),
        line(1),
        col(0),
        ch(0),
        prevline(1),
        prevcol(0),
        prevch(0),
        look(EOF),
        hasUtf8Bom(false),
        minKeywordLength(0),
        maxKeywordLength(0xFFFF),
        script_stream(0)
{
    PIKA_NEW(FileScriptStream, script_stream, (fs));
    PrepKeywords();
    GetLook();
    
}

Tokenizer::Tokenizer(CompileState* s, const char* buf, size_t len)
        : is_str_ctor(0),
        tokenBegin(0),
        tokenEnd(0),        
        state(s),
        tabIndentSize(4),
        line(1),
        col(0),
        ch(0),
        prevline(1),
        prevcol(0),
        prevch(0),
        look(EOF),
        hasUtf8Bom(false),
        minKeywordLength(0),
        maxKeywordLength(0xFFFF),
        script_stream(0)
{
    PIKA_NEW(StringScriptStream, script_stream, (buf, len));
    PrepKeywords();
    GetLook();
    
}

Tokenizer::Tokenizer(CompileState* cs, IScriptStream* stream)
    : is_str_ctor(0),
    tokenBegin(0),
    tokenEnd(0),        
    state(cs),
    tabIndentSize(4),
    line(1),
    col(0),
    ch(0),
    prevline(1),
    prevcol(0),
    prevch(0),
    look(EOF),
    hasUtf8Bom(false),
    minKeywordLength(0),
    maxKeywordLength(0xFFFF),
    script_stream(stream)
{
    PrepKeywords();
    GetLook();

}

void Tokenizer::PrepKeywords()
{
    minKeywordLength = 0xFFFF;
    maxKeywordLength = 0;
    
    for (size_t k = 0; k < NumKeywordDescriptors; ++k)
    {
        size_t keyLength = strlen(static_keywords[k].name);
        
        static_keywords[k].length = keyLength;
        minKeywordLength = Min(minKeywordLength, keyLength);
        maxKeywordLength = Max(maxKeywordLength, keyLength);
    }
}

Tokenizer::~Tokenizer()
{
    if (!script_stream->IsREPL())
        Pika_delete(script_stream);
}

void Tokenizer::GetNext()
{
    EatWhitespace();
    
    prevline     = line;
    prevcol      = col;
    prevch       = ch;
    tokenVal.str = 0;
    tokenVal.len = 0;
    tokenType    = -1;
    tokenBegin   = tokenEnd - 1;
    
    // identifier or keyword
    if (IsLetter(look) || IsIdentifierExtra(look)) 
    {        
        ReadIdentifier();
    }
    // integer, real or radix number
    else if (IsDigit(look) || look == '.') 
    {
        if (look == '.' && !IsDigit(Peek()))
        {
            return ReadControl();
        }
        ReadNumber();
    }
    // string literal
    else if (look == '\'' || look == '\"' || (look == '}' && is_str_ctor != 0)) 
    {
        if (look == is_str_ctor) {
            state->SyntaxException(Exception::ERROR_syntax, line, col, "cannot have a %s quote string inside a %s quote string constructor.", look == '\"' ? "double" : "single", look == '\"' ? "double" : "single");
        }
        ReadString(); 
    }
    else if (look == '`')
    {
        ReadString();
        tokenType = TOK_identifier;
    }
    // control character
    else 
    {
        ReadControl(); 
    }
}

void Tokenizer::EatWhitespace()
{
    while (IsSpace(look))
    {    
        GetLook();
    }
}

u4 Tokenizer::ReadDigits(u4 radix, u8& int_part)
{
    u4 count = 0;
    
    while (IsLetterOrDigit(look) || (look == '_'))
    {
        if (look != '_')
        {
            unsigned int currdig = RadixToNumber(look);
            
            if (currdig >= radix)
            {
                return count;
            }
            else
            {
                count++;
                int_part = (int_part * radix) + currdig;
            }
        }
        GetLook();
    }
    return count;
}

void Tokenizer::ReadNumber()
{
    NumberParser<Tokenizer> numparse(this);
    numparse.ReadNumber();
    
    if (numparse.ParseOk())
    {
        tokenVal.real    = numparse.tokenVal.real;
        tokenVal.integer = numparse.tokenVal.integer;
        tokenType        = numparse.tokenType;
    }
    else
    {
        // TODO: We need to provide more information, maybe even the subject string.
        NumberParseError("Attempt to parse number from string."); 
    }
}

void Tokenizer::GetMoreInput()
{
    if (script_stream->IsREPL()) {
        REPLStream* repl = (REPLStream*)script_stream;
        repl->NewLoop("...");
        GetLook();
    }
}

void Tokenizer::ReadIdentifier()
{
    // [a-zA-Z_][a-zA-Z0-9_]*[?!]?
    
    if (!(IsLetter(look) || IsIdentifierExtra(look))) // make sure we start with an appropriate character.
        state->SyntaxException(Exception::ERROR_syntax, line, col, "Expecting letter or '_' while reading identifier");
    
    GetLook();
        
    while (IsLetterOrDigit(look) || IsIdentifierExtra(look))
    {
        GetLook();
    }
    
    // Optional terminating ? or !
    if (look == '!' || look == '?')
    {
        GetLook();
    }
    
    ASSERT(tokenEnd > tokenBegin);
    
    size_t toklen = tokenEnd - 1 - tokenBegin;
    
    // might be a keyword
    if (toklen >= minKeywordLength && toklen <= maxKeywordLength)
    {        
        for (size_t i = 0; i < NumKeywordDescriptors; ++i)
        {
            if (toklen == static_keywords[i].length)
            {
                if (StrCmpWithSize(GetBeginPtr(), static_keywords[i].name, toklen) == 0)
                {
                    tokenType = static_keywords[i].ttype;
                    return;                    
                }
            }
        }            
    }
    
    // its an identifier, not a keyword.
    size_t len = toklen;
    char* strtemp = (char*)Pika_malloc((len + 1) * sizeof(char));
    
    Pika_memcpy(strtemp, GetBeginPtr(), len*sizeof(char));
    
    strtemp[len] = '\0';
    
    tokenVal.str = strtemp;
    tokenVal.len = len;
    tokenType    = TOK_identifier;
}

void Tokenizer::ReadString()
{
    int str_tok_type = TOK_stringliteral;
    int termchar = look;
    bool is_identifier_string = false;
    if (termchar == '`')
        is_identifier_string = true;
        
    if (termchar != '\'' && termchar != '\"' && termchar != '`')
    {
        if (termchar == '}' && is_str_ctor != 0)
        {
            str_tok_type = TOK_string_l;
            termchar = is_str_ctor;            
        }
        else
        {
            return;
        }
    }    
    GetLook();
    
    while (look != termchar && !IsEndOfStream())
    {
        if (look == '\\')
        {
            GetLook(); // move past the back slash
            GetLook(); // move past next char
        }
        else if (look == '{' && !is_identifier_string && termchar != '`')
        {
            if (is_str_ctor && is_str_ctor != termchar)
                state->SyntaxException(Exception::ERROR_syntax, prevline, prevcol, "cannot have nested string constructors");
            if (str_tok_type == TOK_string_l)
                str_tok_type = TOK_string_b;
            else
                str_tok_type = TOK_string_r;
            is_str_ctor = termchar;
            break;
        }
        else
        {
            GetLook();
        }
    }
    if (str_tok_type == TOK_string_l)
        is_str_ctor = 0;
    
    if (IsEndOfStream() && look != termchar)
        state->SyntaxException(Exception::ERROR_syntax, prevline, prevcol, "unclosed string literal");
        
    GetLook();
    
    const char* strbeg = GetBeginPtr() + 1; // move past the opening " or '
    const char* strend = GetEndPtr() - 1; // move in front of the closing " or '
    
    ASSERT(strend > strbeg);
    
    size_t len = strend - 1 - strbeg;
    char* strtemp = (char*)Pika_malloc((len + 1) * sizeof(char));
    
    Pika_memcpy(strtemp, strbeg, len * sizeof(char));
    
    strtemp[len] = '\0';
    tokenVal.str = Pika_TransformString(strtemp, len, &tokenVal.len);
    tokenType    = str_tok_type;
    
    Pika_free(strtemp);
}

void Tokenizer::ReadControl()
{
    switch (look)
    {    
    case '?':
    {
        tokenType = look;
        GetLook();
        
        if (look == '?')
        {
            tokenType = TOK_nullselect;
            GetLook();
        }
    }
    break;
        
    case '+':
    {
        tokenType = look;
        GetLook();
        
        if (look == '+')
        {
            tokenType = TOK_increment;
            GetLook();
            
        }
        else if (look == '=')
        {
            tokenType = TOK_addassign;
            GetLook();
        }
    }
    break;
        
    case '-':
    {
        tokenType = look;
        GetLook();
        
        if (look == '-')
        {
            tokenType = TOK_decrement;
            GetLook();
            
        }        
        else if (look == '=')
        {
            tokenType = TOK_subassign;
            GetLook();
        }
    }
    break;
        
    case '*':
    {
        tokenType = look;
        GetLook();
        
        if (look == '=')
        {
            tokenType = TOK_mulassign;
            GetLook();
        }
        else if (look == '*')
        {
            tokenType = TOK_starstar;
            GetLook();
        }
    }
    break;
        
    case '#':
    {
        GetLook();
        ReadSingleLineComment();
        return GetNext();
    }
    break;
    
    case '|':
    {
        tokenType = look;
        GetLook();
        
        if (look == '=')
        {
            tokenType = TOK_bitorassign;
            GetLook();
        }
        else if (look == '|')
        {
            tokenType = TOK_pipepipe;
            GetLook();
        }
    }
    break;
        
    case '/':
    {
        tokenType = look;
        GetLook();
        
        if (look == '=')
        {
            tokenType = TOK_divassign;
            GetLook();
        }
        else if (look == '/')
        {
            
            tokenType = TOK_div;            
            GetLook();
            
            if (look == '=')
            {
                tokenType = TOK_idivassign;
                GetLook();
            }
        }
    }
    break;

    case '(':
    {
        int lineStart = line;
        tokenType = look;
        GetLook();
        
        if (look == '*')
        {
            GetLook();
            ReadMultiLineComment(lineStart);
            return GetNext();
        }
    }
    break;
    
    case '%':
    {
        tokenType = look;
        GetLook();
        
        if (look == '=')
        {
            tokenType = TOK_modassign;
            GetLook();
        }
    }
    break;
        
    case '<':
    {
        tokenType = look;
        GetLook();
        
        if (look == '<')
        {
            tokenType = TOK_lsh;
            GetLook();
            
            if (look == '=')
            {
                tokenType = TOK_lshassign;
                GetLook();
            }
        }
        else if (look == '=')
        {
            tokenType = TOK_lte;
            GetLook();
        }
        else if (look == '>')
        {
            tokenType = TOK_ne;
            GetLook();
        }
    }
    break;
        
    case '>':
    {
        // >, greater
        tokenType = look;
        GetLook();
        
        if (look == '>')
        {
            // >>, right shift signed
            tokenType = TOK_rsh;
            GetLook();
            
            if (look == '>')
            {
                // >>>, right shift unsigned
                tokenType = TOK_ursh;
                GetLook();
                
                if (look == '=')
                {
                    // >>>=, right shift unsigned + assign
                    tokenType = TOK_urshassign;
                    GetLook();
                }
            }
            else if (look == '=')
            {
                // >>>=, right shift signed + assign
                tokenType = TOK_rshassign;
                GetLook();
            }
        }
        else if (look == '=')
        {
            // >=, greater than equal to
            tokenType = TOK_gte;
            GetLook();
        }
    }
    break;
        
    case '.':
    {
        // . , member access
        tokenType = look;
        GetLook();
        
        if (look == '.')
        {
            // .. , cat operator
            tokenType = TOK_cat;
            GetLook();
            
            if (look == '.')
            {
                // ... , cat space operator
                tokenType = TOK_catspace;
                GetLook();

                if (look == '=')
                {
                    tokenType = TOK_catspassign;
                    GetLook();
                }
            }
            else if (look == '=')
            {
                tokenType = TOK_catassign;
                GetLook();
            }            
        }
    }
    break;
        
    case '&':
    {
        tokenType = look;
        GetLook();
        
        if (look == '=')
        {
            tokenType = TOK_bitandassign;
            GetLook();
        }
        else if (look == '&')
        {
            tokenType = TOK_ampamp;
            GetLook();
        }
    }
    break;
            
    case '^':
    {
        tokenType = look;
        GetLook();
        
        if (look == '=')
        {
            tokenType = TOK_bitxorassign;
            GetLook();
        }
        else if (look == '^')
        {
            tokenType = TOK_caretcaret;
            GetLook();
        }
    }
    break;
        
    case '=':
    {
        tokenType = look;
        GetLook();
        
        if (look == '=')
        {
            tokenType = TOK_eq;
            GetLook();
            
            if (look == '=')
            {
                tokenType = TOK_same;
                GetLook();
            }
        }
        else if (look == '>')
        {
            tokenType = TOK_implies;
            GetLook();
        }        
    }
    break;
        
    case '!':
    {
        tokenType = look;
        GetLook();
        
        if (look == '=')
        {
            tokenType = TOK_ne;
            GetLook();
            
            if (look == '=')
            {
                tokenType = TOK_notsame;
                GetLook();
            }
        }
    }
    break;
    
    case ':':        
        tokenType = look;
        GetLook();
        
        if (look == ':')
        {
            tokenType = TOK_coloncolon;
            GetLook();
        }    
    break;
    
    default:
        if (!IsAscii(look) && look != EOF && look != EOI) {
            if (line == 1 && col == 1) {
                CheckBom(); 
                return;
            }
            else {
                state->SyntaxException(Exception::ERROR_syntax, line, col, "Non-ascii character encountered %d.\n", look);
            }
        }
        tokenType = look;
        GetLook();    
    }
}

void Tokenizer::ReadMultiLineComment(int lineStart)
{
    int depth = 1;
    while (!IsEndOfStream() && depth)
    {
        if (look == '(')
        {
            GetLook();
            
            if (look == '*')
            {
                depth++;
                GetLook();
            }
        }
        else if (look == '*')
        {
            GetLook();
            
            if (look == ')')
            {
                depth--;
                GetLook();
            }
        }
        else
        {
            GetLook();
        }
    }
    
    if (depth)
    {
        state->SyntaxException(Exception::ERROR_syntax, lineStart, "unclosed multi-line comment");
    }
}

void Tokenizer::ReadSingleLineComment()
{
    while (!IsEndOfStream() && look != '\n')
        GetLook();
}

bool Tokenizer::IsEndOfStream()
{
    return script_stream->IsEof() || script_stream->IsEoi();
}

int Tokenizer::Peek()
{
    return script_stream ? script_stream->Peek() : EOF;
}

void Tokenizer::NumberParseError(const char* msg)
{
    state->SyntaxException(Exception::ERROR_syntax, line, col, msg);
}

void Tokenizer::GetLook()
{
    if (script_stream && !script_stream->IsEof())
    {
        look = script_stream->Get();
        if (look == EOI)
        {
            // We have reached an end of the currrent line of input from stdin.
            ++line;       // Start a new line...
            col = ch = 0; // at column 0.
            script_buf.Push('\n'); // Treat it as a newline in the script buffer.
            tokenEnd++;
            return;
        }
        
        script_buf.Push(look);
        tokenEnd++;
        if (IsSpace(look))
        {
            // Look for a valid newline
            // CR Mac Classic
            // LF Mac OSX, *nix
            // CR + LF Windows, OS/2
            
            if (look == '\r')
            {
                if (!IsEndOfStream())
                {
                    int next_look = script_stream->Peek();
                    
                    if (next_look != '\n')
                    {
                        // single CR
                        ++line;
                        col = ch = 0;
                    }
                    // otherwise CR LF
                }
            }
            
            if (look == '\n')
            {
                // LF
                ++line;
                col = ch = 0;
            }
            else if (look == '\t')
            {
                ++ch;
                col += tabIndentSize;
            }
            else
            {
            
                ++ch;
                ++col;
            }
        }
        else
        {
            ++ch;
            ++col;
        }
    }

    else
    {
        HandleEOS();
    }
}

FileScriptStream::FileScriptStream(std::ifstream* fs) : buffer(0), pos(0), bufferLength(0), stream(fs)
{
    if (!stream)
        CheckGood();
    
    stream->seekg(0, std::ios_base::end);
		
	size_t len = 0;
    std::streamoff tell_len = stream->tellg(); 
	
	// Unfortunately tellg can return -1 in case of failure.
	if (tell_len < 0) 
	{
		RaiseException(Exception::ERROR_system, "bad file or input stream.");
	}
	else
	{
		len = (size_t)tell_len;
	}
    
	stream->seekg(0, std::ios_base::beg);
    
    CheckGood();
    
    buffer = (char*)Pika_malloc((len + 1));
    bufferLength = len;
    stream->read(buffer, len);
    try
	{
		CheckGood();
	} 
	catch(...)
	{ 
		// Failed constructor means the destructor will not be called.
		// This means we need to free the buffer before the constructor is exited.
		if (buffer)
			Pika_free(buffer);
		throw; // re-throw
	}
    buffer[bufferLength] = EOF;
}

FileScriptStream::~FileScriptStream() { Pika_free(buffer); }

void FileScriptStream::CheckGood()
{
    if (!stream || !stream->good())
    {
        RaiseException(Exception::ERROR_system, "bad file or input stream.");
    }
}

int FileScriptStream::Get() { return pos <= bufferLength && buffer ? buffer[pos++] : EOF; }
int FileScriptStream::Peek() { return pos <= bufferLength && buffer ? buffer[pos] : EOF; }
bool FileScriptStream::IsEof() { return pos > bufferLength || !buffer; }

void Tokenizer::CheckBom()
{
    u1 x = look;
    if (x == 0xEF) {
        x = script_stream->Get();
        if (x == 0xBB) {
            x = script_stream->Get();
            if (x == 0xBF) {  
                look = ' ';            
                GetNext();
                return;
            }
        }        
        state->SyntaxException(Exception::ERROR_syntax, 0, 0, "invalid or incomplete utf-8 Byte Order Marker found.");
    }
}

StringScriptStream::StringScriptStream(const char* buf, size_t len) 
    : buffer(0),
    pos(0),
    bufferLength(len)
{
    buffer = (char*)Pika_malloc((bufferLength + 1) * sizeof(char));
    Pika_memcpy(buffer, buf, bufferLength);
    buffer[bufferLength] = EOF;
}

StringScriptStream::~StringScriptStream() { Pika_free(buffer); }

int  StringScriptStream::Get()   { return pos <= bufferLength ? buffer[pos++] : EOF; }
int  StringScriptStream::Peek()  { return pos <= bufferLength ? buffer[pos]   : EOF; }
bool StringScriptStream::IsEof() { return pos > bufferLength; }

int  StdinScriptStream::Get()   { return std::cin.get(); }
int  StdinScriptStream::Peek()  { return std::cin.peek(); }
bool StdinScriptStream::IsEof() { return std::cin.eof();  }


REPLStream::REPLStream() : pos(0), bufferLength(0), is_eof(false)
{
    Pika_memzero(buffer, sizeof(buffer));
}

REPLStream::~REPLStream() {}

void REPLStream::NewLoop(const char* pmt)
{
    if (is_eof) 
        return;
    pos = bufferLength = 0;
    while (GetNewLine(pmt ? pmt : ">>>"))
    {
        // Line is non-empty
        if (bufferLength > 0)
        {
#if !defined(HAVE_READLINE)            
            Pika_addhistory(buffer);
#endif            
            return;
        }
    }
}

int REPLStream::Get()
{
    if (is_eof)
    {
        return EOF;
    }
    
    if (pos < bufferLength)
    {
        return buffer[pos++];
    }
    else 
    {            
        return EOI;
    }
    return EOF;
}

int REPLStream::Peek()
{
    if (is_eof)
    {
        return EOF;
    }
    
    if (pos < bufferLength)
    {
        return buffer[pos];
    }
    return EOI;
}

bool REPLStream::IsEof()
{
    return is_eof;
}

bool REPLStream::IsEoi()
{
    return !is_eof && pos >= bufferLength;
}

bool REPLStream::GetNewLine(const char* pmt)
{
    const char* prompt = pmt ? pmt : ">>>";
    Pika_memzero(buffer, sizeof(buffer));
#if !defined(HAVE_READLINE)
    const char* res = Pika_readline(prompt);
    is_eof = (res == NULL);
    if (!is_eof)
    {
        Pika_strcpy(buffer, res);
        bufferLength = strlen(buffer);
        pos = 0;
    }
#else        
    std::cout << prompt;
    std::cout.flush();
    void* res = std::cin.getline(buffer, REPL_BUFF_SZ);
    is_eof = (res == NULL);
    if (!is_eof)
    {
        bufferLength = strlen(buffer);
        pos = 0;
    }
#endif        
    return !is_eof;
}
}// pika
