/*
 *  PZInflater.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_ZINFLATER_HEADER
#define PIKA_ZINFLATER_HEADER

namespace pika {
    struct Inflater: ZStream {
        PIKA_DECL(Inflater, ZStream)
    public:
        Inflater(Engine*, Type*);
        
        virtual ~Inflater();
        
        static void      Constructor(Engine* eng, Type* obj_type, Value& res);
        static Inflater* StaticNew(Engine* eng, Type* type);
    };
}// pika

#endif
