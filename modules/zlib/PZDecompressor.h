/*
 *  PZDecompressor.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_ZINFLATER_HEADER
#define PIKA_ZINFLATER_HEADER

namespace pika {
    struct Decompressor: ZStream {
        PIKA_DECL(Decompressor, ZStream)
    public:
        Decompressor(Engine*, Type*);
        
        virtual ~Decompressor();
        
        static void          Constructor(Engine* eng, Type* obj_type, Value& res);
        static Decompressor* StaticNew(Engine* eng, Type* type);
    protected:
        virtual ClassInfo*  GetErrorClass();
        virtual void        Begin();
        virtual int         Call(int flush);
        virtual void        End();
    };
}// pika

#endif
