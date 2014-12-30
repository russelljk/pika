/*
 *  PZipWriter.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PZipFile.h"
#include "PZipWriter.h"
#include "ZipWriter_OpenFile.h"

namespace pika {
    
    pint_t ReadIntAttrFrom(Context* ctx, Object* obj, const char* attr, bool fail=true)
    {
        Value res(NULL_VALUE);
        if (obj->GetAttr(ctx, attr, res) && res.IsInteger()) {
            return res.val.integer;
        } else if (fail) {
            RaiseException("Attempt to read attribute %s failed.", attr);
        }
        return 0;
    }
    
    ZipWriter::ZipWriter(Engine* engine, Type* type) : 
        ThisSuper(engine, type),
        file(0),
        filename(engine->emptyString),
        globalComment(0) 
    {
    }
    
    ZipWriter::~ZipWriter()
    {
        Close();
    }
    
    String* ZipWriter::GetFileName()
    {
        return this->filename;
    }
    
    bool ZipWriter::WriteToFile(String* buff)
    {
        if (zipWriteInFileInZip(this->file, buff->GetBuffer(), buff->GetLength()) == ZIP_OK)
        {
            return true;
        }
        return false;
    }
        
    bool ZipWriter::_OpenFileLong(
        String* name,
        pint_t year, pint_t month, pint_t day,
        pint_t hour, pint_t minute, pint_t second,
        String* extra,
        String* globalExtra,
        String* comment,
        pint_t method,
        pint_t level)
    {        
        // File Info
        zip_fileinfo info;
        Pika_memzero(&info, sizeof(zip_fileinfo));
        info.tmz_date.tm_year = year;
        info.tmz_date.tm_mon = month;
        info.tmz_date.tm_mday = day;
        info.tmz_date.tm_hour = hour;
        info.tmz_date.tm_min = minute;
        info.tmz_date.tm_sec = second;
        
        const char* extra_ptr = 0;
        size_t extra_sz = 0;
        
        if (extra) {
            extra_ptr = extra->GetBuffer();
            extra_sz = extra->GetLength();
        }
        
        const char* gloablExtra_ptr = 0;
        size_t globalExtra_sz = 0;
        
        if (globalExtra) {
            gloablExtra_ptr = globalExtra->GetBuffer();
            globalExtra_sz = globalExtra->GetLength();
        }
        
        if (zipOpenNewFileInZip(this->file,
            name->GetBuffer(),
            &info,
            extra_ptr, extra_sz,
            gloablExtra_ptr, globalExtra_sz,
            comment ? comment->GetBuffer() : 0,
            method,
            level) == ZIP_OK)
        {
            return true;
        }
        return false;
    }
    
    void ZipWriter::CloseFile()
    {
        zipCloseFileInZip(this->file);
    }
    
    void ZipWriter::Close()
    {
        this->CloseFile();
                
        if (file)
        {
            zipClose(file, this->globalComment ? globalComment->GetBuffer() : 0);
            file = 0;
        }
    }
    
    String* ZipWriter::GetGlobalComment()
    {
        return this->globalComment;
    }
    
    void ZipWriter::SetGlobalComment(String* c)
    {
        WriteBarrier(c);
        this->globalComment = c;
    }
    
    void ZipWriter::MarkRefs(Collector* c)
    {
        ThisSuper::MarkRefs(c);
        
        if (this->filename)
        {
            this->filename->Mark(c);
        }
        
        if (this->globalComment)
        {
            this->globalComment->Mark(c);
        }
    }
        
    void ZipWriter::Open(String* fn, pint_t append)
    {
        if (this->file)
        {
            RaiseException(
                ZipWriterError::StaticGetClass(),
                "Cannot open an already opened ZipWriter."
            );            
        }
        
        this->file = zipOpen(fn->GetBuffer(), append);
        
        if (!this->file)
        {
            RaiseException(
                ZipWriterError::StaticGetClass(),
                "Attempt to open zip file failed."
            );
        }
        filename = fn;
    }
    
    ZipWriter* ZipWriter::StaticNew(Engine* eng, Type* type)
    {
        ZipWriter* obj = 0;
        GCNEW(eng, ZipWriter, obj, (eng, type));
        return obj;
    }

    void ZipWriter::Constructor(Engine* eng, Type* obj_type, Value& res)
    {
        Object* obj = ZipWriter::StaticNew(eng, obj_type);
        res.Set(obj);
    }
    
    bool ZipWriter::HandleResult(int res)
    {
        if (res == ZIP_OK)
        {
            return true;
        }
        return false;
    }
    
    PIKA_IMPL(ZipWriterError)
    PIKA_IMPL(ZipWriter)

}// pika

using namespace pika;

void Init_ZipWriter(Engine* engine, Package* zipfile)
{
    String* ZipWriter_String = engine->GetString("ZipWriter");
    Type*   ZipWriter_Type   = Type::Create(engine, ZipWriter_String, engine->Object_Type, ZipWriter::Constructor, zipfile);
        
    String* ZipWriterError_String = engine->GetString("ZipWriterError");
    Type*   ZipWriterError_Type   = Type::Create(engine, ZipWriterError_String, engine->RuntimeError_Type, 0, zipfile);
    
    SlotBinder<ZipWriter>(engine, ZipWriter_Type)
    .Method(&ZipWriter::Open,           "open")
    .Method(&ZipWriter::Close,          "close")
    .Method(&ZipWriter::WriteToFile,    "writeToFile")
    .Method(&ZipWriter::_OpenFileLong,  "_openFileLong")
    .Method(&ZipWriter::CloseFile,      "closeFile")
    .PropertyRW("globalComment",
        &ZipWriter::GetGlobalComment, "getGlobalComment",
        &ZipWriter::SetGlobalComment, "setGlobalComment")
    .PropertyR("fileName",
        &ZipWriter::GetFileName, "getFileName")
    .Alias("onDispose", "close")
    ;
    
    static NamedConstant Append_Constants[] = {
        { "APPEND_STATUS_CREATE",       APPEND_STATUS_CREATE      },
        { "APPEND_STATUS_CREATEAFTER",  APPEND_STATUS_CREATEAFTER },
        { "APPEND_STATUS_ADDINZIP",     APPEND_STATUS_ADDINZIP    },
    };
    
    Basic::EnterConstants(zipfile, Append_Constants, countof(Append_Constants));
    
    engine->SetTypeFor(ZipWriterError::StaticGetClass(), ZipWriterError_Type);
    engine->SetTypeFor(ZipWriter::StaticGetClass(),      ZipWriter_Type);
    
    zipfile->SetSlot(ZipWriter_String,      ZipWriter_Type);
    zipfile->SetSlot(ZipWriterError_String, ZipWriterError_Type);
    
    engine->Exec(reinterpret_cast<const char*>(&ZipWriter_OpenFile_pika[0]), ZipWriter_OpenFile_pika_len, 0, zipfile);
}
