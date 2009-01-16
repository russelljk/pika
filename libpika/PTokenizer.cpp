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

#define PIKA_keyword(x) { x, Token2String::GetNames()[x - TOK_global], 0 }

namespace pika
{

struct KeywordDescriptor
{
    ETokenType ttype;
    const char* name;
    size_t length;
};

static KeywordDescriptor static_keywords[] =
{
    // Keywords.
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
    PIKA_keyword(TOK_unless),
    PIKA_keyword(TOK_foreach),
    PIKA_keyword(TOK_for),
    PIKA_keyword(TOK_else),
    PIKA_keyword(TOK_then),
    PIKA_keyword(TOK_do),
    PIKA_keyword(TOK_in),
    PIKA_keyword(TOK_of),
    PIKA_keyword(TOK_is),
    PIKA_keyword(TOK_has),
    PIKA_keyword(TOK_true),
    PIKA_keyword(TOK_false),
    PIKA_keyword(TOK_while),
    PIKA_keyword(TOK_until),
    PIKA_keyword(TOK_try),
    PIKA_keyword(TOK_catch),
    PIKA_keyword(TOK_raise),
    PIKA_keyword(TOK_break),
    PIKA_keyword(TOK_continue),
    PIKA_keyword(TOK_select),
    PIKA_keyword(TOK_yield),
    PIKA_keyword(TOK_mod),
    PIKA_keyword(TOK_and),
    PIKA_keyword(TOK_or),
    PIKA_keyword(TOK_xor),
    PIKA_keyword(TOK_not),
    PIKA_keyword(TOK_property),
    PIKA_keyword(TOK_finally),
    PIKA_keyword(TOK_package),
    PIKA_keyword(TOK_with),
    PIKA_keyword(TOK_case),
    PIKA_keyword(TOK_new),
    PIKA_keyword(TOK_typeof),
    PIKA_keyword(TOK_bind),
    PIKA_keyword(TOK_assert),
    PIKA_keyword(TOK_delete),
    PIKA_keyword(TOK_to),
    PIKA_keyword(TOK_downto),
    PIKA_keyword(TOK_by),
    PIKA_keyword(TOK_class),
    PIKA_keyword(TOK_div),
    PIKA_keyword(TOK_gen),
};

#define NumKeywordDescriptors ((sizeof (static_keywords))/(sizeof (KeywordDescriptor)))
    
INLINE bool IsAscii(int x)  { return isascii(x) != 0; }
INLINE bool IsLetter(int x) { return IsAscii(x) && (isalpha(x) != 0); }
INLINE bool IsDigit(int x)  { return IsAscii(x) && (isdigit(x) != 0); }
INLINE bool IsSpace(int x)  { return IsAscii(x) && (isspace(x) != 0); }

INLINE bool IsUpper(int x)  { return IsAscii(x) && (isupper(x) != 0); }
INLINE bool IsLower(int x)  { return IsAscii(x) && (islower(x) != 0); }

INLINE int  ToLower(int x)  { return(IsLetter(x)) ? tolower(x) : x; }
INLINE int  ToUpper(int x)  { return(IsLetter(x)) ? toupper(x) : x; }

INLINE bool IsLetterOrDigit(int x)   { return IsAscii(x) && (isalnum(x) != 0); }
INLINE bool IsIdentifierExtra(int x) { return x == '_'   || x == '$'; }
INLINE bool IsValidDigit(int x)      { return IsDigit(x) || x == '_'; }

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
                
                val *= pow((float)radix, (double)exponent);
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

// StringToNumber //////////////////////////////////////////////////////////////////////////////////

StringToNumber::StringToNumber(const char* str, size_t len, bool eatwsp, bool use_html_hex, bool must_consume)
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
            EatWhitesp();
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
            EatWhitesp();
        }
        is_number = !must_consume || (curr >= end);
    }
}

void StringToNumber::EatWhitesp()
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
    EatWhitesp();
    
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

// Tokenizer ///////////////////////////////////////////////////////////////////////////////////////

Tokenizer::Tokenizer(CompileState* s, FILE* fs)
        : tokenBegin(0),
        tokenEnd(0),
        state(s),
        tabIndentSize(4),
        line(1),
        col(1),
        ch(1),
        prevline(1),
        prevcol(1),
        prevch(1),
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
        : tokenBegin(0),
        tokenEnd(0),        
        state(s),
        tabIndentSize(4),
        line(1),
        col(1),
        ch(1),
        prevline(1),
        prevcol(1),
        prevch(1),
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
/*
Tokenizer::Tokenizer(CompileState* s, ScriptStream* ss)
        : tokenBegin(0),
        state(s),
        tokenEnd(0),
        line(1),
        col(1),
        ch(1),
        prevline(1),
        prevcol(1),
        prevch(1),
        look(EOF),
        script_stream(ss),
        tabIndentSize(4)
{
    PrepKeywords();
    GetLook();
}
*/
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
    else if (look == '\'' || look == '\"') 
    {
        ReadString(); 
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

void Tokenizer::ReadIdentifier()
{
    // [a-zA-Z$_][a-zA-Z0-9$_]*[?!]?
    
    if (!(IsLetter(look) || IsIdentifierExtra(look))) // make sure we start with an appropriate character.
        state->SyntaxException(Exception::ERROR_syntax, line, "Expecting letter, '$' or '_' while reading identifier");
        
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
        bool iskeyword = false;
        
        for (size_t i = 0; i < NumKeywordDescriptors; ++i)
        {
            if (toklen == static_keywords[i].length)
            {
                if (StrCmpWithSize(GetBeginPtr(), static_keywords[i].name, toklen) == 0)
                {
                    iskeyword = true;
                    tokenType = static_keywords[i].ttype;
                    break;
                }
            }
        }
        
        if (iskeyword)
            return;
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
    int termchar = look;
    
    if (termchar != '\'' && termchar != '\"')
        return;
        
    GetLook();
    
    while (look != termchar && !IsEndOfStream())
    {
        if (look == '\\')
        {
            GetLook(); // move past the back slash
            GetLook(); // move past next char
        }
        else
        {
            GetLook();
        }
    }
    
    if (IsEndOfStream() || look != termchar)
        state->SyntaxException(Exception::ERROR_syntax, prevline, "unclosed string literal");
        
    GetLook();
    
    const char* strbeg = GetBeginPtr() + 1; // move past the opening " or '
    const char* strend = GetEndPtr() - 1; // move in front of the closing " or '
    
    ASSERT(strend > strbeg);
    
    size_t len = strend - 1 - strbeg;
    char* strtemp = (char*)Pika_malloc((len + 1) * sizeof(char));
    
    Pika_memcpy(strtemp, strbeg, len * sizeof(char));
    
    strtemp[len] = '\0';
    tokenVal.str = Pika_TransformString(strtemp, len, &tokenVal.len);
    tokenType    = TOK_stringliteral;
    
    Pika_free(strtemp);
}

void Tokenizer::ReadControl()
{
    switch (look)
    {
    case ':':
    {
        tokenType = look;
        GetLook();
        
        if (look == '=')
        {
            tokenType = TOK_assign;
            GetLook();
        }
    }
    break;
        
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
            GetLook();
            ReadSingleLineComment();
            return GetNext();
        }
    }
    break;

    case '(':
    {
        tokenType = look;
        GetLook();

        if (look == '*')
        {
            GetLook();
            ReadMultiLineComment();
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
            // .. , slice operator
            tokenType = TOK_slice;
            GetLook();
            
            if (look == '.')
            {
                // ... , varargs decl
                tokenType = TOK_rest;
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
        
    case '@':
    {
        tokenType = TOK_cat;
        GetLook();
        
        if (look == '@')
        {
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
        
    default:    
        tokenType = look;
        GetLook();    
    }
}

void Tokenizer::ReadMultiLineComment()
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
        state->SyntaxException(Exception::ERROR_syntax, line, "unclosed multi-line comment");
    }
}

void Tokenizer::ReadSingleLineComment()
{
    while (!IsEndOfStream() && look != '\n')
        GetLook();
}

bool Tokenizer::IsEndOfStream()
{
    return script_stream->IsEof();
}

int Tokenizer::Peek()
{
    return script_stream ? script_stream->Peek() : EOF;
}

void Tokenizer::NumberParseError(const char* msg)
{
    state->SyntaxException(Exception::ERROR_syntax, line, msg);
}

void Tokenizer::GetLook()
{
    if (script_stream && !script_stream->IsEof())
    {
        look = script_stream->Get();
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
                        col = ch = 1;
                    }
                    // otherwise CR LF
                }
            }
            
            if (look == '\n')
            {
                // LF
                ++line;
                col = ch = 1;
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

}// pika
