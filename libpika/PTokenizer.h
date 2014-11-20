/*
 * PTokenizer.h
 * See Copyright Notice in Pika.h
 *
 * --------------------------------------------------------------------------------------
 * classes:
 *      StringToNumber
 *      IScriptStream
 *          FileScriptStream
 *      Tokenizer
 * --------------------------------------------------------------------------------------
 */
#ifndef PIKA_TOKENIZER_HEADER
#define PIKA_TOKENIZER_HEADER

#include "PTokenDef.h"

/* End of Input. Used for REPL. Not the same as End of File, and EOI means that more input can be */
#define EOI (-2) 

namespace pika {

struct CompileState;

/** Converts a C string to an integer or floating point value. Uses the same algorithm and
  * formats as the Tokenizer, the only exception is that html hex values (#ffcc99) are supported.
  */
struct StringToNumber
{
    StringToNumber(const char* str, size_t len, bool eatwsp = true, bool use_html_hex = true, bool must_consume = true);
    StringToNumber(const char* str, const char* end, bool must_consume = true);
    
    INLINE bool     IsValid()   { return is_number; }
    INLINE bool     IsInteger() { return IsValid() && is_integer; }
    INLINE bool     IsReal()    { return IsValid() && is_real; }

    INLINE s8       GetInteger() { return integer; }
    INLINE double   GetReal()    { return real; }

    INLINE int      Peek() { if (curr >= end) return EOF; return *(curr);}  
    INLINE int      Look() { return look; }
    
    INLINE void     NumberParseError(const char* msg) {is_number = 0;}

    void            GetLook();
    
    INLINE          operator bool() { return IsValid(); }
private:
    void            EatWhiteSpace();    
    void            ReadNumber();
        
    int             look;
    const char*     curr;
    const char*     buffer;
    const char*     end;
    union
    {
        double      real;
        s8          integer; 
    };
    u4              is_number:1;
    u4              is_real:1;
    u4              is_integer:1;
    u4              eat_white:1;
    u4              html_hex:1;
};

/** Interface for a script's input stream. */
struct IScriptStream
{
    virtual ~IScriptStream() = 0;
    
    virtual bool IsREPL() const { return false; }
        
    /** Get and return the current character in the stream. 
      * The position of the stream should move to the next character before this method returns. */
    virtual int  Get() = 0;

    /** Get and return the next character in the stream. */
    virtual int  Peek() = 0;
    
    /** Returns true if we have reached the end of the stream. */
    virtual bool IsEof() = 0;

    /** Returns true if we have reached the end of the current input line. Used only for REPL. */
    virtual bool IsEoi() { return false; }
};

/** C file based input stream. 
  * @note Reads the entire file into a buffer. */
struct FileScriptStream : IScriptStream
{
    FileScriptStream(std::ifstream* fs);
    virtual ~FileScriptStream();

    virtual int  Get();
    virtual int  Peek();
    virtual bool IsEof();

private:
    void CheckGood();
    
    char*  buffer;
    size_t pos;
    size_t bufferLength;
    std::ifstream* stream;
};

struct StdinScriptStream : IScriptStream
{
    virtual ~StdinScriptStream() {}
    
    /** Get and return the current character in the stream. 
      * The position of the stream should move to the next character before this method returns. */
    virtual int  Get();

    /** Get and return the next character in the stream. */
    virtual int  Peek();
    
    /** Returns true if we have reached the end of the stream. */
    virtual bool IsEof();
};

struct REPLStream : IScriptStream
{
    enum {
        REPL_BUFF_SZ = 256
    };
    
    REPLStream();
    
    virtual ~REPLStream();
    
    virtual bool IsREPL() const { return true; }
    void NewLoop(const char* pmt=0);
    virtual int  Get();
    virtual int  Peek();
    virtual bool IsEof();
    virtual bool IsEoi();
    
    bool GetNewLine(const char* pmt);
private:
    char buffer[REPL_BUFF_SZ];
    size_t pos;
    size_t bufferLength;
    bool is_eof;
};

/** String based input stream. */
struct StringScriptStream : IScriptStream
{
    StringScriptStream(const char* buf, size_t len);
    virtual ~StringScriptStream();
    
    virtual int  Get();
    virtual int  Peek();
    virtual bool IsEof();

private:
    char*  buffer;
    size_t pos;
    size_t bufferLength;
};

/** Converts a input stream into a series of tokens. */
class Tokenizer
{
public:
    Tokenizer(CompileState*, std::ifstream* stream);
    Tokenizer(CompileState*, const char* buf, size_t len);
    Tokenizer(CompileState*, IScriptStream* stream);
    virtual ~Tokenizer();
    
    int             Peek();
    INLINE int      Look() const { return look; }
    
    INLINE int            GetTokenType()   const { return tokenType;    }    
    INLINE size_t         GetBeginOffset() const { return tokenBegin;   }
    INLINE size_t         GetEndOffset()   const { return tokenEnd - 1; }    
    INLINE const char*    GetBuffer()      const { return script_buf.GetAt(0); }    
    INLINE u4             GetCol()         const { return prevcol; }
    INLINE int            GetLn()          const { return prevline; }    
    INLINE const YYSTYPE& GetVal()         const { return tokenVal; }
    
    void            GetNext();
    void            GetLook();
    void            GetMoreInput();
    
    void            NumberParseError(const char* msg);
protected:
    virtual void    PrepKeywords();
    virtual void    HandleEOS() {look = EOF;}

    const char*     GetBeginPtr() const { return script_buf.GetAt(tokenBegin); }
    const char*     GetEndPtr()   const { return script_buf.GetAt(tokenEnd);   }
    
    void            CheckBom();
    
    bool            IsEndOfStream();
    void            EatWhitespace();
    u4              ReadDigits(u4 radix, u8& int_part);
    void            ReadNumber();
    void            ReadIdentifier();
    void            ReadString(bool is_plain=false);
    void            ReadControl();
    void            ReadMultiLineComment(int);
    void            ReadSingleLineComment();
    int             is_str_ctor;
    size_t          tokenBegin;
    size_t          tokenEnd;
    YYSTYPE         tokenVal;
    CompileState*   state;
    int             tabIndentSize;
    int             line;
    int             col;
    int             ch;
    int             prevline;
    int             prevcol;
    int             prevch;
    int             tokenType;
    int             look;
    bool            hasUtf8Bom;
    size_t          minKeywordLength;
    size_t          maxKeywordLength;
    Buffer<char>    script_buf;
    IScriptStream*  script_stream;
};

}// pika

#endif
