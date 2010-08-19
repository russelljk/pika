/*
 *  PFile.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_FILE_HEADER
#define PIKA_FILE_HEADER

namespace pika {

/* Simple file class based on C file-streams. */

class PIKA_API File : public Object
{
    PIKA_DECL(File, Object)
public:
    File(Engine*, Type*);

    virtual ~File();

    virtual void MarkRefs(Collector*);
    virtual bool ToBoolean();

    virtual String* GetFilename();
    virtual void    Init(Context*);

    virtual bool Open(String*, String*);
    virtual void Close();

    virtual bool    IsEof();
    virtual bool    IsOpen() const;
    virtual Object* Clone();
    virtual String* ReadLine();
    virtual Array*  ReadLines();
    virtual String* Read(Context* ctx);

    virtual bool    Write(Context* ctx);

    virtual pint_t      Seek(pint_t, pint_t);
    virtual void        Flush();
    virtual void        Rewind();
    virtual pint_t      SetPos(pint_t);
    virtual pint_t      Advance(pint_t);
    
    bool GetUsePaths() const;    
    void SetUsePaths(bool use);
    
    void SetFilename(String*);
    void SetHandle(FILE*);
    
    static File* Create(Engine*, Type*);
    static void  Constructor(Engine* eng, Type* obj_type, Value& res);
    static void  StaticInitType(Engine* eng);
private:
    String* filename;
    FILE*   handle;
    bool use_paths;
};

}// pika

#endif
