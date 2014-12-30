/*
 *  PZipWriter.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_ZIPWRITER_HEADER
#define PIKA_ZIPWRITER_HEADER

namespace pika {
    struct ZipWriter;
    struct ZipWriterError;
        
    struct ZipWriterError: RuntimeError
    {
        PIKA_DECL(ZipWriterError, RuntimeError)
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
    struct ZipWriter : Object
    {
        PIKA_DECL(ZipWriter, Object)
    public:
        ZipWriter(Engine* engine, Type* type);
        virtual ~ZipWriter();
        
        virtual void MarkRefs(Collector*);
        
        virtual void Open(String* name, pint_t append);        
        virtual void Close();
        virtual bool _OpenFileLong(
            String* name,
            pint_t year, pint_t month, pint_t day,
            pint_t hour, pint_t minute, pint_t second,
            String* extra,
            String* globalExtra,
            String* comment,
            pint_t method,
            pint_t level);
        
        static void Constructor(Engine* eng, Type* obj_type, Value& res);
        static ZipWriter* StaticNew(Engine* eng, Type* type);
        
        bool WriteToFile(String*);
                
        /** Closes the current file for reading. */
        void CloseFile();
                
        String* GetFileName();
             
        String* GetGlobalComment();    
        void SetGlobalComment(String* c);
    protected:
        /** Sets or clears the currentFile based on the result of a method call. */
        bool HandleResult(int res);
        
        zipFile file;        //!< Handle used by minizip for our zip file.
        String* filename;    //!< Name of the zip file we have opened.
        String* globalComment;
    };
    
    DECLARE_BINDING(ZipWriter);
}// pika

#endif
