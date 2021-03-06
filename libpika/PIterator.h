/*
 *  PIterator.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_ITERATOR_HEADER
#define PIKA_ITERATOR_HEADER

namespace pika {

enum IterateKind {
    IK_values,
    IK_keys,
    IK_both,
    IK_default,
};

class PIKA_API IterateHelper 
{
public:
    IterateHelper(Iterator* iter, Context* ctx);    
    ~IterateHelper();
    
    operator bool();
    Value Next();
    
    Iterator* iterator;
    Context* context;
};

class PIKA_API Iterator : public Object
{
    PIKA_DECL(Iterator, Object)
protected:
    Iterator(Engine*, Type*);
    Iterator(Iterator*);
public:
    virtual ~Iterator();
    
    virtual bool ToBoolean();
    virtual int Next(Context*);
    
    static Iterator* Create(Engine* eng, Type* typ);
    static void Constructor(Engine* eng, Type* type, Value& res);
    static void StaticInitType(Engine*);
};

}// pika

#endif
