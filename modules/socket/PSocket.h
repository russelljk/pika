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
    
    void            SetSockOpt(pint_t opt, Value val);
    Value           GetSockOpt(pint_t opt);
    bool            Accept(Socket**, SocketAddress**);
    void            Bind(Value& addr);
    void            Connect(Value& addr);
    void            Listen(int);
    void            Close();
    void            Shutdown(int);
    void            Send(String*);
    void            SendTo(String*, Value&);    
    String*         Recv(pint_t);
    String*         RecvFrom(pint_t, SocketAddress** fromAddr);
    static void     Constructor(Engine* eng, Type* obj_type, Value& res);
    static Socket*  StaticNew(Engine* eng, Type* type);
};

}// pika

#endif
