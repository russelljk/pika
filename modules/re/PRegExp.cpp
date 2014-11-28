/*
 *  PRegExp.cpp
 *  See Copyright Notice in PRegExp.h
 */
#include "Pika.h"
#include "PRegExp.h"
#include "PlatRE.h"


#define NUM_MATCHES  128
#define ERR_BUF_SZ   1024

namespace pika {

namespace {

// Converts an integer index (possibly bogus) into the range [0-len] ... inclusive.
INLINE size_t IntToIndex(int const i, size_t const len)
{
    size_t ri = 0;
    if (i > 0)
        ri = (size_t)i;
    return ri > len ? len : ri;
}

INLINE static size_t IndexOfLongestMatch(const Buffer<size_t>& matches)
{
    size_t longest = 0;
    size_t longestLen = 0;
    bool haslongest = false;
    
    size_t const buffsize = matches.GetSize();
    
    for (size_t c = 0; c < buffsize; c+= 2)
    {
        size_t clen = matches[c] - matches[c+1];
        if (!haslongest || clen > longestLen)
        {
            haslongest = true;
            longest = c;
            longestLen = clen;
        }
    }
    return longest;
}

}// anonymous namspace

class RegExp : public Object
{
public:
    PIKA_DECL(RegExp, Object)
    
    RegExp(Engine* engine, Type* type) : Object(engine, type),
        is_multiline(false),
        is_insensitive(false),
        is_global(false),
        is_utf8(true),
        last_index(0),
        pattern(0),
        pattern_string(0)
    {}
    
    RegExp(const RegExp* rhs) : 
        ThisSuper(rhs),
        pattern(rhs->pattern)
    {
    }
    
    virtual ~RegExp()
    {
        FreeAll();
    }
    
    void FreeAll()
    {
        if (pattern)
        {
            Pika_regfree(this->pattern);
            this->pattern = 0;
        }
    }
    
    virtual Object* Clone()
    {
        RegExp* r = 0;
        GCNEW(engine, RegExp, r, (this));
        return r;
    }
    
    virtual void Init(Context* ctx)
    {
        u4 argc = ctx->GetArgCount();
        String* re = ctx->GetStringArg(0);
        
        if (argc == 2) {
            String* opts = ctx->GetStringArg(1);
            const char* buff = opts->GetBuffer();
            for (size_t i = 0; i < opts->GetLength(); ++i)
            {
                switch (buff[i]) {
                case 'a':
                    is_utf8 = false;
                case 'g':
                    is_global = true;
                    break;
                case 'i':
                    is_insensitive = true;
                    break;
                case 'm':
                    is_multiline = true;
                    break;
                    break;
                default:
                    RaiseException("Unknown RegExp.init option encountered in '%s' only combinations of 'g', 'i' and 'm' are allowed.", buff);
                }
            }
        }
        else if (argc != 1) {
            ctx->WrongArgCount();
        }

        Compile(re);
    }
    
    static RegExp* StaticNew(Engine* eng, Type* type, String* pattern)
    {
        RegExp* re;
        GCNEW(eng, RegExp, re, (eng, type));
        if (pattern)
        {
            re->Compile(pattern);
        }
        return re;
    }
    
    void Compile(String* re)
    {
        if (re->HasNulls())
        {
            RaiseException(Exception::ERROR_runtime, "Attempt to compile a regular expression containing one or more NUL characters.");
        }
        WriteBarrier(re);
        pattern_string = re;
        
        int options = RE_NO_UTF8_CHECK;
        
        if (is_utf8)
            options |= RE_UTF8;

        if (is_multiline)
            options |= RE_MULTILINE;
        
        if (is_insensitive)
            options |= RE_INSENSITIVE;
            
        int errcode = 0;

        if (this->pattern)
            FreeAll();
        
        char errdescr[ERR_BUF_SZ];
        this->pattern = Pika_regcomp(pattern_string->GetBuffer(), options, errdescr, ERR_BUF_SZ, &errcode);
        if (!this->pattern)
        {
            RaiseException("Attempt to compile regular expression from pattern %s failed: %s.", pattern_string->GetBuffer(), errdescr);
        }
    }
    
    /** Returns the substring subj[start to end]. */
    Value GetMatch(size_t start, size_t end, String* subj)
    {
        Value res(NULL_VALUE);
        size_t len = end - start;
        // If its an empty match.
        if (len == 0)
        {
            res.Set(engine->emptyString);            
        }
        else
        {
            // Create a nonempty match string.
            String* matchstr = engine->GetString(subj->GetBuffer() + start, len);
            res.Set(matchstr);
        }
        return res;
    }

    /** Returns an Object containing the substring of subj and indices start,stop. */    
    Value GetMatchObject(size_t start, size_t stop, String* subj)
    {
        GCPAUSE_NORUN(engine);
        Object* obj = Object::StaticNew(engine, engine->Object_Type);
        Value res(NULL_VALUE);
        size_t len = stop - start;
        // If its an empty match.
        obj->SetSlot(engine->GetString("start"), Value((pint_t)start));
        obj->SetSlot(engine->GetString("stop"),  Value((pint_t)stop));
        
        if (len == 0)
        {
            res.Set(engine->emptyString);
        }
        else
        {
            // Create a nonempty match string.
            String* matchstr = engine->AllocString(subj->GetBuffer() + start, len);
            res.Set(matchstr);
        }
        obj->SetSlot(engine->GetString("match"), res);
        res.Set(obj);
        return res;
    }
    
    virtual String* ToString()
    {
        String* res = String::ConcatSep(this->type->GetName(), this->pattern_string ? this->pattern_string : engine->emptyString, ':');
        return res;
    }
    
    Array* Exec(String* subj, bool objs = false)
    {
        if (!this->pattern)
            return 0;
        
        GCPAUSE_NORUN(engine);
        Buffer<size_t> matches; // Buffer to store matches.
        
        if (!DoExec(subj, matches))
            return 0;
        
        Array* res = Array::Create(engine, engine->Array_Type, 0, 0); // Resultant Array object.
        
        for (size_t i = 0; i < matches.GetSize(); i += 2)
        {
            size_t const start = matches[i];
            size_t const end   = matches[i + 1];
            
            Value match = objs ? GetMatchObject(start, end, subj) : GetMatch(start, end, subj);
            res->Push(match);
        }
        
        return res;
    }
    
    bool Exec2(String* subj, Buffer<size_t>& matches)
    {
        if (!this->pattern)
            return false;
                
        if (!DoExec(subj, matches))
            return false;
        
        return true;
    }

    bool Test(String* subj)
    {
        if (!this->pattern)
            return false;
                
        Buffer<size_t> matches;
        
        if (!DoExec(subj, matches))
            return false;
        
        if (matches.GetSize())
        {
            return true;
        }
        return false;
    }
    
    void Reset()
    {
        this->last_index = 0;
    }
    
    bool    IsGlobal()      const { return is_global != 0; }    
    bool    IsMultiline()   const { return is_multiline != 0; }
    bool    IsInsensitive() const { return is_insensitive != 0; }
    bool    IsUtf8()        const { return is_utf8 != 0; }
    String* Pattern()             { return pattern_string; }    
    size_t  GetLastIndex()        { return last_index; }
    
    void SetLastIndex(pint_t idx)
    {
        if (idx < 0)
            RaiseException(Exception::ERROR_index, "Property 'lastIndex' cannot be negative.");
        last_index = idx;
    }
    
    void MoveIndex() { last_index++; }
    
    virtual void MarkRefs(Collector* c)
    {
        ThisSuper::MarkRefs(c);
        if (pattern_string)
            pattern_string->Mark(c);
    }
    
    static void Constructor(Engine* eng, Type* obj_type, Value& res)
    {
        Object* re = RegExp::StaticNew(eng, obj_type, 0);
        res.Set(re);
    }
    
protected:        
    bool DoExec(String* s, Buffer<size_t>& res)
    {
        Pika_regmatch ovector[NUM_MATCHES];
        const char* subj = s->GetBuffer();
        size_t subjLen = s->GetLength();
        
        if (subjLen < last_index)
            return false;
        
        int matchCount = Pika_regexec(this->pattern, subj + last_index, subjLen - last_index, ovector, NUM_MATCHES, RE_NO_UTF8_CHECK);
        if (matchCount <= 0)
            return false;
        res.Resize((size_t)matchCount * 2);

        size_t next_last = last_index;

        for (int i = 0; i < matchCount; ++i)
        {
            size_t start = IntToIndex(ovector[i].start, subjLen) + last_index;
            size_t end = IntToIndex(ovector[i].end,   subjLen) + last_index;
            if (end < start)
                Swap(start, end);
            if (end > next_last)
                next_last = end;
            res[i*2] = start;
            res[i*2 + 1] = end;
        }
        
        if (IsGlobal())
        {
            last_index = next_last;
        }
        return true;
    }
    
    u1 is_multiline;
    u1 is_insensitive;
    u1 is_global;
    u1 is_utf8;
    size_t last_index;
    Pika_regex* pattern;
    String* pattern_string;
};

PIKA_IMPL(RegExp)

namespace {
    int RegExp_exec(Context* ctx, Value& self)
    {
        RegExp* re = static_cast<RegExp*>(self.val.object);
        String* subj = ctx->GetStringArg(0);
        bool objs = false;
        if (ctx->GetArgCount() == 2)
        {
            objs = ctx->GetBoolArg(1);
        }
        else if (ctx->GetArgCount() != 1)
        {
            ctx->WrongArgCount();
        }
        Array* a = re->Exec(subj, objs);
        if (a)
        {
            ctx->Push(a);
        }
        else
        {
            ctx->PushNull();
        }
        return 1;
    }
    
    void BufferAppend(Buffer<char>& buff, const char* source, size_t count)
    {
        if (count)
        {
            size_t const last_size=buff.GetSize();
            if (SizeAdditionOverflow(last_size, count)) {
                RaiseException(Exception::ERROR_overflow, "String.matchReplace resultant string is too large. The max string size is "SIZE_T_FMT".", (size_t)PIKA_STRING_MAX_LEN);
            } else {
                buff.Resize(last_size + count);            
                Pika_memcpy(buff.GetAt(last_size), source, count);   
            }
        }
    }
    
    String* CallFormatFunction(Context* ctx, Value& func, u4 argc)
    {
        ctx->PushNull();
        ctx->Push(func);
        if (ctx->SetupCall(argc))
            ctx->Run();
        Value res = ctx->PopTop();
        if (res.IsString())
            return res.val.str;
        else        
            RaiseException("Formatting function for String.matchReplace must return a string.");
        return 0;
    }
    
    // replaces matched substrings in the self object.
    int String_matchReplace(Context* ctx, Value& self)
    {
        Engine* eng = ctx->GetEngine();
                
        String* source = self.val.str;
        RegExp* re = ctx->GetArgT<RegExp>(0);
        
        Value format_val = ctx->GetArg(1);        
        String* format = 0;
        if (format_val.IsString()) {
            format = format_val.val.str;
        }
               
        Buffer<char> buff;
        Buffer<size_t> matches;
        
        buff.SetCapacity(source->GetLength());
        
        // Tracks the part of the string between valid matches
        struct BufferBounds
        {
            size_t start;
            size_t stop;
        }   bounds;
        bounds.start = 0;
        bounds.stop = 0;
        
        String* pos_args[PIKA_MAX_POS_ARGS];
        Pika_memzero(pos_args, sizeof(pos_args));
        
        size_t last = re->GetLastIndex(); // track the last index so that we don't loop indefinetly.        
        while (re->Exec2(source, matches))
        {
            // Move ahead if the last search didn't.
            // This will happen when using * in certain situations.
            if (re->GetLastIndex() == last)
                re->MoveIndex();
            last = re->GetLastIndex();
            
            u2 pos_count = 0;
            BufferBounds match_bounds;
            match_bounds.start = PIKA_STRING_MAX_LEN;
            match_bounds.stop = 0;
                        
            for (size_t i = 0; (i < matches.GetSize()) && (pos_count < PIKA_MAX_POS_ARGS); i += 2)
            {
                size_t start = matches[ i ];
                size_t stop = matches[ i + 1 ];
                
                match_bounds.start = Min(match_bounds.start, start);
                match_bounds.stop = Max(match_bounds.stop, stop);
                // check for last index used bound.stop
                // create String for this match and set 
                String* match_str = (re->GetMatch(start, stop, source)).val.str;
                pos_args[ pos_count++ ] = match_str;
                
                // We push the string not only for the format function call but
                // to keep it safe from the GC.
                
                ctx->Push(match_str);
            }
            
            // sprintp Will replace all the match indexes in format with the matches.
            
            String* pos_string = (format) ? format : CallFormatFunction(ctx, format_val, pos_count);            
            String* replacement = String::sprintp(eng, pos_string, pos_count, pos_args);
            
            if (format)
                ctx->Pop(pos_count);
            
            // !!! From here to the end of the while loop there can be no new GC allocations.
            
            // Copy the un-matched characters from the source string.
            bounds.stop = match_bounds.start;
            if (bounds.stop > bounds.start && bounds.stop < source->GetLength())
            {
                BufferAppend(buff, source->GetBuffer() + bounds.start, bounds.stop - bounds.start);
            }
    
            // Now copy the replacement string.
            BufferAppend(buff, replacement->GetBuffer(), replacement->GetLength());            
            bounds.start = bounds.stop = match_bounds.stop; // set up the next loop.            
        }
        
        if (bounds.start < source->GetLength())
        {
            BufferAppend(buff, source->GetBuffer() + bounds.start, source->GetLength() - bounds.start);
        }
        if (buff.GetSize() >= PIKA_STRING_MAX_LEN)
            RaiseException("Attempt to create a String too large in String.matchReplace. The max string size is "SIZE_T_FMT".", (size_t)PIKA_STRING_MAX_LEN);
        String* resultant = eng->GetString(buff.GetAt(0), buff.GetSize());
        ctx->Push(resultant);
        if (eng->GetActiveContext() == ctx)
            eng->GetGC()->CheckIf();
        return 1;
    }
}

}// pika

using pika::RegExp;

PIKA_DOC(RegExp_init, "/(pattern, options)\
\n\
Initializes a new RegExp instance. \n\
The parameter |pattern| should be the regular \
expression pattern. Ensure that the string is properly escaped otherwise you \
may end up with a different pattern than intended. \n\
The parameter |options| is \
the string containing the options to compile the |pattern| with. Use 'g' for \
global, 'i' for case insensitive, 'm' for multiline, 'a' ascii no utf-8 support.\n\
[[[\
RegExp = import 'RegExp'\n\
re = RegExp.new('[a-zA-Z]+', 'gm')\
]]]\
")

PIKA_DOC(RegExp_is_global, "Is this regular expression global. This means each \
operation moves the [lastIndex], otherwise each operation will start at the \
beginning of the [String string].")

PIKA_DOC(RegExp_is_multiline, "Does this regular expression work across multiple \
lines, otherwise each operation will end at a newline.")

PIKA_DOC(RegExp_is_utf8, "Is this regular expression utf-8 aware. By default this will be true.")

PIKA_DOC(RegExp_is_insensitive, "Is this regular expression case insensitive.")

PIKA_DOC(RegExp_pattern, "The pattern for this regular expression.")

PIKA_DOC(RegExp_getLastIndex, "Returns the last index from the previous \
operation. If the regular expression is not [global?] then the last index will be ignored.")

PIKA_DOC(RegExp_setLastIndex, "/(index)\
\n\
Sets [lastIndex] to |index|. This will be the starting point in a \
[String string] for any future operations. If the regular expression is not \
[global?] then the last index will be ignored.")

PIKA_DOC(RegExp_lastIndex, "The last index from the previous operation. If the \
regular expression is not [global?] then the last index will be ignored.")

PIKA_DOC(RegExp_exec, "/(string, obj? = false)\
\n\
Returns an [Array array] of [String] matches for the |string| given. If |obj?| \
is true then a array of [Object objects] will be returned with the start, stop \
and match string members.")

PIKA_DOC(RegExp_test, "/(string)\
\n\
Returns whether or not |string| is matched. If the regular expression is \
[global? global] then [lastIndex] will be moved.")

PIKA_DOC(RegExp_compile, "/(pattern)\
Compile the |pattern| given. The options that this instance was initialized \
with will be used.")

PIKA_DOC(RegExp_reset, "For global regular expressions it resets the position of lastIndex.")

PIKA_DOC(String_matchReplace, "/(re, fmt)\
\n\
Replaces every match from the [imports.RegExp.RegExp regular expression], |re|, using the format \
[String string] or [Function function] |fmt|. \
If |fmt| is a string then each match will be placed by applying [sprintp] to the format string and match results. \
If |fmt| is a function then for each match |fmt| will be called with the current matches. The return value will then be used as the format string passed to [sprintp].\
\n\
This method is only available when the [Module] [imports.RegExp] is [import imported].")

PIKA_MODULE(re, eng, re)
{
    pika::GCPAUSE(eng);
    //////////////////
    pika::Package* Pkg_World = eng->GetWorld();
    pika::String* RegExp_String = eng->AllocString("RegExp");
    pika::Type* RegExp_Type = pika::Type::Create(eng, RegExp_String, eng->Object_Type, RegExp::Constructor, Pkg_World);
    
    pika::SlotBinder<pika::RegExp>(eng, RegExp_Type)
    .RegisterMethod(pika::RegExp_exec, "exec", 1, true, false, PIKA_GET_DOC(RegExp_exec))
    .Method(&RegExp::Test,     "test", PIKA_GET_DOC(RegExp_test))
    .Method(&RegExp::Compile,  "compile", PIKA_GET_DOC(RegExp_compile))
    .Method(&RegExp::Reset,    "reset",   PIKA_GET_DOC(RegExp_reset))
    .MethodVA(&RegExp::Init,   "init",    PIKA_GET_DOC(RegExp_init))
    .PropertyRW("lastIndex",    &RegExp::GetLastIndex, "getLastIndex", 
                                &RegExp::SetLastIndex, "setLastIndex", 
                                                           PIKA_GET_DOC(RegExp_getLastIndex), 
                                                           PIKA_GET_DOC(RegExp_setLastIndex), 
                                                           PIKA_GET_DOC(RegExp_lastIndex))
    .PropertyR("global?",       &RegExp::IsGlobal,      0, 0, PIKA_GET_DOC(RegExp_is_global))
    .PropertyR("multiline?",    &RegExp::IsMultiline,   0, 0, PIKA_GET_DOC(RegExp_is_multiline))
    .PropertyR("utf8?",         &RegExp::IsUtf8,        0, 0, PIKA_GET_DOC(RegExp_is_utf8))
    .PropertyR("insensitive?",  &RegExp::IsInsensitive, 0, 0, PIKA_GET_DOC(RegExp_is_insensitive))
    .PropertyR("pattern",       &RegExp::Pattern,       0, 0, PIKA_GET_DOC(RegExp_pattern))
    ;
    
    static pika::RegisterFunction String_Methods[] = {
        { "matchReplace", pika::String_matchReplace, 2, pika::DEF_STRICT, PIKA_GET_DOC(String_matchReplace) }
    };
    
    eng->String_Type->EnterMethods(String_Methods, countof(String_Methods));
    re->SetSlot(RegExp_String, RegExp_Type);
    return re;
}

