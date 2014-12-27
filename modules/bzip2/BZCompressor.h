/*
 *  BZCompressor.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_BZCOMPRESSOR_HEADER
#define PIKA_BZCOMPRESSOR_HEADER

namespace pika {

struct Compressor: BZStream {
    PIKA_DECL(Compressor, BZStream)
public:
    Compressor(Engine*, Type*);
    
    virtual ~Compressor();

    static void         Constructor(Engine* eng, Type* obj_type, Value& res);
    static Compressor*  StaticNew(Engine* eng, Type* type);
    
    pint_t  GetLevel();
    void    SetLevel(pint_t);
protected:
    virtual ClassInfo*  GetErrorClass();
    virtual void        Begin();
    virtual int         Call(int flush);
    virtual void        End();
    
    pint_t level;
};

}// pika

#endif
