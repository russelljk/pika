/*
 *  PString.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PValue.h"
#include "PString.h"
#include "PDef.h"
#include "PArray.h"
#include "PType.h"
#include "PContext.h"
#include "PEngine.h"
#include "PPlatform.h"
#include "PTokenizer.h"
#include "PPackage.h"
#include "PNativeBind.h"

namespace pika {

//////////////////////////////////////////StringIterator//////////////////////////////////////////

class StringIterator : public Iterator
{
public:
    StringIterator(Engine* eng, Type* itertype, String* str, IterateKind k)
            : Iterator(eng, itertype),
            valid(false),
            kind(k),
            string(str),
            currentIndex(0)
    {
        valid = Rewind();
    }
    
    virtual ~StringIterator() {}
    
    virtual bool Rewind()
    {        
        currentIndex = 0;
        return string && (currentIndex < string->length);
    }
    
    virtual bool ToBoolean()
    {
        return valid;
    }

    INLINE String* GetElement()
    {
        char buff[2];
        buff[0] = string->buffer[currentIndex];
        buff[1] = 0;
        return engine->GetString(buff);
    }
        
    virtual int Next(Context* ctx)
    {
        if (currentIndex < string->length)
        {
            
            int retc = 0;
            switch (kind) {
            case IK_values:
                ctx->Push(GetElement());
                retc = 1;
                break;
            case IK_keys:                
                ctx->Push((pint_t)currentIndex);
                retc = 1;
                break;
            case IK_both:
                ctx->Push((pint_t)currentIndex);
                ctx->Push(GetElement());
                retc = 2;
                break;
            case IK_default:
            default:
                if (ctx->GetRetCount() == 2)
                {
                    ctx->Push((pint_t)currentIndex);
                    ctx->Push(GetElement());
                    retc = 2;
                }
                else
                {
                    ctx->Push(GetElement());
                    retc = 1;
                }
                break;
            }
            currentIndex++;
            valid = true;
            return retc;
        }
        else
        {
            valid = false;
        }
        return 0;
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
    bool valid;
    IterateKind kind;
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

Value String::ToValue()
{
    Value v(this);
    return v;
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
    
    return eng->GetString(&eng->string_buff[0], len);
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

PIKA_DOC(String_iterate, "/([kind])\
\n\
Returns an [Iterator iterator] for this string. The values iterated depend on the value of |kind|. If |kind| is equal to 'elements', '' or not specified then characters will be enumerated. If |kind| is 'indices' then indices will be enumerated.")

Iterator* String::Iterate(String* iter_type)
{
    // "lines" "words" might be useful to have. line world enumerate each substring separated by '\n'. words are chars+numbers,
    //  separated by whitespace.
    
    IterateKind kind = IK_default;
    if (iter_type == engine->elements_String) {
        kind = IK_values;
    } else if (iter_type == engine->keys_String) {
        kind = IK_keys;
    } else if (iter_type != engine->emptyString) {
        return Iterator::Create(engine, engine->Iterator_Type);
    }

    Iterator* e = 0;
    PIKA_NEW(StringIterator, e, (engine, engine->Iterator_Type, this, kind));
    engine->AddToGC(e);
    return e;
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

String* String::Strip(StringDirection sd, const char* what)
{
    if (!this->GetLength())
        return this;
    
    const char* start = this->GetBuffer();
    const char* end = this->GetBuffer() + (this->GetLength() - 1);
        
    if (sd != SD_right) {
        while (strchr(what, *start) && start < end) {
            start++;
        }
        
        if (start >= end)
            return engine->emptyString;
    }
    
    if (sd != SD_left) {
        while (end > start && strchr(what, *end)) {
            end--;
        }
    }
    return engine->AllocString(start, end - start + 1);
}

String* String::Chomp(String* c)
{
    if (!c || this->GetLength() == 0 || c->GetLength() == 0) {
        return this;
    }
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

INLINE void BufferAddCString(Buffer<char>& buff, const char* str, size_t amt)
{
    if (!amt) return;
    size_t const pos = buff.GetSize();
    buff.Resize(buff.GetSize() + amt);
    Pika_memcpy(buff.GetAt(pos), str, amt);
}

INLINE void BufferAddString(Buffer<char>& buff, String* str)
{
    return BufferAddCString(buff, str->GetBuffer(), str->GetLength());
}

String* String::Escape(Array* entities, Array* replacements)
{
    size_t const num_replacements = replacements->GetLength();
    if (entities->GetLength() != num_replacements)
    {
        RaiseException(Exception::ERROR_runtime, "String.escape - The entities String and the replacement Array must be the same size.");
    }
        
    typedef std::pair<String*, String*> ReplacementPair;
    
    Buffer<ReplacementPair> lookups(num_replacements);
    
    for (size_t i=0; i < num_replacements; ++i)
    {
        Value& e = (*entities)[i];
        Value& r = (*replacements)[i];
        
        if (!(e.IsString() && r.IsString()))
        {
            RaiseException(Exception::ERROR_type, "String.escape - All elements in both arguments must be of type String.");
        }
        lookups[i].first = e.val.str;
        lookups[i].second = r.val.str;
    }
    
    Buffer<char> buff;
    buff.SetCapacity(this->GetLength());    
    const char* curr = this->GetBuffer();
    const char* end = curr + this->GetLength();
    bool ismatched = false;
    
    while (curr < end) {        
        ismatched = false;
        
        for (size_t i = 0; i < lookups.GetSize(); ++i) {
            ReplacementPair& p = lookups[i];
            String* ent = p.first;
            size_t const amt = ent->GetLength();
            
            if (strncmp(curr, ent->GetBuffer(), amt) == 0) {
                curr += amt;                
                BufferAddString(buff, p.second);
                ismatched = true;
                break;
            }
        }
        
        if (ismatched) 
            continue;

        buff.Push(*curr++);
    }
    
    ptrdiff_t amt = curr - end;
    if (amt > 0) {
        BufferAddCString(buff, curr, amt);
    }
    return engine->GetString(buff.GetAt(0), buff.GetSize());
}

String* String::Escape(String* entities, Array* replacements)
{
    if (entities->GetLength() != replacements->GetLength())
    {
        RaiseException(Exception::ERROR_runtime, "String.escape - The entities String and the replacement Array must be the same size.");
    }
    size_t num_replacements = replacements->GetLength();
    Buffer<String*> cached(num_replacements);
    Pika_memzero(cached.GetAt(0), num_replacements * sizeof(String*));
    Buffer<char> buff;
    buff.SetCapacity(this->GetLength());
    size_t num_ents = entities->GetLength();
    const char* curr = this->GetBuffer();
    const char* end = curr + this->GetLength();
    const char* ent_start = entities->GetBuffer();
    
    while (curr < end) 
    {
        const char* ent_ptr = strchr(ent_start, *curr);        
        if (!ent_ptr) 
        {
            buff.Push(*curr);
        } 
        else 
        {
            ptrdiff_t ent_idx = ent_ptr - ent_start;
            String* rep = cached[ent_idx];
            assert(ent_idx < num_ents);
            
            if (!rep) 
            {
                Value& v = (*replacements)[ent_idx];
                if (!v.IsString()) 
                {
                    RaiseException(Exception::ERROR_type, "String.escape replacement Array element %d. Excepted type String.", (pint_t)ent_idx);
                }
                cached[ent_idx] = rep = v.val.str;
            }
            BufferAddString(buff, rep);
        }
        ++curr;
    }
    ptrdiff_t amt = curr - end;
    BufferAddCString(buff, curr, amt);
    return engine->GetString(buff.GetAt(0), buff.GetSize());
}

String* String::sprintp(Engine*  eng,    // context
                        String*  fmt,    // format string
                        u2       argc,   // argument count + 1
                        String*  args[]) // arguments, args[0] is ignored
{
    TStringBuffer& buff = eng->string_buff;
    
    if (fmt->GetLength())
    {
        buff.Clear();
        
        const char* cfmt = fmt->GetBuffer();
        const char* fmtend = cfmt + fmt->GetLength();
        
        while (cfmt < fmtend)
        {
            int ch = *cfmt++;
            if (ch == '\\' && cfmt < fmtend)
            {
                ch = *cfmt++;
                if (ch != '$')
                    buff.Push('\\');
                buff.Push(ch);
                continue;
            }
            else if (ch == '$')
            {
                unsigned pos = 0;
                ch = *cfmt;
                
                if (!isdigit(ch))
                    RaiseException("Expected number after %c.", '$');
                    
                while (cfmt++ < fmtend && isdigit(ch))
                {
                    pos = pos * 10 + (ch - '0');
                    ch = *cfmt;
                }
                
                if (pos < argc)
                {
                    String* posstr = args[pos];
                    size_t oldsize = buff.GetSize();
                    buff.Resize(oldsize + posstr->GetLength());
                    Pika_memcpy(buff.GetAt((int)oldsize), posstr->GetBuffer(), posstr->GetLength());
                }
                else
                {
                    if (argc > 1)
                    {
                        RaiseException(Exception::ERROR_index, "positional argument %u out of range [0-%u].", pos, argc - 1);
                    }
                    else
                    {
                        RaiseException(Exception::ERROR_index, "positional argument %u used but not specified.", pos);
                    }
                }
                cfmt--;
            }
            else
            {
                buff.Push(ch);
            }
        }
        
        return eng->GetString(buff.GetAt(0), buff.GetSize());
    }
    return fmt;
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
            RaiseException(Exception::ERROR_index, "Cannot split string of length: "PINT_FMT" at position: "PINT_FMT, (pint_t)len, at);
            
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
        String* searchStr = 0;
        if (argc == 1)
        {
            searchStr = ctx->GetStringArg(0);
        }
        else if (argc == 0)
        {
            searchStr = ctx->GetEngine()->GetString(WHITESPACE_CSTRING);
        }
        else
        {
            ctx->WrongArgCount();
        }
        Array* v = self.val.str->Split(searchStr);
        if (v) {
            ctx->Push(v);
            return 1;
        }
        return 0;
    }
    
    static int slice(Context* ctx, Value& self)
    {
        pint_t from, to;
        String* str  = self.val.str;
        String* res  = 0;
        
        if (ctx->IsArgNull(0)) {
            from = 0;
            to   = ctx->GetIntArg(1);
        } else if (ctx->IsArgNull(1)) {
            from = ctx->GetIntArg(0);
            to   = str->GetLength();
        } else {
            from = ctx->GetIntArg(0);
            to   = ctx->GetIntArg(1);
        }
        
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
    
    static int getLength(Context* ctx, Value& self)
    {
        String* src = self.val.str;
        size_t len = src->GetLength();
        ctx->Push((pint_t)len);
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

#define STARTING_POS() \
        if (ctx->GetArgCount() == 2)\
        {\
            start = ctx->GetIntArg(1);\
            if (start < 0 || ((size_t)start) >= str->GetLength())\
            {\
                return 0;\
            }\
        }

    static int firstOf(Context* ctx, Value& self)
    {
        String* setStr = ctx->GetStringArg(0);
        pint_t start = 0;
        String* str = self.val.str;
        
        STARTING_POS()
                
        size_t pos = strcspn(str->GetBuffer() + start, setStr->GetBuffer());
        pos += start;
        if (pos >= str->GetLength())
            return 0;
        ctx->Push((pint_t)pos);
        return 1;
    }
    
    static int firstNotOf(Context* ctx, Value& self)
    {
        String* setStr = ctx->GetStringArg(0);
        pint_t start = 0;
        String* str = self.val.str;
        
        STARTING_POS()
        
        size_t pos = strspn(str->GetBuffer() + start, setStr->GetBuffer());
        pos += start;
        if (pos >= str->GetLength())
            return 0;
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
        ctx->PushNull();
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
        ctx->PushNull();
        return 1;
    }
    
    static int charAt(Context* ctx, Value& self)
    {
        String* str = self.val.str;
        pint_t idx = ctx->GetIntArg(0);
        Engine* eng = ctx->GetEngine();
        
        if (idx >= 0 && ((size_t)idx < str->length))
        {
            ctx->Push(eng->AllocString(str->buffer + idx, 1));
        }
        else
        {
            RaiseException(Exception::ERROR_index, "Attempt to get character at index "PINT_FMT". Index must be between 0 and "SIZE_T_FMT".", idx, str->GetBuffer());
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
            RaiseException("String.concatSpace: insufficient memory to create the resulting string.");
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
                    ctx->GetEngine()->AllocString(WHITESPACE_CSTRING); // space,cr,nl,tab,vtab,formfeed
        
        String* res = vstr->Chomp(other);
        if (res)
            ctx->Push(res);
        else
            ctx->Push(ctx->GetEngine()->emptyString);
        return 1;
    }    
    
    static int escape(Context* ctx, Value& self)
    {
        String* vstr = self.val.str;
        Array* reps = ctx->GetArgT<Array>(1);
        String* res = 0;
        Value& vent = ctx->GetArg(0);
        
        if (vent.IsString()) {
            String* ents = vent.val.str;
            res = vstr->Escape(ents, reps);
        } else {
            Array* aents = ctx->GetArgT<Array>(0);
            res = vstr->Escape(aents, reps);
        }       
        
        ctx->Push(res);
        return 1;
    }
    
    static int strip(Context* ctx, Value& self)
    {
        u2 argc = ctx->GetArgCount();
        String* src = self.val.str;
        String* res = 0;
        const char* what = WHITESPACE_CSTRING;
        if (argc == 1) {
            String* arg = ctx->GetStringArg(0);
            what = arg->GetBuffer();
        } else if (argc != 0) {
            ctx->WrongArgCount();
        }
        res = src->Strip(SD_both, what);
        ctx->Push(res);
        return 1;
    }
    
    static int stripLeft(Context* ctx, Value& self)
    {
        u2 argc = ctx->GetArgCount();
        String* src = self.val.str;
        String* res = 0;
        const char* what = WHITESPACE_CSTRING;
        if (argc == 1) {
            String* arg = ctx->GetStringArg(0);
            what = arg->GetBuffer();
        } else if (argc != 0) {
            ctx->WrongArgCount();
        }
        res = src->Strip(SD_left, what);
        ctx->Push(res);
        return 1;
    }
    
    static int stripRight(Context* ctx, Value& self)
    {
        u2 argc = ctx->GetArgCount();
        String* src = self.val.str;
        String* res = 0;
        const char* what = WHITESPACE_CSTRING;
        if (argc == 1) {
            String* arg = ctx->GetStringArg(0);
            what = arg->GetBuffer();
        } else if (argc != 0) {
            ctx->WrongArgCount();
        }
        res = src->Strip(SD_right, what);
        ctx->Push(res);
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
                RaiseException("String.times: string too large to repeat "PINT_FMT" times.", rep);
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
    
    static int iterate(Context* ctx, Value& self)
    {
        String* obj = self.val.str;
        String* iter_type = 0;
        u2 argc = ctx->GetArgCount();
        
        if (argc == 1)
        {
            iter_type = ctx->GetStringArg(0);
        }
        else if (argc == 0)
        {
            iter_type = ctx->GetEngine()->emptyString;
        }
        else
        {
            ctx->WrongArgCount();
        }
        
        Iterator* e = obj->Iterate(iter_type);
        
        if (e)
        {
            ctx->Push(e);
            return 1;
        }
        return 0;
    }
        
    static int join(Context* ctx, Value& self)
    {
        Engine* eng = ctx->GetEngine();
        String* src_str = self.val.str;
        size_t src_len = src_str->GetLength();
        Value arg = ctx->GetArg(0);
        Value result(NULL_VALUE);
        Iterator* iterator = 0;
        
        if (eng->Iterator_Type->IsInstance(arg))
        {
            iterator = (Iterator*)arg.val.object;
        }
        else if (!(iterator = GetIteratorFrom(ctx, arg, 0)))
        {
            RaiseException("Cannot join string '%s' with non iterable object %s.", src_str->GetBuffer(), eng->SafeToString(ctx, arg));
        }
        
        IterateHelper iter(iterator, ctx);        
        
        if (!iter)
        {
            ctx->Push(eng->emptyString);
            return 1;
        }
        else
        {
            ctx->Push(iterator);
        }
        
        Buffer<char> buff;
        buff.SetCapacity(32);
        
        for (;;)
        {
            Value elem = iter.Next();            
            if (!iter)
                break;
            String* elem_str = eng->ToString(ctx, elem);
            size_t pos = buff.GetSize();
            size_t elem_len = elem_str->GetLength();
            
            if (SizeAdditionOverflow(pos, elem_len) ||
                SizeAdditionOverflow(pos + elem_len, src_len))
            {
                RaiseException("Attempt to create a String too large in String.join. The max string size is "SIZE_T_FMT".", (size_t)PIKA_STRING_MAX_LEN);
            }
            
            if (pos)
            {
                buff.Resize(pos + elem_len + src_len);
                Pika_memcpy(buff.GetAt(pos), src_str->GetBuffer(), src_len);
                pos += src_len;
            }
            else
            {
                buff.Resize(pos + elem_len);

            }
            Pika_memcpy(buff.GetAt(pos), elem_str->GetBuffer(), elem_len);
        } 
        if (buff.GetSize() >= PIKA_STRING_MAX_LEN)
            RaiseException("Attempt to create a String too large in String.join. The max string size is "SIZE_T_FMT".", (size_t)PIKA_STRING_MAX_LEN);
            
        String* strresult = eng->GetString(buff.GetAt(0), buff.GetSize());
        ctx->Top().Set(strresult); // Overwrite the iterator
        return 1;
    }
    
    static int asBytes(Context* ctx, Value& self)
    {
        const char* digits = "0123456789abcdef";
        Engine* engine = ctx->GetEngine();
        String* src_str = self.val.str;
        u2 const argc = ctx->GetArgCount();        
        size_t const src_size = src_str->GetLength();
        const char* orig_buff = src_str->GetBuffer();
        pint_t start_pos = 0;
        pint_t end_pos = src_size;
        
        switch(argc) {
        case 2: {
            end_pos = ctx->GetIntArg(1);
        }
        case 1: {
            start_pos = ctx->GetIntArg(0);
        }        
        break;        
        default:
            if (argc != 0) {
                ctx->WrongArgCount();
            }
        }
        
        if (end_pos < 0 || start_pos < 0 || end_pos > src_size || start_pos > src_size || start_pos > end_pos) {
            RaiseException(Exception::ERROR_type, "Invalid range of %d to %d passed to String.asBytes", start_pos, end_pos);
        }
        
        if (end_pos == start_pos) {
            ctx->Push(engine->emptyString);
            return 1;
        }
        
        size_t total_size = (end_pos - start_pos) * 4;
        engine->string_buff.Resize(total_size);
        size_t pos = 0;
        for (size_t i = start_pos; i < end_pos; ++i)
        {
            u1 byte = orig_buff[i];
            size_t b1 = byte % 16;
            size_t b2 = (byte / 16);
            if (b2)
            {
                b2 = b2 % 16;
            }            
            engine->string_buff[pos++] = '\\';
            engine->string_buff[pos++] = 'x';
            engine->string_buff[pos++] = digits[b2];
            engine->string_buff[pos++] = digits[b1];
        }
        String* str = engine->GetString(engine->string_buff.GetAt(0), engine->string_buff.GetSize());
        ctx->Push(str);
        return 1;
    }
    
    static int search(Context* ctx, Value& self)
    {
        Engine* eng = ctx->GetEngine();
        String* src_str = self.val.str;
        String* arg = ctx->GetStringArg(0);
        
        const char* res = strstr(src_str->GetBuffer(), arg->GetBuffer());
        if (!res) {
            ctx->PushNull();
        } else {
            pint_t diff = (pint_t)(res - src_str->GetBuffer());
            ctx->Push(diff);
        }
        return 1;
    }
};

PIKA_DOC(String_chomp, 
"/([set])\n"
"Drops characters as long as they appear in the string |set|."
"It stops when the zero indexed character is no longer in |set| or the string's length is 0."
"If |set| is not specified them all whitespace characters will be removed from"
"the front of the string."
)

PIKA_DOC(String_times, 
"/(rep)\n"
"Returns the string repeated |rep| times."
)

PIKA_DOC(String_reverse, 
"/()\n"
"Returns the entire string reversed."
)

PIKA_DOC(String_toString, 
"/()\n"
"Returns a reference to this string."
)

PIKA_DOC(String_is_letter, 
"/()\n"
"Returns true if and only if each character in the string is a letter. "
"If the string is empty false is returned"
)

PIKA_DOC(String_is_letterOrDigit, 
"/()\n"
"Returns true if and only if each character in the string is a letter or a digit. "
"If the string is empty false is returned"
)

PIKA_DOC(String_is_digit, 
"/()\n"
"Returns true if and only if each character in the string is a digit. "
"If the string is empty false is returned"
)

PIKA_DOC(String_is_ascii, 
"/()\n"
"Returns true if and only if each character in the string is a valid ascii character. "
"If the string is empty false is returned"
)

PIKA_DOC(String_is_whitespace, 
"/()\n"
"Returns true if and only if each character in the string is a whitespace character, including newlines. "
"If the string is empty false is returned"
)

PIKA_DOC(String_toInteger, "/()\n"
"Returns the [Integer] represented by this string. The integer must be in the same format"
" as an integer literal in Pika. The integer can be specified in any base from 2 to 36."
" If the conversion cannot take place an exception will be raised."
)

PIKA_DOC(String_toReal, "/()\n"
"Returns the [Real] represented by this string. The real must be in the same format"
" as an real literal in Pika. The real can be specified in any base from 2 to 36 or contain an exponent."
" If the conversion cannot take place an exception will be raised."
)

PIKA_DOC(String_toNumber, "/()\n"
"Returns the [Real] or [Integer] represented by this string. See [toInteger] and [toReal] for more information on valid formats."
" If the conversion cannot take place an exception will be raised."
)

PIKA_DOC(String_toLower, "/()\n"
"Converts and returns a copy of this string, with each letter converted to lower-case. Non-letter characters are copied as they appear in the string."
)

PIKA_DOC(String_toUpper, "/()\n"
"Converts and returns a copy of this string, with each letter converted to upper-case. Non-letter characters are copied as they appear in the string."
)

PIKA_DOC(String_join, "/(obj)\
\n\
Joins the elements of |obj|, converted to a string, with this string inserted between elements. \
The object, |obj|, must implement the '''iterate''' function or be an instance of an [Iterator] derived class.\
[[[\
print ';'.join( [ 1, 2, 3 ] ) #=> '1;2;3'\
]]]\
"
)

PIKA_DOC(String_splitAt, "/(p)\
\n\
Split the string into two substrings at position |p|.")

PIKA_DOC(String_split, "/(set)\
\n\
Split the string where ever an character from |set| is present. The resultant strings will be returned in an [Array].\
[[[\
s = ',;'\n\
print( 'dog,cat;wolf,lion'.split(s) ) #=> ['dog', 'cat', 'wolf', 'lion']\
]]]\
")

PIKA_DOC(String_charAt, "/(pos)\
\n\
Returns the character at the position, |pos|, given. The character will be returned as a one character string.")

PIKA_DOC(String_byteAt, "/(pos)\
\n\
Returns the byte at the position, |pos|, given. The byte will be returned as an [Integer integer].")

PIKA_DOC(String_firstOf, "/(set)\
\n\
Returns the index of the first occurrence of any character in this string that is in the string |set|.\
[[[\
print( '123abc'.firstOf('abcd') ) #=> 3\
]]]\
")

PIKA_DOC(String_firstNotOf, "/(set)\
\n\
Returns the index of the first occurrence of any character in this string that is not in the string |set|.\
[[[\
print( '123abc'.firstNotOf('12') ) #=> 2\
]]]\
")

PIKA_DOC(String_lastOf, "/(set)\
\n\
Returns index of the last occurrence of any character in this string that is in the string |set|.\
[[[\
print( '123abc'.lastOf('123') ) #=> 2\
]]]\
")

PIKA_DOC(String_lastNotOf, "/(set)\
\n\
Returns index of the last occurrence of any character in this string that is not in the string |set|.\
[[[\
print( '123abc'.lastNotOf('abc') ) #=> 2\
]]]\
")

PIKA_DOC(String_substring, "/(start, stop)\
\n\
Returns a sub-string of this string from the range &#91;|start| - |stop|&#93;. If |stop| \
is less than |start| the returned string will be reversed.")

PIKA_DOC(String_cat, "/(left, right)\
\n\
Concatenates the string |left| to the string |right|.")

PIKA_DOC(String_catSp, "/(left, right)\
\n\
Concatenates the string |left| to the string |right|, inserting a space character in between them.")

PIKA_DOC(String_fromByte, "/(byte)\
\n\
Converts the character |byte| into a string.")

PIKA_DOC(String_new, "/(val)\
\n\
Creates a string from |val|, calling the conversion function '''toString''' as needed.")

PIKA_DOC(String_replaceChar, "/(x, r)\
\n\
Replaces all instances of |x| with |r|. Both |x| and |r| should be strings of length one.")

void String::StaticInitType(Engine* eng)
{
    PIKA_DOC(String_Type, "The base string type used in Pika. Strings are immutable, meaning they cannot be modified. \
    They can contains any valid byte. This means they can contain arbitrary binary data, including null characters. They \
    can also be used for utf-8 encoded characters.\
    \n\n\
    Strings are created for each single or double quote string literal appearing in a script. Since they are immutable two strings containing the same\
    sequence of characters will reference the same object.")
    
    static RegisterFunction String_Methods[] =
    {
        // name, function, argc, strict, varargs
        { "replaceChar",	StringApi::replaceChar,         2, DEF_STRICT,   PIKA_GET_DOC(String_replaceChar) },
        { "toInteger",  	StringApi::toInteger,           0, 0,            PIKA_GET_DOC(String_toInteger) },
        { "toReal",     	StringApi::toReal,              0, 0,            PIKA_GET_DOC(String_toReal) },
        { "toNumber",       StringApi::toNumber,            0, 0,            PIKA_GET_DOC(String_toNumber) },
        { "toLower",        StringApi::toLower,             0, 0,            PIKA_GET_DOC(String_toLower) },
        { "toUpper",        StringApi::toUpper,             0, 0,            PIKA_GET_DOC(String_toUpper) },
        { "charAt",         StringApi::charAt,              1, DEF_STRICT,   PIKA_GET_DOC(String_charAt) },
        { "split",          StringApi::split,               0, DEF_VAR_ARGS, PIKA_GET_DOC(String_split) },
        { "splitAt",        StringApi::splitAt,             1, 0,            PIKA_GET_DOC(String_splitAt) },
        { "byteAt",     	StringApi::byteAt,              1, 0,            PIKA_GET_DOC(String_byteAt) },
        { "firstOf",        StringApi::firstOf,             1, DEF_VAR_ARGS, PIKA_GET_DOC(String_firstOf) },
        { "firstNotOf",     StringApi::firstNotOf,          1, DEF_VAR_ARGS, PIKA_GET_DOC(String_firstNotOf) },
        { "lastOf",         StringApi::lastOf,              2, DEF_VAR_ARGS, PIKA_GET_DOC(String_lastOf) },
        { "lastNotOf",      StringApi::lastNotOf,           2, DEF_VAR_ARGS, PIKA_GET_DOC(String_lastNotOf) },
        { "substring",      StringApi::slice,               2, DEF_STRICT,   PIKA_GET_DOC(String_substring) },
        { "chomp",          StringApi::chomp,               1, DEF_VAR_ARGS, PIKA_GET_DOC(String_chomp) },
        { "times",          StringApi::times,               1, DEF_STRICT,   PIKA_GET_DOC(String_times)   },
        { "opMul",          StringApi::times,               1, DEF_STRICT,   PIKA_GET_DOC(String_times)   },
        { "toString",       StringApi::toString,            0, DEF_STRICT,   PIKA_GET_DOC(String_toString) },
        { "reverse",        StringApi::reverse,             0, DEF_STRICT,   PIKA_GET_DOC(String_reverse) },
        { "letter?",        StringApi::is_letter,           0, DEF_STRICT,   PIKA_GET_DOC(String_is_letter) },
        { "letterOrDigit?", StringApi::is_letterOrDigit,    0, DEF_STRICT,   PIKA_GET_DOC(String_is_letterOrDigit) },
        { "digit?",         StringApi::is_digit,            0, DEF_STRICT,   PIKA_GET_DOC(String_is_digit) },
        { "ascii?",         StringApi::is_ascii,            0, DEF_STRICT,   PIKA_GET_DOC(String_is_ascii) },
        { "whitespace?",    StringApi::is_whitespace,       0, DEF_STRICT,   PIKA_GET_DOC(String_is_whitespace) },
        { "iterate",        StringApi::iterate,             0, DEF_VAR_ARGS, PIKA_GET_DOC(String_iterate) },
        { "join",           StringApi::join,                0, DEF_VAR_ARGS, PIKA_GET_DOC(String_join) },
        { "search",         StringApi::search,              0, DEF_VAR_ARGS, 0 },
        { "getLength",      StringApi::getLength,           0, DEF_STRICT,   0 },

        { "strip",          StringApi::strip,               0, DEF_VAR_ARGS, 0 },
        { "stripLeft",      StringApi::stripLeft,           0, DEF_VAR_ARGS, 0 },
        { "stripRight",     StringApi::stripRight,          0, DEF_VAR_ARGS, 0 },
        { "asBytes",        StringApi::asBytes,             0, DEF_VAR_ARGS, 0 },
        { "escape",         StringApi::escape,              2, DEF_STRICT,   0 },
    };
    
    static RegisterFunction String_ClassMethods[] =
    {
        { "cat",        StringApi::concat,      0, DEF_VAR_ARGS, PIKA_GET_DOC(String_cat) },
        { "catSp",      StringApi::concatSpace, 0, DEF_VAR_ARGS, PIKA_GET_DOC(String_catSp) },
        { "fromByte",   StringApi::fromByte,    1, DEF_STRICT,   PIKA_GET_DOC(String_fromByte) },
        { NEW_CSTR,     StringApi::init,        1, DEF_STRICT,   PIKA_GET_DOC(String_new) },
    };
    eng->String_Type = Type::Create(eng, eng->AllocString("String"), eng->Basic_Type, StringApi::Constructor, eng->GetWorld());
        
    eng->String_Type->SetFinal(true);
    eng->String_Type->SetAbstract(true);    
    eng->String_Type->EnterMethods(String_Methods, countof(String_Methods));
    eng->String_Type->EnterClassMethods(String_ClassMethods, countof(String_ClassMethods));
    eng->String_Type->SetDoc(eng->GetString(PIKA_GET_DOC(String_Type)));
    Value string_MAX((pint_t)PIKA_STRING_MAX_LEN);
    eng->String_Type->SetSlot("MAX", string_MAX);
    eng->GetWorld()->SetSlot("String", eng->String_Type);
    
    SlotBinder<String>(eng, eng->String_Type)
    .Alias(OPSLICE_STR, "substring");
}

}// pika
