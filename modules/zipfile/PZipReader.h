/*
 *  PZipReader.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_ZIPREADER_HEADER
#define PIKA_ZIPREADER_HEADER

namespace pika {
    struct ZipReader;
    struct ZipReaderError;
    struct ZipReaderFile;
    
    struct ZipReaderError: RuntimeError
    {
        PIKA_DECL(ZipReaderError, RuntimeError)
    };
    
    struct ZipReaderFile: Object
    {
        PIKA_DECL(ZipReaderFile, Object)
    public:
        ZipReaderFile(Engine* engine, Type* type, ZipReader* zf);        
        virtual ~ZipReaderFile() {}
        
        void SetName(String*);
        virtual void MarkRefs(Collector*);
        static ZipReaderFile* StaticNew(Engine*, Type*, ZipReader*);
        
        size_t GetFileSize();
        size_t GetCompressedSize();
        String* GetFileName();
        ZipReader* GetReader() { return this->zipfile; }
        
        unz_file_info file_info;
        String* filename;
        ZipReader* zipfile;
    };
    
    /** Zip file reader class.
      *
      * We've mirrored the minizip API for the most part. 
      *
      * Information about the current file can be accessed by the currentFile member. 
      * All reading from the file is handled by this class. This is because the 
      * minizip library doesn't allow multiple opened files at the same time (unfortunately).
      *
      * Any methods operating on the current file have CurrentFile in thier name.
      *
      */
    struct ZipReader : Object
    {
        PIKA_DECL(ZipReader, Object)
    public:
        ZipReader(Engine* engine, Type* type);
        virtual ~ZipReader();
        
        virtual void MarkRefs(Collector*);
        
        virtual void Open(String*);        
        virtual void Close();
        
        static void Constructor(Engine* eng, Type* obj_type, Value& res);
        static ZipReader* StaticNew(Engine* eng, Type* type);
        
        pint_t GetFileCount();
        
        bool FirstFile();
        bool NextFile();
        bool FindFile(String* name, bool caseSensitive);
        
        bool SetFilePos(pint_t, pint_t);        
        std::pair<pint_t, pint_t> GetFilePos();
                
        bool OpenCurrentFile();
        bool OpenCurrentFilePassword(String*);
                
        ZipReaderFile* GetCurrentFile();        
        void CloseCurrentFile();
        String* ReadAllCurrentFile();
        String* ReadCurrentFile(pint_t);
        
    protected:
        /** Sets or clears the currentFile based on the result of a method call. */
        bool HandleResult(int res);

        /** Sets the currentFile based on the position in the opened zip file. */
        void SetCurrentFile();
        
        /** Clears the currentFile. */
        void ClearCurrentFile();
        ZipReaderFile*  currentFile; //!< The currently selected or opened file.
        Table           files;       //!< Table of all files we have come accross.
        String*         filename;    //!< Name of the zip file we have opened.
        unzFile         file;        //!< Handle used by minizip for our zip file.
        unz_global_info globalInfo;
    };
    
    DECLARE_BINDING(ZipReader);
    DECLARE_BINDING(ZipReaderFile);
}// pika

#endif
