#ifndef PIKA_SOCKETADDRESS_HEADER
#define PIKA_SOCKETADDRESS_HEADER

namespace pika {

struct SocketAddress : Object {
    PIKA_DECL(SocketAddress, Object)
public:
    SocketAddress(Engine*, Type*, Pika_address* addr = 0);
    virtual ~SocketAddress();
    
    virtual void Init(Context*);
    
    static void Constructor(Engine* eng, Type* obj_type, Value& res);
    static SocketAddress* StaticNew(Engine* eng, Type* type, Pika_address* addr = 0);
    static Type* StaticGetType(Engine* eng);
    pint_t GetPort();
    
    String* GetAddressString();
    Pika_address* GetAddress();
protected:
    Pika_address* addr;
};

}// pika

#endif
