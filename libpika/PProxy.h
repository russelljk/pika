#ifndef PPROXY_HEADER
#define PPROXY_HEADER

namespace pika {

class Proxy : public Object
{
    PIKA_DECL(Proxy, Object)
public:
    Proxy(Engine* eng, Type* type);    
    virtual ~Proxy();    
    
    virtual void Init(Context* ctx);        
    virtual void MarkRefs(Collector* c);    
    
    Function* Attach(Function* fn);    
    
    void SetReader(Function* fn, bool attach=false);
    Function* Reader();
    bool IsRead();
    
    void SetWriter(Function* fn, bool attach=false);
    Function* Writer();
    bool IsWrite();
    
    bool IsReadWrite();
    
    Value Obj();
    Value Name();
    
    static void Constructor(Engine* eng, Type* type, Value& res);
    static void StaticInitType(Engine* eng);
private:
    Value object;
    Value name;
    Property* property;
};

}// pika

#endif
