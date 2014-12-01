#ifndef PIKA_SOCKET_HEADER
#define PIKA_SOCKET_HEADER

namespace pika {

struct Socket : Object
{
    PIKA_DECL(Socket, Object)

protected:    
    Pika_socket* socket;
    
public:
    Socket(Engine* engine, Type* type);
    virtual ~Socket();
    
    virtual void Init(Context* ctx);
    
    void            Connect(String* address);
    static void     Constructor(Engine* eng, Type* obj_type, Value& res);
    static Socket*  StaticNew(Engine* eng, Type* type);
};

}// pika

#endif
