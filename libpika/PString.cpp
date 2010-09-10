/*
 *  PString.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PValue.h"
#include "PString.h"
#include "PDef.h"
#include "PEnumerator.h"
#include "PArray.h"
#include "PType.h"
#include "PContext.h"
#include "PEngine.h"
#include "PPlatform.h"
#include "PTokenizer.h"
#include "PPackage.h"

namespace pika {

//////////////////////////////////////////StringEnumerator//////////////////////////////////////////

class StringEnumerator : public Enumerator
{
public:
    StringEnumerator(Engine* eng, String* str, bool elements = false)
            : Enumerator(eng),
            started(false),
            elements(elements),
            string(str),
            currentIndex(0) {}
            
    virtual ~StringEnumerator() {}
    
    virtual bool Rewind()
    {
        started = true;
        currentIndex = 0;
        return string && (currentIndex < string->length);
    }
    
    virtual bool IsValid()
    {
        if (!started)
            return Rewind();
        return currentIndex < string->length;
    }
    
    virtual void GetCurrent(Value& c)
    {
        if (elements)
        {
            char buff[2];
            buff[0] = string->buffer[currentIndex];
            buff[1] = 0;
            c = engine->AllocString(buff);
        }
        else
        {
            c = (pint_t)currentIndex;
        }
        return;
    }
    
    virtual void Advance()
    {
        ++currentIndex;
    }
    
    virtual void MarkRefs(Collector *c)
    {
        if (string)
            string->Mark(c);
    }
    
private:
    bool started;
    bool elements;
    String* string;
    size_t currentIndex;
};

///////////////////////////////////////////////String///////////////////////////////////////////////

PIKA_IMPL(String)

String::String(Engine* eng, size_t len, const char* s)
        : Basic(eng),
        next(0),
        length(len),
        hashcode(Pika_StringHash(s, len))
{
    if (length && s)
    {
        Pika_memcpy(&buffer[0], s, length);
    }
    buffer[length] = 0;
}

String* String::Create(Engine* eng, const char* str, size_t len, bool norun)
{
    size_t totalSize = len + sizeof(String);
    void* ret = Pika_malloc(totalSize);
    String* s = new(ret) String(eng, len, str);
    if (norun)
        eng->AddToGC(s);
    else
        eng->AddToGCNoRun(s);
    return s;
}

int String::Compare(const String* rhs) const
{
    return Pika_StringCompare(buffer, length, rhs->buffer, rhs->length);
}

String* String::Slice(size_t from, size_t to)
{
    if (to > length || from > length || from == to)
    {
        return engine->emptyString;
    }
    else if (from > to)
    {
        size_t len = from - to;
        engine->string_buff.Resize(len + 1);
        engine->string_buff[len] = '\0';
        char* begDst = engine->string_buff.GetAt(0);
        char* endDst = begDst + len;
        char* src    = &buffer[from - 1];
        
        for (char* curr = begDst; curr != endDst; ++curr)
        {
            *curr = *src--;
        }
        return engine->AllocString(begDst, len);
    }
    else
    {
        return engine->AllocString(&buffer[from], to - from);
    }
}

// TODO: rep should be either an integer or real
//       if rep is real
//          res = str * floor(rep)
//          res.concat(res.substring(decimal(rep) * res.length))

String* String::Multiply(String* vstr, pint_t rep)
{
    size_t  len  = vstr->length;
    Engine* eng  = vstr->GetEngine();
    GCPAUSE_NORUN(eng);
    if (rep < 0)
    {
        if (rep == PINT_MIN)
            RaiseException("String too large. cannot multiply string"PINT_FMT" times", PINT_MAX);
            
        vstr = vstr->Reverse();
        rep = -rep; // TODO: pint_t overflow
    }
    
    if (rep == 0)
    {
        return eng->emptyString;
    }
    else if (rep == 1)
    {
        return vstr;
    }
    else
    {
        size_t urep = rep;
        
        if ((PIKA_STRING_MAX_LEN / urep) < len)
        {
            RaiseException("String too large. Attempt to repeat string of length "SIZE_T_FMT" "PINT_FMT" times.", len, rep); // XXX: needs platform dep. format for size_t
        }
        size_t newlen = len * urep;
        eng->string_buff.Resize(newlen);
        
        for (size_t i = 0; i < urep; ++i)
        {
            Pika_memcpy(eng->string_buff.GetAt(i * len), &vstr->buffer[0], len * sizeof(char));
        }
        String* strret = eng->AllocString(eng->string_buff.GetAt(0), newlen);
        return strret;
    }
    return eng->emptyString;
}

String* String::Concat(String* a, String* b)
{
    ASSERT(a && b);
    Engine* eng = a->GetEngine();
    
    size_t alen = a->length;
    
    if (!alen)
        return b;
        
    size_t blen = b->length;
    
    if (!blen)
        return a;
        
    size_t  len = alen + blen;
    
    eng->string_buff.Resize(len + 1);
    eng->string_buff[0] = 0;
    
    Pika_memcpy(&eng->string_buff[0],    a->buffer, alen * sizeof(char));
    Pika_memcpy(&eng->string_buff[alen], b->buffer, blen * sizeof(char));
    
    eng->string_buff[len] = 0;
    
    return eng->AllocString(&eng->string_buff[0], len);
}

Array* String::Split(String* search)
{
    GCPAUSE_NORUN(engine);
    
    String* src = this;
    String* str = 0;
    
    size_t alen = src->length;
    engine->string_buff.Resize(alen + 1);
    
    if (alen)
        Pika_memcpy(&engine->string_buff[0], src->buffer, alen * sizeof(char));
        
    engine->string_buff[alen] = 0;
    char* strctx;
    char* tok = Pika_strtok(&engine->string_buff[0], search->buffer, &strctx);
    
    if ( !tok )
        return 0;
        
    Array* v = Array::Create(engine, 0, 0, 0);
    if ( !v )
        return 0;
        
    while (tok != 0)
    {
        str = engine->AllocString(tok);
        Value vtemp(str);
        v->Push(vtemp);
        tok = Pika_strtok(0, search->buffer, &strctx);
    }
    return v;
}

String* String::ToLower()
{
    TStringBuffer& scratch = engine->string_buff;
    scratch.Resize(length + 1);
    
    for (size_t i = 0; i < length; ++i)
        scratch[i] = tolower(buffer[i]);
        
    scratch[length] = '\0';
    return engine->AllocString( &scratch[0] );
}

String* String::ToUpper()
{
    TStringBuffer& scratch = engine->string_buff;
    scratch.Resize(length + 1);
    
    for (size_t i = 0; i < length; ++i)
        scratch[i] = toupper(buffer[i]);
        
    scratch[length] = '\0';
    return engine->AllocString( &scratch[0] );
}

String* String::ConcatSpace(String* a, String* b)
{
    return ConcatSep(a, b, ' ');
}

String* String::ConcatSep(String* a, String* b, char sep)
{
    ASSERT(a && b);
    Engine* eng = a->GetEngine();
    
    size_t alen = a->length;
    size_t blen = b->length;
    size_t len  = alen + blen + 1;
    
    eng->string_buff.Resize(len + 1);
    eng->string_buff[0] = 0;
    
    if (alen)
        Pika_memcpy(&eng->string_buff[0], a->buffer, alen * sizeof(char));
        
    eng->string_buff[alen] = sep;
    
    if (blen)
        Pika_memcpy(&eng->string_buff[alen + 1], b->buffer, blen * sizeof(char));
        
    eng->string_buff[len] = 0;
    
    return eng->AllocString(&eng->string_buff[0], len);
}

String::~String() {}

bool String::Finalize()
{
    if (!(gcflags & Persistent))
        gcflags |= ReadyToCollect;
    return false;
}

Type* String::GetType() const
{
    return engine->String_Type;
}

bool String::GetSlot(const Value& key, Value& result)
{
    if (key.tag == TAG_string && key.val.str == engine->length_String)
    {
        result.Set((pint_t)length);
        return true;
    }
    
    return engine->String_Type->GetField(key, result);
}

bool String::BracketRead(const Value& key, Value& result)
{
    if ((key.tag == TAG_integer))
    {
        // If its a valid index into the string.
        pint_t idx = key.val.integer;
        if ((idx >= 0) && (idx < (pint_t)length))
        {
            char buff = buffer[idx];
            result = engine->AllocString(&buff, 1);
            return true;
        }
        else
        {
            return false;
         }
    }
    return ThisSuper::BracketRead(key, result);
}
    
bool String::SetSlot(const Value& key, Value& value, u4 attr) { return false; }
bool String::CanSetSlot(const Value& key) { return false; }
bool String::DeleteSlot(const Value& key) { return false; }

Enumerator* String::GetEnumerator(String* enumtype)
{
    // "lines" "words" might be useful to have. line world enumerate each substring separated by '\n'. words are chars+numbers,
    //  separated by whitespace.
    if (enumtype == engine->elements_String || enumtype == engine->indices_String || enumtype == engine->emptyString)
    {
        Enumerator* e = 0;
        PIKA_NEW(StringEnumerator, e, (engine, this, enumtype == engine->elements_String));
        engine->AddToGC(e);
        return e;
    }
    return Basic::GetEnumerator(enumtype);
}

Value String::ToNumber()
{
    StringToNumber stonum(buffer, length);
    Value res(NULL_VALUE);
    if (stonum)
    {
        if (stonum.IsInteger())
        {
            res.Set( (pint_t)stonum.GetInteger() );
        }
        else
        {
            res.Set( (preal_t)stonum.GetReal() );
        }
    }
    return res;
}

bool String::ToInteger(pint_t& res)
{
    StringToNumber stonum(buffer, length);
    if (stonum)
    {
        if (stonum.IsInteger())
        {
            res = (pint_t)stonum.GetInteger();
        }
        else
        {
            res = (pint_t)stonum.GetReal();
        }
        return true;
    }
    return false;
}

bool String::ToReal(preal_t& res)
{
    StringToNumber stonum(buffer, length);
    if (stonum)
    {
        if (stonum.IsInteger())
        {
            res = (preal_t)stonum.GetInteger();
        }
        else
        {
            res = (preal_t)stonum.GetReal();
        }
        return true;
    }
    return false;
}

String* String::Reverse()
{
    if (length <= 1)
    {
        return this;
    }
    engine->string_buff.Resize(length);
    
    char* begDst = engine->string_buff.GetAt(0);
    char* endDst = begDst + length;
    char* src    = &buffer[length - 1];
    
    for (char* curr = begDst; curr != endDst; ++curr)
    {
        *curr = *src--;
    }
    return engine->AllocString(begDst, length);
}

String* String::Chomp(String* c)
{
    if (!c || 
        this->GetLength() == 0 || 
        c->GetLength() == 0)
    return this;
    const char* buff = this->GetBuffer();
    const char* cbuff = c->GetBuffer();
    for (size_t i = 0; i < this->GetLength(); ++i)
    {
        const char* s = strchr(cbuff, buff[i]);
        if ( !s )
        {
            return this->Slice(i, this->GetLength());
        }
    }
    return engine->emptyString;
}

/////////////////////////////////////////////StringApi//////////////////////////////////////////////

class StringApi
{
public:
    static int toInteger(Context* ctx, Value& self)
    {
        String* str = self.val.str;
        pint_t i;
        if (str->ToInteger(i))
        {
            ctx->Push(i);
        }
        else
        {
            RaiseException("Attempt to convert string \"%s\" to integer.", str->buffer);
        }
        return 1;
    }
    
    static int replaceChar(Context* ctx, Value& self)
    {
        Engine* engine = ctx->GetEngine();
        String* str = self.val.str;
        String* r = ctx->GetStringArg(0);
        String* w = ctx->GetStringArg(1);
        if (r->GetLength() != 1 || w->GetLength() != 1)        
            return 0;
        char rx = r->GetBuffer()[0];
        char wx = w->GetBuffer()[0];
        Buffer<char> buf;
        buf.Resize((size_t)str->GetLength());
        Pika_memcpy(buf.GetAt(0), str->GetBuffer(), str->GetLength());
        char* pos = 0;
        while ((pos = strrchr(buf.GetAt(0), rx)))
            *pos = wx;
        ctx->Push(engine->AllocString(buf.GetAt(0), buf.GetSize()));
        return 1;
    }
    
    static int toReal(Context* ctx, Value& self)
    {
        String* str = self.val.str;
        preal_t r;
        if (str->ToReal(r))
        {
            ctx->Push(r);
        }
        else
        {
            RaiseException("Attempt to convert string \"%s\" to real.", str->buffer);
        }
        return 1;
    }
    
    static int toUpper(Context* ctx, Value& self)
    {
        String *str = self.val.str->ToUpper();
        
        if (str)
        {
            ctx->Push( str );
            return 1;
        }
        return 0;
    }
    
    static int toLower(Context* ctx, Value& self)
    {
        String *str = self.val.str->ToLower();
        
        if (str)
        {
            ctx->Push( str );
            return 1;
        }
        return 0;
    }
    
    static int splitAt(Context* ctx, Value& self)
    {
        String* src = self.val.str;
        pint_t  at  = ctx->GetIntArg(0);
        size_t  len = src->GetLength();
        Engine* eng = ctx->GetEngine();
        
        if (at < 0 || (size_t)at >= len)
            RaiseException("cannot slit string of length: "PINT_FMT" at position: "PINT_FMT, (pint_t)len, at);
            
        String* stra = eng->AllocString(src->GetBuffer(), at);
        ctx->Push(stra);
        
        String* strb = eng->AllocString(src->GetBuffer() + (size_t)at, len - (size_t)at);
        ctx->Push(strb);
                
        return 2;
    }
    
    static int split(Context* ctx, Value& self)
    {
        u2 argc = ctx->GetArgCount();
        Value* argv = ctx->GetArgs();
        
        if ((argc > 0) && argv[0].IsString())
        {
            Array* v = self.val.str->Split(argv[0].val.str);
            ctx->Push(v);
            return 1;
        }
        return 0;
    }
    
    static int slice(Context* ctx, Value& self)
    {
        pint_t  from = ctx->GetIntArg(0);
        pint_t  to   = ctx->GetIntArg(1);
        String* str  = self.val.str;
        String* res  = 0;
        
        if (to >= 0 && from >= 0)
        {
            res = str->Slice(from, to);
        }
        
        if (res)
        {
            ctx->Push(res);
            return 1;
        }
        return 0;
    }
    
    static int reverse(Context* ctx, Value& self)
    {
        String* src = self.val.str;
        String* res = src->Reverse();
        ctx->Push(res);
        return 1;
    }
    
    static int is_letter(Context* ctx, Value& self)
    {
        String* src = self.val.str;
        size_t len = src->GetLength();
        const char* buff = src->GetBuffer();
        if (!len)
        {
            ctx->PushFalse();        
            return 1;
        }
        for (size_t a = 0; a < len; ++a) {
            if (!IsLetter(buff[a])) {
                ctx->PushFalse();
                return 1;
            }
        }
        ctx->PushTrue();
        return 1;
    }
    
    static int is_digit(Context* ctx, Value& self)
    {
        String* src = self.val.str;
        size_t len = src->GetLength();
        const char* buff = src->GetBuffer();
        
        if (!len)
        {
            ctx->PushFalse();        
            return 1;
        }
        
        for (size_t a = 0; a < len; ++a)
        {
            if (!IsDigit(buff[a]))
            {
                ctx->PushFalse();
                return 1;
            }
        }
        ctx->PushTrue();
        return 1;
    }
    
    static int is_letterOrDigit(Context* ctx, Value& self)
    {
        String* src = self.val.str;
        size_t len = src->GetLength();
        const char* buff = src->GetBuffer();
        
        if (!len)
        {
            ctx->PushFalse();        
            return 1;
        }
        
        for (size_t a = 0; a < len; ++a)
        {
            if (!IsLetterOrDigit(buff[a]))
            {
                ctx->PushFalse();
                return 1;
            }
        }
        ctx->PushTrue();
        return 1;
    }
    
    static int is_ascii(Context* ctx, Value& self)
    {
        String* src = self.val.str;
        size_t len = src->GetLength();
        const char* buff = src->GetBuffer();
        
        if (!len)
        {
            ctx->PushFalse();        
            return 1;
        }
        
        for (size_t a = 0; a < len; ++a)
        {
            if (!IsAscii(buff[a]))
            {
                ctx->PushFalse();
                return 1;
            }
        }
        ctx->PushTrue();
        return 1;
    }
    
    static int is_whitespace(Context* ctx, Value& self)
    {
        String* src = self.val.str;
        size_t len = src->GetLength();
        const char* buff = src->GetBuffer();
        
        if (!len)
        {
            ctx->PushFalse();        
            return 1;
        }
        
        for (size_t a = 0; a < len; ++a)
        {
            if (!IsSpace(buff[a]))
            {
                ctx->PushFalse();
                return 1;
            }
        }
        ctx->PushTrue();
        return 1;
    }
    
    static int firstOf(Context* ctx, Value& self)
    {
        String* setStr = ctx->GetStringArg(0);
        String* str = self.val.str;
        
        size_t pos = strcspn(str->GetBuffer(), setStr->GetBuffer());
        
        ctx->Push((pint_t)pos);
        return 1;
    }
    
    static int firstNotOf(Context* ctx, Value& self)
    {
        String* setStr = ctx->GetStringArg(0);
        String* str = self.val.str;
        
        size_t pos = strspn(str->GetBuffer(), setStr->GetBuffer());
        ctx->Push((pint_t)pos);
        return 1;
    }
    
    static int lastOf(Context* ctx, Value& self)
    {
        String* setStr = ctx->GetStringArg(0);
        String* str = self.val.str;
        
        if (str->length != 0 && setStr->length != 0)
        {
            for (pint_t src_i = (pint_t)str->length - 1; src_i >= 0; --src_i)
            {
                for (pint_t set_i = 0; set_i < (pint_t)setStr->length; ++set_i)
                {
                    if (setStr->buffer[set_i] == str->buffer[src_i])
                    {
                        ctx->Push((pint_t)src_i);
                        return 1;
                    }
                }
            }
        }
        ctx->Push((pint_t)0);
        return 1;
    }
    
    static int lastNotOf(Context* ctx, Value& self)
    {
        String* setStr = ctx->GetStringArg(0);
        String* str = self.val.str;
        
        if (str->length != 0 && setStr->length != 0)
        {
            for (pint_t src_i = (pint_t)str->length - 1; src_i >= 0; --src_i)
            {
                bool bfound = false;
                
                for (pint_t set_i = 0; set_i < (pint_t)setStr->length; ++set_i)
                {
                    if (setStr->buffer[set_i] == str->buffer[src_i])
                    {
                        bfound = true;
                        break;
                    }
                }
                if (!bfound)
                {
                    ctx->Push((pint_t)src_i);
                    return 1;
                }
            }
        }
        ctx->Push((pint_t)0);
        return 1;
    }
    
    static int charAt(Context* ctx, Value& self)
    {
        String* str = self.val.str;
        pint_t   idx = ctx->GetIntArg(0);
        Engine* eng = ctx->GetEngine();
        
        if (idx >= 0 && ((size_t)idx < str->length))
        {
            ctx->Push(eng->AllocString(str->buffer + idx, 1));
        }
        else
        {
            ctx->Push(eng->emptyString);
        }
        return 1;
    }
    
    static int toNumber(Context* ctx, Value& self)
    {
        String* str = self.val.str;
        Value res = str->ToNumber();
        ctx->Push( res );
        return 1;
    }
    
    /** Converts a byte [0-ff] to a String. */
    static int fromByte(Context* ctx, Value&)
    {
        Engine* eng = ctx->GetEngine();
        //u1      uch = static_cast<u1>(ctx->GetIntArg(0) & 0xff);
        char uch = ctx->GetIntArg(0);
        String* res = eng->AllocString((char*)&uch, 1);
        
        ctx->Push(res);
        return 1;
    }
    
    /** Returns a byte [0-ff]. */
    static int byteAt(Context* ctx, Value& self)
    {
        u2 argc = ctx->GetArgCount();
        Value* argv = ctx->GetArgs();
        
        if ((argc == 1) && ctx->GetEngine()->ToInteger(argv[0]))
        {
            pint_t index = argv[0].val.integer;
            String *str = self.val.str;
            
            if (index >= 0 && index < (pint_t)str->length)
            {
                u1 uch = static_cast<u1>(str->buffer[index]);
                ctx->Push((pint_t)uch);
            }
            else
            {
                RaiseException(Exception::ERROR_index, "String.byteAt: attempt to get byte at position \""SIZE_T_FMT"\" from string of length \""SIZE_T_FMT"\".\n",index,str->length);
                return 0;
            }
            return 1;
        }
        return 0;
    }
    
    static int concat(Context* ctx, Value&)
    {
        u2 argc = ctx->GetArgCount();
        Engine* eng = ctx->GetEngine();
        
        if (argc == 0)
        {
            ctx->Push(eng->emptyString);
            return 1;
        }
        else if (argc == 1)
        {
            String* res = eng->ToString(ctx, ctx->GetArg(0));
            ctx->Push(res);
            return 1;
        }
        
        GCPAUSE(eng);
        Value* args = ctx->GetArgs();
        size_t neededsize = 0;
        
        for (u2 a = 0; a < argc; ++a)
        {
            String* str = eng->ToString(ctx, ctx->GetArg(a));
            size_t strlen = str->length;
            size_t newsize = neededsize + strlen;
            if (newsize < neededsize || newsize > PIKA_STRING_MAX_LEN)
                RaiseException("string.concatSpace: resulting string is too large.");
            neededsize = newsize;
            args[a].Set(str);
        }
        
        char* buff = (char*)Pika_malloc(neededsize);
        if (!buff)
        {
            RaiseException("string.concatSpace: insufficient memory to create the resulting string.");
        }
        
        try
        {
            Value* curr = args;
            Value* end = curr + argc;
            
            for (size_t pos = 0; curr != end; curr++)
            {
                // copy the next string
                String* currStr = curr->val.str;
                Pika_memcpy(buff + pos, currStr->GetBuffer(), currStr->GetLength());
                pos += currStr->GetLength();
            }
            String* resultString = eng->AllocString(buff, neededsize);
            ctx->Push(resultString);
        }
        catch (...)
        {
            Pika_free(buff);
            throw;
        }
        Pika_free(buff);
        return 1;
    }
    
    static int concatSpace(Context* ctx, Value&)
    {
        u2 argc = ctx->GetArgCount();
        Engine* eng = ctx->GetEngine();
        
        if (argc == 0)
        {
            ctx->Push(eng->emptyString);
            return 1;
        }
        else if (argc == 1)
        {
            String* res = eng->ToString(ctx, ctx->GetArg(0));
            ctx->Push(res);
            return 1;
        }
        
        GCPAUSE(eng);
        Value* args = ctx->GetArgs();
        const char* gluestr = " ";
        size_t const gluesz = 1;
        size_t neededsize = gluesz * (argc - 1);
        
        for (u2 a = 0; a < argc; ++a)
        {
            String* str = eng->ToString(ctx, ctx->GetArg(a));
            size_t strlen = str->length;
            size_t newsize = neededsize + strlen;
            if (newsize < neededsize || newsize > PIKA_STRING_MAX_LEN)
                RaiseException("string.concatSpace: resulting string is too large.");
            neededsize = newsize;
            args[a].Set(str);
        }
        char* buff = (char*)Pika_malloc(neededsize);
        
        if (!buff)
        {
            RaiseException("string.concatSpace: insufficient memory to create the resulting string.");
        }
        try
        {
            Value* curr = args;
            Value* end = curr + argc;
            
            for (size_t pos = 0;curr != end; curr++)
            {
                //
                // copy the next string
                //
                String* currStr = curr->val.str;
                Pika_memcpy(buff + pos, currStr->GetBuffer(), currStr->GetLength());
                pos += currStr->GetLength();
                //
                // copy the glue string
                //
                if ((curr + 1) != end)
                {
                    Pika_memcpy(buff + pos, gluestr, gluesz);
                    pos += gluesz;
                }
            }
            String* resultString = eng->AllocString(buff, neededsize);
            ctx->Push(resultString);
        }
        catch (...)
        {
            Pika_free(buff);
            throw;
        }
        Pika_free(buff);
        return 1;
    }
    
    static int chomp(Context* ctx, Value& self)
    {
        GCPAUSE(ctx->GetEngine());
        String* vstr = self.val.str;
        String* other = (ctx->GetArgCount() == 1 && !ctx->IsArgNull(0)) ?
                    ctx->GetStringArg(0) : 
                    ctx->GetEngine()->AllocString(" \r\n\t\v\f"); // space,cr,nl,tab,vtab,formfeed
        
        String* res = vstr->Chomp(other);
        if (res)
            ctx->Push(res);
        else
            ctx->Push(ctx->GetEngine()->emptyString);
        return 1;
    }    
    
    static int times(Context* ctx, Value& self)
    {
        GCPAUSE(ctx->GetEngine());    
        String* vstr = self.val.str;
        size_t  len  = vstr->length;
        pint_t   rep  = ctx->GetIntArg(0);
        Engine* eng  = ctx->GetEngine();
        
        if (rep <= 0)
        {
            ctx->Push(eng->emptyString);
        }
        else if (rep == 1)
        {
            ctx->Push(vstr);
        }
        else
        {
            size_t urep = rep;
            
            if ((PIKA_STRING_MAX_LEN / urep) < len)
            {
                RaiseException("string.times: string too large to repeat "PINT_FMT" times.", rep);
            }
            size_t newlen = len * urep;
            eng->string_buff.Resize(newlen);
            
            for (size_t i = 0; i < urep; ++i)
            {
                Pika_memcpy(eng->string_buff.GetAt(i * len), &vstr->buffer[0], len * sizeof(char));
            }
            String* strret = eng->AllocString(eng->string_buff.GetAt(0), newlen);
            ctx->Push(strret);
        }
        return 1;
    }
    
    static int init(Context* ctx, Value&)
    {
        Engine* eng = ctx->GetEngine();
        GCPAUSE_NORUN(eng);
        
        Value& arg = ctx->GetArg(0);
        String* res = arg.IsString() ? arg.val.str : eng->ToString(ctx, arg);
        ctx->Push(res);
        return 1;
    }
    
    static int toString(Context* ctx, Value& self)
    {
        ctx->Push(self);
        return 1;
    }
    
    static void Constructor(Engine* eng, Type* type, Value& res)
    {
        res.Set(eng->emptyString);
    }
};

void String::StaticInitType(Engine* eng)
{
    static RegisterFunction String_Methods[] =
    {
        // name, function, argc, strict, varargs
        { "replaceChar",	StringApi::replaceChar,         2, DEF_STRICT },
        { "toInteger",  	StringApi::toInteger,           0, 0 },
        { "toReal",     	StringApi::toReal,              0, 0 },
        { "toNumber",       StringApi::toNumber,            0, 0 },
        { "toLower",        StringApi::toLower,             0, 0 },
        { "toUpper",        StringApi::toUpper,             0, 0 },
        { "charAt",         StringApi::charAt,              1, DEF_STRICT },
        { "split",          StringApi::split,               1, 0 },
        { "splitAt",        StringApi::splitAt,             1, 0 },
        { "byteAt",     	StringApi::byteAt,              1, 0 },
        { "firstOf",        StringApi::firstOf,             2, DEF_VAR_ARGS },
        { "firstNotOf",     StringApi::firstNotOf,          2, DEF_VAR_ARGS },
        { "lastOf",         StringApi::lastOf,              2, DEF_VAR_ARGS },
        { "lastNotOf",      StringApi::lastNotOf,           2, DEF_VAR_ARGS },
        { "substring",      StringApi::slice,               2, DEF_STRICT },
        { OPSLICE_STR,      StringApi::slice,               2, DEF_STRICT },
        { "chomp",          StringApi::chomp,               1, DEF_VAR_ARGS },
        { "times",          StringApi::times,               1, DEF_STRICT },
        { "opMul",          StringApi::times,               1, DEF_STRICT },
        { "toString",       StringApi::toString,            0, DEF_STRICT },
        { "reverse",        StringApi::reverse,             0, DEF_STRICT },
        { "letter?",        StringApi::is_letter,           0, DEF_STRICT },  
        { "letterOrDigit?", StringApi::is_letterOrDigit,    0, DEF_STRICT },
        { "digit?",         StringApi::is_digit,            0, DEF_STRICT },
        { "ascii?",         StringApi::is_ascii,            0, DEF_STRICT },
        { "whitespace?",    StringApi::is_whitespace,       0, DEF_STRICT },    
    };
    
    static RegisterFunction String_ClassMethods[] =
    {
        { "cat",        StringApi::concat,      0, DEF_VAR_ARGS },
        { "catSp",      StringApi::concatSpace, 0, DEF_VAR_ARGS },
        { "fromByte",   StringApi::fromByte,    1, DEF_STRICT },
        { NEW_CSTR,     StringApi::init,        1, DEF_STRICT },
    };
    eng->String_Type = Type::Create(eng, eng->AllocString("String"), eng->Basic_Type, StringApi::Constructor, eng->GetWorld());
    
    eng->String_Type->SetFinal(true);
    eng->String_Type->SetAbstract(true);    
    eng->String_Type->EnterMethods(String_Methods, countof(String_Methods));
    eng->String_Type->EnterClassMethods(String_ClassMethods, countof(String_ClassMethods));
    Value string_MAX((pint_t)PIKA_STRING_MAX_LEN);
    eng->String_Type->SetSlot("MAX", string_MAX);
    eng->GetWorld()->SetSlot("String", eng->String_Type);
}

}// pika
