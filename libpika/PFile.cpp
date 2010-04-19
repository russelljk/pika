/*
 *  PFile.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PFile.h"
#include "PPlatform.h"
#define PIKA_BUFSIZ 1024

namespace pika {

static bool Pika_fok(FILE* f)
{
    if (ferror(f))
    {
        clearerr(f);
        return false;
    }
    return true;
}

/*------------------------------------------------------------------------*/
/** Reads a line from a file.
  *
  * @param fp        [in]    File pointer
  * @param buff      [out]   The line read in, excluding the newline character.
  *
  * @retval   true    When a line was successfully read
  * @retval   false   When the file doesn
  */

// TODO: (Pika_fgetln) CR, CR + LF, and LF should be supported as valid eol markers
// TODO: (Pika_fgetln) We need to check to make sure the buffer size does not exceed the MAX buffer size.

static bool Pika_fgetln(FILE* fp, TStringBuffer& buff)
{
    size_t bufsiz = PIKA_BUFSIZ;
    size_t len    = 0;
    const char*  ptr    = 0;

    buff.Resize(bufsiz);
    buff[0] = '\0';

    if (feof(fp) || (fgets(buff.GetAt(0), bufsiz, fp) == 0))
    {
        // We could not read.
        buff.Resize(0);
        return false;
    }

    // While the string read does not contain a newline
    while ((ptr = Pika_index(buff.GetAt(len), '\n')) == 0)
    {
        // Increase the buffer size
        size_t nbufsiz = bufsiz + PIKA_BUFSIZ;
        buff.Resize(nbufsiz);

        // Read some more
        if (fgets(buff.GetAt(bufsiz), PIKA_BUFSIZ, fp) == 0)
        {
            // eof reached.
            len = strlen(buff.GetAt(0));
            buff.Resize(len);
            return true;
        }

        len = bufsiz;
        bufsiz = nbufsiz;
    }

    // At this point buff[len] == '\n'

    // Exclude the newline from the resultant string.
    len = (ptr - buff.GetAt(0));
    buff.Resize(len);

    return true;
}

String* File::ReadLine()
{
    if (!handle) return 0;

    GCPAUSE_NORUN(engine);

    if (!Pika_fgetln(handle, engine->string_buff))
        return 0;

    if (!engine->string_buff.GetSize()) 
        return engine->emptyString;

    String* str = engine->AllocString(engine->string_buff.GetAt(0),
                                      engine->string_buff.GetSize());
    return str;
}

Object* File::Clone()
{
    return File::Create(engine, GetType());
}

Array* File::ReadLines()
{
    if (!handle || feof(handle)) return 0;

    GCPAUSE_NORUN(engine);

    Array* v = 0;

    while (!feof(handle))
    {
        if (Pika_fgetln(handle, engine->string_buff))
        {
            String* str = engine->AllocString(engine->string_buff.GetAt(0),
                                              engine->string_buff.GetSize());
            Value vstr(str);

            if (!v)
            {
                v = Array::Create(engine, engine->Array_Type, 0, 0);
            }
            v->Push(vstr);
        }
    }
    return v;
}

//////////////////////////////////////////////// File //////////////////////////////////////////////

PIKA_IMPL(File)

File::File(Engine* eng, Type* obj_type)
    : ThisSuper(eng, obj_type),
    filename(0), 
    handle(0)
    
{}

File::~File()
{
    Close();
}

void File::MarkRefs(Collector* c)
{
    ThisSuper::MarkRefs(c);

    if (filename) filename->Mark(c);
}

bool File::ToBoolean()
{
    return handle != 0;
}

bool File::Open(String* name, String* opts)
{
    if (handle) Close();
    GCPAUSE(engine);

    String* fullpath = engine->GetPathManager()->FindFile(name);

    if (fullpath)
        name = fullpath;
    
    if ( (handle = fopen(name->GetBuffer(), opts->GetBuffer())) )
    {
        filename = name;
        return Pika_fok(handle);
    }
    return false;
}

void File::Close()
{
    if (handle)
    {
        if (handle != stderr && handle != stdin  && handle != stdout)
        {
            fclose(handle);
        }
        handle = 0;
    }
    filename = 0;
}

bool File::IsEof()
{
    return handle ? (feof(handle) != 0) : true;
}

String* File::Read(Context* ctx)
{
    if (!handle)
        return engine->emptyString;

    u2 argc = ctx->GetArgCount();

    if (argc == 1)
    {
        pint_t amt = ctx->GetIntArg(0);
        if (amt < 0)
            return engine->emptyString;

        engine->string_buff.Resize(amt);
        size_t amtread = fread(engine->string_buff.GetAt(0), 1, amt, handle);

        return engine->AllocString(engine->string_buff.GetAt(0), amtread);
    }
    else if (argc == 0)
    {
        size_t readamt = PIKA_BUFSIZ; // Number of bytes to read at a time.
        size_t count   = 0;           // Number of bytes read in total.
        size_t n       = 0;           // Number of bytes we just read.

        do
        {
            engine->string_buff.Resize(readamt + count);
            n = fread(engine->string_buff.GetAt(count), 1, readamt, handle);
            count += n;
        }
        while (n > 0 && !feof(handle) && Pika_fok(handle));

        return engine->AllocString(engine->string_buff.GetAt(0), count);
    }
    else
    {
        ctx->WrongArgCount();
    }
    return engine->emptyString;
}

bool File::Write(Context* ctx)
{
    if ( !handle )
        return false;

    GCPAUSE(engine);
    u2 argc = ctx->GetArgCount();

    for (u2 i = 0; i < argc; ++i)
    {
        String* buf = ctx->ArgToString(i);

        if ( !(buf && buf->GetLength()) )
            continue;

        size_t amtwritten = fwrite( buf->GetBuffer(), buf->GetLength(), 1, handle );
        return ( amtwritten == buf->GetLength() ) && Pika_fok(handle);
    }
    return Pika_fok(handle);
}

File* File::Create(Engine* eng, Type* obj_type)
{
    File* f;
    PIKA_NEW(File, f, (eng, obj_type));
    eng->AddToGC(f);
    return f;
}

void File::Init(Context* ctx)
{
    u2 argc = ctx->GetArgCount();

    if ( argc == 2 )
    {
        String* name = ctx->GetStringArg(0);
        String* opts = ctx->GetStringArg(1);

        if ( !Open(name, opts) )
        {
            RaiseException("error opening file: %s. (%s)", name->GetBuffer(), strerror(errno));
        }
    }
    else if ( argc != 0 )
    {
        ctx->WrongArgCount();
    }
}

pint_t File::Seek(pint_t pos, pint_t startingPoint)
{
    if (handle)
    {
        if (startingPoint != SEEK_SET && startingPoint != SEEK_CUR && startingPoint != SEEK_END)
        {
            RaiseException("incorrect starting point for seek.");
        }
        int res = fseek(handle, pos, startingPoint);
        return res;
    }
    return 0;
}

pint_t File::SetPos(pint_t pos)
{
    if (handle)
    {

        int whence;
        if (pos < 0)
        {
            whence = SEEK_END;
            pos = -pos;
        }
        else
        {
            whence = SEEK_SET;
        }

        int res = fseek(handle, pos, whence);
        return res;
    }
    return 0;
}

pint_t File::Advance(pint_t pos)
{
    if (handle)
    {
        int whence = SEEK_CUR;
        int res = fseek(handle, pos, whence);
        return res;
    }
    return 0;
}

void File::Flush()
{
    if (handle && handle != stdin)
    {
        fflush(handle);
    }
}

String* File::GetFilename() { return filename; }

bool File::IsOpen() const { return handle != 0; }

void File::SetFilename(String* str)
{
    if (str)
    {
        filename = str;
        WriteBarrier(filename);
    }
    else
    {
        filename = 0;
    }
}

void File::SetHandle(FILE* f)
{
    handle = f;
}

void File::Rewind()
{
    if (handle)
    {
        rewind(handle);
    }
}

}// pika

static bool File_rename(String* oldname, String* newname)
{
    return rename(oldname->GetBuffer(), newname->GetBuffer()) == 0;
}

static String* TempName(Context* ctx)
{
    char buffer[L_tmpnam]; // or PIKA_MAX_PATH

    if (tmpnam(buffer))
    {
        String* tempname = ctx->GetEngine()->AllocString(buffer);
        return tempname;
    }
    return 0;
}

static void File_new(Engine* eng, Type* type, Value& obj)
{
    File* f = File::Create(eng, type);
    obj.Set(f);
}

void InitFileAPI(Engine* eng)
{
    String* File_String = eng->AllocString("File");
    Type*   File_Type   = Type::Create(eng, File_String, eng->Object_Type, File_new, eng->GetWorld());
    
    // Create the std file streams
    
    File* file_stdout = File::Create(eng, File_Type);
    File* file_stderr = File::Create(eng, File_Type);
    File* file_stdin  = File::Create(eng, File_Type);
    
    SlotBinder<File>(eng, File_Type, eng->GetWorld())
    .Method(&File::ToBoolean,   "toBoolean")
    .Method(&File::Open,        "open")
    .Method(&File::Close,       "close")
    .Method(&File::IsEof,       "eof?")
    .MethodVA(&File::Write,     "write")
    .MethodVA(&File::Read,      "read")
    .Method(&File::Seek,        "seek")
    .Method(&File::SetPos,      "setPos")
    .Method(&File::Advance,     "advance")
    .Method(&File::Flush,       "flush")
    .Method(&File::ReadLine,    "readLine")
    .Method(&File::ReadLines,   "readLines")
    .Method(&File::IsOpen,      "open?")
    .Method(&File::Rewind,      "rewind")
    .Constant((pint_t)SEEK_SET, "SEEK_SET")
    .Constant((pint_t)SEEK_SET, "SEEK_BEG")
    .Constant((pint_t)SEEK_CUR, "SEEK_CUR")
    .Constant((pint_t)SEEK_END, "SEEK_END")
    .Constant(file_stdout,      "stdout")
    .Constant(file_stderr,      "stderr")
    .Constant(file_stdin,       "stdin")
    .StaticMethodVA(TempName,   "tempName")
    .StaticMethod(File_rename,  "rename")
    ;
    
    file_stdout->SetFilename(eng->AllocString("stdout"));
    file_stderr->SetFilename(eng->AllocString("stderr"));
    file_stdin ->SetFilename(eng->AllocString("stdin"));
    
    file_stdout->SetHandle(stdout);
    file_stderr->SetHandle(stderr);
    file_stdin ->SetHandle(stdin);
    
    eng->GetWorld()->SetSlot(File_String, File_Type);
}
