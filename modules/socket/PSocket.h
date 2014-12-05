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
    
    virtual pint_t  Fileno();
    virtual void    SetSockOpt(pint_t opt, Value val);
    virtual Value   GetSockOpt(pint_t opt);
    virtual bool    Accept(Socket**, SocketAddress**);
    virtual void    Bind(Value& addr);
    virtual void    Connect(Value& addr);
    virtual void    Listen(int);
    virtual void    Close();
    virtual void    Shutdown(int);
    virtual void    Send(String*);
    virtual void    SendTo(String*, Value&);    
    virtual String* Recv(pint_t);
    virtual String* RecvFrom(pint_t, SocketAddress** fromAddr);
    
    static void     Constructor(Engine* eng, Type* obj_type, Value& res);
    static Socket*  StaticNew(Engine* eng, Type* type);
};

}// pika

#endif
