/*
 *  PFile.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PFile.h"
#include "PPlatform.h"
#define PIKA_BUFSIZ 1024

namespace pika {
namespace  {


bool Pika_fok(FILE* f)
{
    if (ferror(f))
    {
        clearerr(f);
        return false;
    }
    return true;
}

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

bool Pika_fgetln(FILE* fp, TStringBuffer& buff)
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

}

PIKA_DOC(File_readLine, "/()\
\n\
Returns the next line in the file. If the file is not open or has reached the \
end of file marker then '''null''' is returned.")

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
    return 0;
}

PIKA_DOC(File_readLines, "/()\
\n\
Returns an [Array] filled with all the lines remaining in the file. If the file \
is not open or has reached the end of file marker then '''null''' is returned.")

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
    handle(0),
    use_paths(true)    
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

PIKA_DOC(File_toBoolean, "/()\
\n\
Returns '''true''' if the file handle is valid. Otherwise, '''false''' will be returned.\
")

bool File::ToBoolean()
{
    return handle != 0;
}

PIKA_DOC(File_open, "/(name, opts)\
\n\
Opens the file |name| and returns '''true''' of '''false''' depending on if the \
file could be opened successfully. |opts| should be a [String string] \
containing the options.\n\
Valid options include: 'r' - read, 'w' - write, 'b' - binary, 't' - text.\
[[[\
file = File.new()\n\
if file.open('path/to/file', 'wt')\
]]]\n\
If file.usPaths is '''true''' then [imports.os.paths os.paths] will be searched if |name| is not \
a relative or absolute path from the current directory.\
")

bool File::Open(String* name, String* opts)
{
    if (handle) Close();
    GCPAUSE(engine);
    
    String* fullpath = use_paths ? engine->Paths()->FindFile(name) : 0;
    
    if (fullpath)
        name = fullpath;
    
    if ((handle = fopen(name->GetBuffer(), opts->GetBuffer())))
    {
        filename = name;
        return Pika_fok(handle);
    }
    return false;
}

PIKA_DOC(File_close, "/()\
\n\
Closes the file if it is open. If the file not open or is a \
std input stream nothing will be done.\
")

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

pint_t File::Fileno()
{
    if (handle) {
        return fileno(handle);
    }
    return 0;
}

PIKA_DOC(File_eof, "/()\
\n\
Returns '''true''' if the file is open and has reached the end of the file. \
Returns '''false''' if an open file has not reach the end of the file. \
If the file is closed or is not open '''true''' will be returned. \
")

bool File::IsEof()
{
    return handle ? (feof(handle) != 0) : true;
}

PIKA_DOC(File_read, "/([amt])\
\n\
If no arguments are passed then the whole file will be read. Otherwise the \
number of bytes/characters specified by |amt| will be read and returned.\
")

String* File::Read(Context* ctx)
{
    if (!handle)
        RaiseException("Attempt to read an invalid or closed file.");

    u2 argc = ctx->GetArgCount();

    if (argc == 1)
    {
        pint_t arg0 = ctx->GetIntArg(0);
        if (arg0 <= 0)
            return engine->emptyString;
        size_t const amt = (size_t)(arg0);
        engine->string_buff.Resize(amt);
        size_t amtread = fread(engine->string_buff.GetAt(0), 1, amt, handle);
        
        return engine->AllocString(engine->string_buff.GetAt(0), amtread);
    }
    else if (argc == 0)
    {
        size_t readamt = PIKA_BUFSIZ;   // Number of bytes to read at a time.
        size_t count = 0;               // Number of bytes read in total.
        size_t n = 0;                   // Number of bytes we just read.

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

PIKA_DOC(File_write, "/(*va)\
\n\
Write each argument to the file. Each argument will be converted to a [String] \
if not already one. If the file is invalid or closed and exception will be raised.\
")

bool File::Write(Context* ctx)
{
    if ( !handle )
        RaiseException("Attempt to write to an invalid or closed file.");

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

PIKA_DOC(File_init, "/([name, opts])\
\n\
Initializes a new File instance. Optionally you can \
open the file |name| with options |opts| for more information on the arguments \
see [open File.open]. If |name| and |opts| are supplied and the file could not be opened \
and exception will be raised.\
")

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

PIKA_DOC(File_seek, "/(off, start)\
\n\
Sets the current position of the file. The position is determined by the \
offset, |off|, from the starting point |start|.\
\n\
|start| should be one of:\
<ul><li>File.SEEK_BEG - Start from the beginning of the File</li>\
<li>File.SEEK_END - Start from the end of the File.</li>\
<li>File.SEEK_CUR - Start from the current position</li></ul>\
")

pint_t File::Seek(pint_t off, pint_t startingPoint)
{
    if (handle)
    {
        if (startingPoint != SEEK_SET && startingPoint != SEEK_CUR && startingPoint != SEEK_END)
        {
            RaiseException("incorrect starting point for seek.");
        }
        int res = fseek(handle, off, startingPoint);
        return res;
    }
    return 0;
}

PIKA_DOC(File_setPos, "/(pos)\
\n\
Sets the position of the file. If |pos| is positive then the starting point is \
the beginning of the file. If |pos| is negative then the starting point is the \
end of the file.\
")

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

PIKA_DOC(File_advance, "/(off)\
\n\
Advance the file |off| number of bytes from the current position.\
")

pint_t File::Advance(pint_t off)
{
    if (handle)
    {
        int whence = SEEK_CUR;
        int res = fseek(handle, off, whence);
        return res;
    }
    return 0;
}

PIKA_DOC(File_flush, "/(off)\
\n\
Flush the file's buffer forcing any pending [read] or [write] operations.\
")

void File::Flush()
{
    if (handle && handle != stdin)
    {
        fflush(handle);
    }
}

String* File::GetFilename() { return filename; }

PIKA_DOC(File_opened, "/()\
\n\
Returns '''true''' if the file is opened and '''false''' if it is not opened.\
")

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

PIKA_DOC(File_rewind, "/()\
\n\
Rewinds the file if it is opened.")

void File::Rewind()
{
    if (handle)
    {
        rewind(handle);
    }
}

PIKA_DOC(File_getUsePaths, "/()\
\n\
Returns whether [imports.os.paths os.paths] is used to find files.")

bool File::GetUsePaths() const
{
    return use_paths;
}

PIKA_DOC(File_setUsePaths, "/(use?)\
\n\
If |use?| is '''true''' then [imports.os.paths os.paths] will be used to find files. \
Otherwise file names will need to be relative to the working \
directory or an absolute path to the file.")

void File::SetUsePaths(bool use)
{
    use_paths = use;
}

namespace {

PIKA_DOC(File_rename, "/(old_name, new_name)\
\n\
Renames the file named |old_name| to |new_name|. Returns '''true''' if successful, '''false''' on failure.\
")

bool File_rename(String* oldname, String* newname)
{
    return rename(oldname->GetBuffer(), newname->GetBuffer()) == 0;
}

}// namespace

void File::Constructor(Engine* eng, Type* type, Value& obj)
{
    File* f = File::Create(eng, type);
    obj.Set(f);
}

PIKA_DOC(file_stdout, "A File object for the standard output stream")
PIKA_DOC(file_stderr, "A File object for the standard error stream")
PIKA_DOC(file_stdin, "A File object for the standard input stream")

void File::StaticInitType(Engine* eng)
{
    String* File_String = eng->AllocString("File");
    Type*   File_Type   = Type::Create(eng, File_String, eng->Object_Type, File::Constructor, eng->GetWorld());
    
    // Create the std file streams
    
    File* file_stdout = File::Create(eng, File_Type);
    File* file_stderr = File::Create(eng, File_Type);
    File* file_stdin  = File::Create(eng, File_Type);
    
    file_stdout->SetSlot(eng->GetString("__doc"), eng->GetString(PIKA_GET_DOC(file_stdout)), Slot::ATTR_forcewrite); 
    file_stderr->SetSlot(eng->GetString("__doc"), eng->GetString(PIKA_GET_DOC(file_stderr)), Slot::ATTR_forcewrite); 
    file_stdin->SetSlot(eng->GetString("__doc"), eng->GetString(PIKA_GET_DOC(file_stdin)), Slot::ATTR_forcewrite); 
    
    SlotBinder<File>(eng, File_Type)
    .Method(&File::ToBoolean,   "toBoolean", PIKA_GET_DOC(File_toBoolean))
    .PropertyRW("usePaths", 
            &File::GetUsePaths, "getUsePaths", 
            &File::SetUsePaths, "setUsePaths",
            PIKA_GET_DOC(File_getUsePaths),
            PIKA_GET_DOC(File_setUsePaths))
    .Method(&File::Open,        "open", PIKA_GET_DOC(File_open))
    .Method(&File::Close,       "close",PIKA_GET_DOC(File_close))
    .Method(&File::IsEof,       "eof?", PIKA_GET_DOC(File_eof))
    .MethodVA(&File::Write,     "write",PIKA_GET_DOC(File_write))
    .MethodVA(&File::Read,      "read", PIKA_GET_DOC(File_read))
    .Method(&File::Seek,        "seek", PIKA_GET_DOC(File_seek))
    .Method(&File::SetPos,      "setPos",PIKA_GET_DOC(File_setPos))
    .Method(&File::Advance,     "advance", PIKA_GET_DOC(File_advance))
    .Method(&File::Flush,       "flush", PIKA_GET_DOC(File_flush))
    .Method(&File::ReadLine,    "readLine",  PIKA_GET_DOC(File_readLine))
    .Method(&File::ReadLines,   "readLines", PIKA_GET_DOC(File_readLines))
    .Method(&File::IsOpen,      "opened?",   PIKA_GET_DOC(File_opened))
    .Method(&File::Rewind,      "rewind",    PIKA_GET_DOC(File_rewind))
    .MethodVA(&File::Init,      "init",      PIKA_GET_DOC(File_init))
    .Method(&File::Fileno,      "fileno")
    .Constant((pint_t)SEEK_SET, "SEEK_BEG")
    .Constant((pint_t)SEEK_CUR, "SEEK_CUR")
    .Constant((pint_t)SEEK_END, "SEEK_END")
    .Constant(file_stdout,      "stdout")
    .Constant(file_stderr,      "stderr")
    .Constant(file_stdin,       "stdin")
    .StaticMethod(File_rename,  "rename", PIKA_GET_DOC(File_rename))
    ;
    
    file_stdout->SetFilename(eng->AllocString("stdout"));
    file_stderr->SetFilename(eng->AllocString("stderr"));
    file_stdin ->SetFilename(eng->AllocString("stdin"));
    
    file_stdout->SetHandle(stdout);
    file_stderr->SetHandle(stderr);
    file_stdin ->SetHandle(stdin);
    
    eng->GetWorld()->SetSlot(File_String, File_Type);
}

}// pika
