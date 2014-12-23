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
        
    protected:
        z_stream stream;
    };
}// pika

#endif
