/*
 *  PZipReader.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PZipFile.h"
#include "PZipReader.h"

namespace pika {
    ZipReaderFile::ZipReaderFile(Engine* engine, Type* type, ZipReader* zf) 
        : ThisSuper(engine, type), zipfile(zf), filename(engine->emptyString)
    {
        Pika_memzero(&file_info, sizeof(unz_file_info));
    }
    
    ZipReaderFile* ZipReaderFile::StaticNew(Engine* eng, Type* type, ZipReader* reader)
    {
        ZipReaderFile* obj = 0;
        GCNEW(eng, ZipReaderFile, obj, (eng, type, reader));
        return obj;
    }
    
    size_t ZipReaderFile::GetFileSize()
    {
        return this->file_info.uncompressed_size;        
    }
    
    size_t ZipReaderFile::GetCompressedSize()
    {
        return this->file_info.compressed_size;
    }
    
    String* ZipReaderFile::GetFileName()
    {
        return this->filename;
    }
    
    ZipReader* ZipReaderFile::GetReader()
    {
        return this->zipfile;
    }
    
    void ZipReaderFile::MarkRefs(Collector* c)
    {
        ThisSuper::MarkRefs(c);
        if (zipfile)   zipfile->Mark(c);
        if (filename) filename->Mark(c);
    }
    
    void ZipReaderFile::SetName(String* name)
    {
        WriteBarrier(name);
        this->filename = name;
    }
    
    ZipReader::ZipReader(Engine* engine, Type* type) : 
        ThisSuper(engine, type),
        file(0),
        filename(engine->emptyString),
        currentFile(0) 
    {
        Pika_memzero(&globalInfo, sizeof(unz_global_info));
    }
    
    ZipReader::~ZipReader()
    {
        Close();
    }
    
    String* ZipReader::GetFileName()
    {
        return this->filename;
    }
    
    String* ZipReader::ReadAllCurrentFile()
    {
        if (!currentFile) {
            RaiseException(
                Exception::ERROR_type,
                "Cannot read from the current file. No current file.");
        }
        return this->ReadCurrentFile(currentFile->GetFileSize());
    }
    
    String* ZipReader::ReadCurrentFile(pint_t amt)
    {
        if (!currentFile) {
            RaiseException(
                Exception::ERROR_type,
                "Cannot read from the current file. No current file.");
        }
        if (amt == -1) {
            amt = currentFile->GetFileSize();
        }
        else if (amt < 0) {
            RaiseException(
                Exception::ERROR_type,
                "Invalid number of bytes given to ZipReader.read.");
        }
        
        engine->string_buff.Resize(amt);
        int res = unzReadCurrentFile(this->file, engine->string_buff.GetAt(0), engine->string_buff.GetSize());
        
        if (res != amt) {
            RaiseException(
                Exception::ERROR_runtime,
                "ZipReader could not read %d bytes from the current file.", amt);
        }
        
        return engine->GetString(engine->string_buff.GetAt(0), 
                        res);
    }
    
    bool ZipReader::OpenCurrentFile()
    {
        if (unzOpenCurrentFile(this->file) == UNZ_OK)
        {
            this->SetCurrentFile();
            return true;
        }
        return false;
    }
    
    bool ZipReader::OpenCurrentFilePassword(String* pass)
    {
        return unzOpenCurrentFilePassword(this->file, pass->GetBuffer()) == UNZ_OK;        
    }
        
    void ZipReader::CloseCurrentFile()
    {
        unzCloseCurrentFile(this->file);
    }
    
    void ZipReader::Close()
    {
        if (currentFile)
        {
            this->CloseCurrentFile();
        }
        
        if (file)
        {
            unzClose(file);
            file = 0;
        }
    }
    
    void ZipReader::MarkRefs(Collector* c)
    {
        ThisSuper::MarkRefs(c);
        
        if (this->filename)
        {
            this->filename->Mark(c);
        }
        
        files.DoMark(c);
    }
    
    pint_t ZipReader::GetFileCount()
    {
        return this->globalInfo.number_entry;
    }
    
    void ZipReader::Open(String* fn)
    {
        if (this->file)
        {
            RaiseException(
                ZipReaderError::StaticGetClass(),
                "Cannot open an already opened ZipReader."
            );            
        }
        
        this->file = unzOpen(fn->GetBuffer());
        
        if (!this->file)
        {
            RaiseException(
                ZipReaderError::StaticGetClass(),
                "Attempt to open zip file failed."
            );
        }
        
        unzGetGlobalInfo(this->file, &globalInfo);
        filename = fn;
    }
    
    ZipReader* ZipReader::StaticNew(Engine* eng, Type* type)
    {
        ZipReader* obj = 0;
        GCNEW(eng, ZipReader, obj, (eng, type));
        return obj;
    }

    void ZipReader::Constructor(Engine* eng, Type* obj_type, Value& res)
    {
        Object* obj = ZipReader::StaticNew(eng, obj_type);
        res.Set(obj);
    }
    
    bool ZipReader::FirstFile()
    {
        int res = unzGoToFirstFile(this->file);
        return this->HandleResult(res);
    }
        
    bool ZipReader::NextFile()
    {
        int res = unzGoToNextFile(this->file);
        return this->HandleResult(res);
    }
    
    bool ZipReader::SetFilePos(pint_t zip_pos, pint_t file_num)
    {
        unz_file_pos pos = {
            static_cast<u4>(zip_pos),
            static_cast<u4>(file_num)
        };
        
        int res = unzGoToFilePos(this->file, &pos);
        return this->HandleResult(res);
    }
    
    std::pair<pint_t, pint_t> ZipReader::GetFilePos()
    {
        unz_file_pos pos;
        std::pair<pint_t, pint_t> res;
        int err = unzGetFilePos(this->file, &pos);
        if (err == UNZ_OK) {
            res.first = pos.pos_in_zip_directory;
            res.second = pos.num_of_file;
        } else {
            res.first = res.second = -1;
        }
        return res;
    }
    
    bool ZipReader::FindFile(String* name, bool caseSensitive)
    {
        int res = unzLocateFile(
            this->file,
            name->GetBuffer(),
            caseSensitive ? 1 : 2
        );
        return this->HandleResult(res);
    }
    
    bool ZipReader::HandleResult(int res)
    {
        if (res == UNZ_OK)
        {
            this->SetCurrentFile();
            return true;
        }
        else if (res == UNZ_END_OF_LIST_OF_FILE)
        {
            this->ClearCurrentFile();
        }
        return false;
    }
        
    void ZipReader::ClearCurrentFile()
    {
        this->currentFile = 0;
    }
    
    ZipReaderFile* ZipReader::GetCurrentFile()
    {
        return this->currentFile;
    }
    
    void ZipReader::SetCurrentFile()
    {
        if (!this->file)
        {
            this->ClearCurrentFile();
            return;
        }
        
        unz_file_pos pos;
        int res = unzGetFilePos(this->file, &pos);
        
        if (res != UNZ_OK)
        {
            this->ClearCurrentFile();
            return;
        }
        
        Value key((pint_t)pos.num_of_file);
        Value val(NULL_VALUE);
        
        if (files.Get(key, val))
        {
            this->currentFile = (ZipReaderFile*)val.val.object;
        }
        else
        {
            ZipReaderFile* zrf = ZipReaderFile::StaticNew(engine, engine->GetTypeFor(ZipReaderFile::StaticGetClass()), this);
                    
            res = unzGetCurrentFileInfo(
                this->file,
                &zrf->file_info,
                0, 0, // filename
                0, 0, // extra
                0, 0  // comment
            );
                    
            size_t fileNameSize = zrf->file_info.size_filename;
            engine->string_buff.Resize(fileNameSize);
        
            res = unzGetCurrentFileInfo(
                this->file,
                0,
                engine->string_buff.GetAt(0),
                engine->string_buff.GetSize(),
                0, 0,
                0, 0
            );
            
            if (res == UNZ_OK) {
                zrf->SetName(engine->GetString(engine->string_buff.GetAt(0), 
                    engine->string_buff.GetSize()));
            }
            val.Set(zrf); 
            this->files.Set(key, val);
            WriteBarrier(zrf);
            this->currentFile = zrf;
        }
    }
    
    PIKA_IMPL(ZipReaderError)
    PIKA_IMPL(ZipReader)
    PIKA_IMPL(ZipReaderFile)
        
    int ZipReader_findFile(Context* ctx, Value& self)
    {
        ZipReader* z = static_cast<ZipReader*>(self.val.object);
        u2 argc = ctx->GetArgCount();
        String* name = 0;
        bool cs = false;
        
        switch(argc) {
        case 2:
            cs = ctx->GetBoolArg(1);
        case 1:
            name = ctx->GetStringArg(0);
            break;
        default:
            ctx->WrongArgCount();
        }
        ctx->PushBool(z->FindFile(name, cs));
        return 1;
    }
}// pika

using namespace pika;

void Init_ZipReader(Engine* engine, Package* zipfile)
{
    String* ZipReader_String = engine->GetString("ZipReader");
    Type*   ZipReader_Type   = Type::Create(engine, ZipReader_String, engine->Object_Type, ZipReader::Constructor, zipfile);
    
    String* ZipReaderFile_String = engine->GetString("ZipReaderFile");
    Type*   ZipReaderFile_Type   = Type::Create(engine, ZipReaderFile_String, engine->Object_Type, 0, zipfile);
    
    ZipReaderFile_Type->SetAbstract(true);
    ZipReaderFile_Type->SetFinal(true);
    
    String* ZipReaderError_String = engine->GetString("ZipReaderError");
    Type*   ZipReaderError_Type   = Type::Create(engine, ZipReaderError_String, engine->RuntimeError_Type, 0, zipfile);
    
    SlotBinder<ZipReader>(engine, ZipReader_Type)
    .Method(&ZipReader::Open, "open")
    .Method(&ZipReader::Close, "close")
    .Method(&ZipReader::FirstFile, "firstFile")
    .Method(&ZipReader::NextFile, "nextFile")
    .Method(&ZipReader::GetFilePos, "getFilePos")
    .Method(&ZipReader::SetFilePos, "setFilePos")
    .RegisterMethod(ZipReader_findFile, "findFile", 0, true, false)
    .Method(&ZipReader::ReadAllCurrentFile, "readAllCurrentFile")
    .Method(&ZipReader::ReadCurrentFile, "readCurrentFile")
    .Method(&ZipReader::OpenCurrentFile, "openCurrentFile")
    .Method(&ZipReader::CloseCurrentFile, "closeCurrentFile")
    .PropertyR("currentFile",
        &ZipReader::GetCurrentFile, "getCurrentFile")
    .PropertyR("fileCount",
        &ZipReader::GetFileCount, "getFileCount")
    .PropertyR("fileName",
        &ZipReader::GetFileName, "getFileName")
    .Alias("onDispose", "close")
    ;
        
    SlotBinder<ZipReaderFile>(engine, ZipReaderFile_Type)
    .PropertyR("fileName",
        &ZipReaderFile::GetFileName, "getFileName")
    .PropertyR("fileSize",
        &ZipReaderFile::GetFileSize, "getFileSize")
    .PropertyR("compressedSize",
        &ZipReaderFile::GetCompressedSize, "getCompressedSize")
    .PropertyR("reader",
        &ZipReaderFile::GetReader, "getReader")
    ;
    
    engine->SetTypeFor(ZipReaderError::StaticGetClass(), ZipReaderError_Type);
    engine->SetTypeFor(ZipReader::StaticGetClass(),      ZipReader_Type);
    engine->SetTypeFor(ZipReaderFile::StaticGetClass(),  ZipReaderFile_Type);
    
    zipfile->SetSlot(ZipReader_String,      ZipReader_Type);
    zipfile->SetSlot(ZipReaderFile_String,  ZipReaderFile_Type);
    zipfile->SetSlot(ZipReaderError_String, ZipReaderError_Type);
}
