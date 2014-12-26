/*
 *  PZStream.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_ZSTREAM_HEADER
#define PIKA_ZSTREAM_HEADER

namespace pika {
    struct ZStream: Object {
        PIKA_DECL(ZStream, Object)
    public:
        ZStream(Engine*, Type*);
        
        virtual ~ZStream();
        
        virtual String*     Process(String* in);
    protected:
        virtual ClassInfo*  GetErrorClass();
        virtual void        Begin();
        virtual int         Call(int flush);
        virtual void        End();
        
        virtual void        DoProcess(const u1* in, size_t in_length, Buffer<u1>& out);
        virtual void        Reset();
        z_stream stream;
    };
}// pika

#endif
