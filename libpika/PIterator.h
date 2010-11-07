
#ifndef PITERATOR_HEADER
#define PITERATOR_HEADER

namespace pika {

enum IterateKind {
    IK_values,
    IK_keys,
    IK_both,
    IK_default,
};

class PIKA_API Iterator : public Object
{
    PIKA_DECL(Iterator, Object)
protected:
    Iterator(Engine*, Type*);
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
