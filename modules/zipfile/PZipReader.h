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
        
        size_t      GetFileSize();
        size_t      GetCompressedSize();
        String*     GetFileName();
        ZipReader*  GetReader();
        
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
        
        /** Returns the total number of files in the central directory. 
          * 
          * Zip files may contain undocumented entries that are deleted
          * but their file data remains. In practice you won't run into
          * any trouble with these deleted files, since they are skipped
          * by the reader.
          */
        pint_t GetFileCount();
        
        /** Go to the first file in this zip archive. Returns true if successful. */
        bool FirstFile();

        /** Go to the first file in this zip archive. 
          * Returns true if successful. False is there are no more files. 
          */
        bool NextFile();

        /** Go to the first file in this zip archive. Returns true if successful. */
        bool FindFile(String* name, bool caseSensitive);
        
        /** Sets the file pos to the given byte in the zip archive and the to the given file number. */
        bool SetFilePos(pint_t nbyte, pint_t nfile);
        
        /** Returns the current position the zip archive and the current file number. */
        std::pair<pint_t, pint_t> GetFilePos();
        
        /** Opens the current file. Returns true if successful. */
        bool OpenCurrentFile();
        
        /** Opens the current file with the given password. Returns true if successful. */
        bool OpenCurrentFilePassword(String*);
        
        /** Returns the current file information via the ZipReaderFile class or 
          * null if no file is selected. */
        ZipReaderFile* GetCurrentFile();
        
        /** Closes the current file for reading. */
        void CloseCurrentFile();
        
        /** Reads the entire current file. */
        String* ReadAllCurrentFile();

        /** Reads the given number of bytes from the file. 
          * If -1 is provided then the entire file is read.
          */
        String* ReadCurrentFile(pint_t);
        
        String* GetFileName();
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
