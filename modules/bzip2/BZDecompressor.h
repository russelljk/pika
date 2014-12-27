/*
 *  BZDecompressor.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_BZINFLATER_HEADER
#define PIKA_BZINFLATER_HEADER

namespace pika {
    struct Decompressor: BZStream {
        PIKA_DECL(Decompressor, BZStream)
    public:
        Decompressor(Engine*, Type*);
        
        virtual ~Decompressor();
        
        static void          Constructor(Engine* eng, Type* obj_type, Value& res);
        static Decompressor* StaticNew(Engine* eng, Type* type);
    protected:
        virtual ClassInfo*  GetErrorClass();
        virtual void        Begin();
        virtual int         Call(int);
        virtual void        End();
    };
}// pika

#endif
