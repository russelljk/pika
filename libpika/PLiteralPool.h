/*
 *  PLiteralPool.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_LITERALPOOL_HEADER
#define PIKA_LITERALPOOL_HEADER

#ifndef PIKA_BUFFER_HEADER
#include "PBuffer.h"
#endif

namespace pika
{

#if defined(PIKA_DLL) && defined(_WIN32)
template class PIKA_API Buffer<Value>;
#endif

//////////////////////////////////////////// LiteralPool ///////////////////////////////////////////

class PIKA_API LiteralPool : public GCObject
{
    LiteralPool(Engine*);
public:
    virtual ~LiteralPool();

    static LiteralPool* Create(Engine* eng);

    virtual void    MarkRefs(Collector*);
    const Value&    Get(u2 idx) const;
    size_t          GetSize()   const;
    
    u2              Add(pint_t i);
    u2              Add(preal_t f);
    u2              Add(String* s);
    u2              Add(const Value& v);
private:
    Engine*         engine;
    Buffer<Value>   literals;
};

INLINE const Value& LiteralPool::Get(u2 idx) const { return literals[idx]; }
INLINE size_t       LiteralPool::GetSize()   const { return literals.GetSize(); }

}//pika

#endif
