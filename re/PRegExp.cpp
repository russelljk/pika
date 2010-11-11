/*
 *  PRegExp.cpp
 *  See Copyright Notice in PRegExp.h
 */
#include "Pika.h"
#include "PRegExp.h"
#include "PlatRE.h"
#if defined(PIKA_WIN)

#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH: break;
    }
    return TRUE;
}

#endif

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
    size_t longest    = 0;
    size_t longestLen = 0;
    bool   haslongest = false;
    
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
                case 'g':
                    is_global = true;
                    break;
                case 'i':
                    is_insensitive = true;
                    break;
                case 'm':
                    is_multiline = true;
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
            String* matchstr = engine->AllocStringNC(subj->GetBuffer() + start, len);
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
        obj->SetSlot(engine->AllocStringNC("start"), Value((pint_t)start));
        obj->SetSlot(engine->AllocStringNC("stop"),  Value((pint_t)stop));
        
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
        obj->SetSlot(engine->AllocStringNC("match"), res);
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
        size_t subjLen = subj->GetLength(); // Total length of the string.
        
        if (subjLen < last_index)
            return 0;
                
        if (!DoExec(subj->GetBuffer() + last_index, subjLen - last_index, matches))
            return 0;
        
        Array* res = Array::Create(engine, engine->Array_Type, 0, 0); // Resultant Array object.
        size_t next_last = last_index;
        for (size_t i = 0; i < (matches.GetSize() / 2); ++i)
        {
            size_t const start = matches[i*2] + last_index;
            size_t const end   = matches[i*2 + 1] + last_index;
            if (end > next_last)
                next_last = end;
            Value match = GetMatch(start, end, subj);
            res->Push(match);
        }
        if (IsGlobal())
            last_index = next_last;
        return res;
    }
    
    bool Test(String* subj)
    {
        if (!this->pattern)
            return false;
        size_t subjLen = subj->GetLength();
        if (subjLen > last_index)
            return false;
        
        Buffer<size_t> matches;
        if (!DoExec(subj->GetBuffer() + last_index, subjLen - last_index, matches))
            return false;
        
        if (matches.GetSize())
        {
            if (IsGlobal()) {
                for (size_t i = 0; i < (matches.GetSize() / 2); ++i)
                {
                    size_t const end   = matches[i*2 + 1];
                    if (end > last_index)
                        last_index = end;
                }
            }            
            return true;
        }
        return false;
    }
    
    bool IsGlobal() const { return is_global; }
    
    bool IsMultiline()   const { return is_multiline; }
    bool IsInsensitive() const { return is_insensitive; }
    bool IsUtf8()        const { return is_utf8; }
    String* Pattern() { return pattern_string; }
    
    size_t GetLastIndex() { return last_index; }
    
    void SetLastIndex(pint_t idx)
    {
        if (idx < 0)
            RaiseException("lastIndex cannot be negative");
        last_index = idx;
    }
        
    bool DoExec(const char* subj, size_t subjLen, Buffer<size_t>& res)
    {
        Pika_regmatch ovector[NUM_MATCHES];
        int matchCount = Pika_regexec(this->pattern, subj, subjLen, ovector, NUM_MATCHES, RE_NO_UTF8_CHECK);
        if (matchCount <= 0)
            return false;
        res.Resize((size_t)matchCount * 2);
        
        for (int i = 0; i < matchCount; ++i)
        {
            size_t start = IntToIndex(ovector[i].start, subjLen);
            size_t end   = IntToIndex(ovector[i].end,   subjLen);
            if (end < start)
                Swap(start, end);
            res[i*2]     = start;
            res[i*2 + 1] = end;
        }
        return true;
    }
    
    virtual void MarkRefs(Collector* c)
    {
        ThisSuper::MarkRefs(c);
        if (pattern_string)
            pattern_string->Mark(c);
    }
    
    static void Constructor(Engine* eng, Type* obj_type, Value& res)
    {
        RegExp* re = RegExp::StaticNew(eng, obj_type, 0);
        res.Set(re);
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
}

}// pika

using pika::RegExp;

PIKA_MODULE(RegExp, eng, re)
{
    pika::GCPAUSE(eng);
    //////////////////
    pika::Package* Pkg_World = eng->GetWorld();
    pika::String* RegExp_String = eng->AllocString("RegExp");
    pika::Type* RegExp_Type = pika::Type::Create(eng, RegExp_String, eng->Object_Type, RegExp::Constructor, Pkg_World);
    
    pika::SlotBinder<pika::RegExp>(eng, RegExp_Type)
    .Register(pika::RegExp_exec, "exec", 1, true, false)
    .Method(&RegExp::Test,     "test")
    .Method(&RegExp::Compile,  "compile")
    .MethodVA(&RegExp::Init,   "init")
    .PropertyRW("lastIndex",    &RegExp::GetLastIndex, "getLastIndex", &RegExp::SetLastIndex, "setLastIndex")
    .PropertyR("global?",       &RegExp::IsGlobal,      0)
    .PropertyR("multiline?",    &RegExp::IsMultiline,   0)
    .PropertyR("utf8?",         &RegExp::IsUtf8,        0)
    .PropertyR("insensitive?",  &RegExp::IsInsensitive, 0)
    .PropertyR("pattern",       &RegExp::Pattern,       0)
    ;
    
    //RegExp_Type->EnterMethods(RegExpFunctions, countof(RegExpFunctions));
    re->SetSlot(RegExp_String, RegExp_Type);
    return RegExp_Type;
}

