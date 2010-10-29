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
    
    RegExp(Engine* engine, Type* type) : Object(engine, type), pattern(0) {}
    
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
        if (ctx->GetArgCount() != 1)
            RaiseException("RegExp.init takes exactly one String arguments.");
        String* re = ctx->GetStringArg(0);
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
        int options = PIKA_UTF8 | PIKA_NO_UTF8_CHECK | PIKA_MULTILINE;
        int errcode = 0;

        if (this->pattern)
            FreeAll();
        char errdescr[ERR_BUF_SZ];
        this->pattern = Pika_regcomp(re->GetBuffer(), options, errdescr, ERR_BUF_SZ, &errcode);
        if (!this->pattern)
        {
            RaiseException("Attempt to compile regular expression from pattern %s failed: %s.", re->GetBuffer(), errdescr);
        }
    }
    
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
            String* matchstr = engine->AllocString(subj->GetBuffer() + start, len);
            res.Set(matchstr);
        }
        return res;
    }
    
    Value GetMatchObject(size_t start, size_t end, String* subj)
    {
        GCPAUSE_NORUN(engine);
        Object* obj = Object::StaticNew(engine, engine->Object_Type);
        Value res(NULL_VALUE);
        size_t len = end - start;
        // If its an empty match.
        obj->SetSlot(engine->AllocStringNC("start"), Value((pint_t)start));
        obj->SetSlot(engine->AllocStringNC("stop"),  Value((pint_t)end));
        
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
    
    Array* Exec(String* subj)
    {
        if (!this->pattern) return 0;
        if (!IsGlobal()) return Exec(subj);
        GCPAUSE_NORUN(engine);
        Buffer<size_t> matches; // Buffer to store matches.
        size_t lastIndex = 0;   // Current pos in the string.
        size_t subjLen = subj->GetLength(); // Total length of the string.
        Array* res = Array::Create(engine, engine->Array_Type, 0, 0); // Resultant Array object.
        
        
        if (!DoExec(subj->GetBuffer() + lastIndex, subjLen - lastIndex, matches))
            return res;
        
        for (size_t i = 0; i < (matches.GetSize() / 2); ++i)
        {
            size_t const start = matches[i*2];
            size_t const end   = matches[i*2 + 1];
            Value match = GetMatch(start, end, subj);
            res->Push(match);
        }
        return res;
    }
    
    Value ExecOnce(String* subj)
    {
        if (!this->pattern) return Value(NULL_VALUE);

        Buffer<size_t> matches;
        if (DoExec(subj->GetBuffer(), subj->GetLength(), matches))
        {
            size_t m = IndexOfLongestMatch(matches);
            
            size_t const matchIndex = matches[m];
            size_t const matchStop = matches[m + 1];
                        
            return GetMatch(matchIndex, matchStop, subj);
        }
        return Value(NULL_VALUE);
    }
    
    bool Test(String* subj)
    {
        if (!this->pattern) return false;
        
        Pika_regmatch ovector[NUM_MATCHES];
        int match = Pika_regexec(this->pattern, subj->GetBuffer(), subj->GetLength(),                              
                              ovector, NUM_MATCHES, PIKA_NO_UTF8_CHECK);
        if (match > 0)
        {
            return true;
            /*int const matchIndex = ovector[0].start;
            int const matchLen   = ovector[0].end - ovector[0].start;
            return matchIndex == 0 && (size_t)matchLen == subj->GetLength();*/
        }
        return false;
    }
    
    bool IsGlobal() const { return true; }
    
    Array* Match(String* subj)
    {
        if (!this->pattern) return 0;
        if (!IsGlobal()) return Exec(subj);
        GCPAUSE_NORUN(engine);
        Buffer<size_t> matches; // Buffer to store matches.
        size_t lastIndex = 0;   // Current pos in the string.
        size_t subjLen = subj->GetLength(); // Total length of the string.
        Array* res = Array::Create(engine, engine->Array_Type, 0, 0); // Resultant Array object.
        
        while (DoExec(subj->GetBuffer() + lastIndex, subjLen - lastIndex, matches))
        {
            size_t const start = matches[0];
            size_t const end   = matches[1];
            size_t const len   = end - start;
            
            // If its an empty match.
            if (len == 0)
            {
                Value estr(engine->emptyString);
                res->Push(estr);
                if (lastIndex == subjLen)
                {
                    break; // Break if we are at the end of the subject string.
                }
                else
                {
                    // Otherwise goto the next character.
                    lastIndex++;
                    continue;
                }
            }
            
            // Create a nonempty match string.
            String* matchstr = engine->AllocString(subj->GetBuffer() + start + lastIndex, len);
            Value vs(matchstr);
            res->Push(vs);
            lastIndex += start + len;
        }
        return res;
    }
    
    bool DoExec(const char* subj, size_t subjLen, Buffer<size_t>& res)
    {
        Pika_regmatch ovector[NUM_MATCHES];
        int matchCount = Pika_regexec(this->pattern, subj, subjLen, ovector, NUM_MATCHES, PIKA_NO_UTF8_CHECK);
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
    
    String* Replace(String* subj, String* rep)
    {
        if (!this->pattern) return subj;
        GCPAUSE_NORUN(engine);
        
        size_t const subjlen = subj->GetLength();
        size_t lastIndex = 0;
        size_t const repSize = rep->GetLength();
        const char*  repBuf  = rep->GetBuffer();
        
        Buffer<char>   buff;
        Buffer<size_t> matches;
        
        while (lastIndex <= subjlen && DoExec(subj->GetBuffer() + lastIndex, subjlen - lastIndex, matches))
        {
            size_t const longest    = IndexOfLongestMatch(matches);
            size_t const matchIndex = matches[longest];
            size_t const nextIndex  = matches[longest + 1];
            size_t const matchLen   = nextIndex - matchIndex;
            size_t const prevStart  = lastIndex;
            size_t const prevEnd    = lastIndex + matchIndex;
            bool   const emptyMatch = (matchLen == 0);
            
            if (prevEnd >= prevStart)
            {
                size_t const sz     = buff.GetSize();
                size_t const amt    = emptyMatch ? 1 : prevEnd - prevStart;
                size_t const repamt = repSize;
                size_t const total  = sz + amt + repamt;
                
                if (total > PIKA_STRING_MAX_LEN)
                {
                    RaiseException("RegExp.replace: resultant string is too large.");
                }
                buff.Resize(total);
                
                if (emptyMatch)
                {
                    // We matched the empty string
                    Pika_memcpy(buff.GetAt(sz),  repBuf, repSize);
                    Pika_memcpy(buff.GetAt(sz + repSize),  subj->GetBuffer() + lastIndex, amt); // Copy the next character
                    lastIndex++;                                                                // and move past it. 
                }
                else
                {
                    if (amt)
                        Pika_memcpy( buff.GetAt(sz),  subj->GetBuffer() + lastIndex, amt);
                    Pika_memcpy( buff.GetAt(sz + amt),  repBuf, repSize);
                }
            }
            
            if (lastIndex > subjlen && matchLen == 0)
            {
                break;
            }
            lastIndex += matchIndex + matchLen;
        }
        
        if (lastIndex == 0) // We had no non-empty matches...
            return subj;    // so return the entire string.
        
        if (lastIndex < subjlen) // We did not consume the entire string...
        {
            size_t const sz    = buff.GetSize();
            size_t const amt   = subjlen - lastIndex;
            size_t const total = sz + amt;
            
            if (total > PIKA_STRING_MAX_LEN)
            {
                RaiseException("RegExp.replace: resultant string is too large.");
            }
            // so copy the rest from the subject's buffer.
            buff.Resize(total);
            Pika_memcpy(buff.GetAt(sz),  subj->GetBuffer() + lastIndex, amt);
        }
        
        if (buff.GetSize())
        {
            return engine->AllocString(buff.GetAt(0), buff.GetSize());
        }
        return engine->emptyString;
    }

    static void Constructor(Engine* eng, Type* obj_type, Value& res)
    {
        RegExp* re = RegExp::StaticNew(eng, obj_type, 0);
        res.Set(re);
    }

    Pika_regex* pattern;
};

PIKA_IMPL(RegExp)

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
    .Method(&RegExp::Match,    "match")
    .Method(&RegExp::Exec,     "exec")
    .Method(&RegExp::ExecOnce, "execOnce")
    .Method(&RegExp::Test,     "test")
    .Method(&RegExp::Compile,  "compile")
    .Method(&RegExp::Replace,  "replace")
    .MethodVA(&RegExp::Init,   "init")
    ;
    //RegExp_Type->EnterMethods(RegExpFunctions, countof(RegExpFunctions));
    re->SetSlot(RegExp_String, RegExp_Type);
    return RegExp_Type;
}

