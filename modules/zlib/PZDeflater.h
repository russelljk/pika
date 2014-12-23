/*
 *  PZDeflater.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_ZDEFLATER_HEADER
#define PIKA_ZDEFLATER_HEADER

namespace pika {
    struct Deflater: ZStream {
        PIKA_DECL(Deflater, ZStream)
    public:
        Deflater(Engine*, Type*);
        
        virtual ~Deflater();
        
        static void      Constructor(Engine* eng, Type* obj_type, Value& res);
        static Deflater* StaticNew(Engine* eng, Type* type);
    };
}// pika

#endif
