/*
 *  BZStream.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_BZSTREAM_HEADER
#define PIKA_BZSTREAM_HEADER

namespace pika {
    struct BZStream: Object {
        PIKA_DECL(BZStream, Object)
    public:
        BZStream(Engine*, Type*);
        
        virtual ~BZStream();
        
        virtual String*     Process(String* in);
    protected:
        virtual ClassInfo*  GetErrorClass();
        virtual void        Begin();
        virtual int         Call(int flush);
        virtual void        End();
        
        virtual void        DoProcess(const char* in, size_t in_length, Buffer<char>& out);
        virtual void        Reset();
        bz_stream stream;
    };
}// pika

#endif
