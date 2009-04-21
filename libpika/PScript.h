/*
 * PScript.h
 * See Copyright Notice in Pika.h
 */
#ifndef PIKA_SCRIPT_HEADER
#define PIKA_SCRIPT_HEADER

#ifndef PIKA_LITERALPOOL_HEADER
#   include "PLiteralPool.h"
#endif

namespace pika {
// Script

class PIKA_API Script : public Package 
{
    PIKA_DECL(Script, Package)
protected:
    friend class Engine;

    Script(Engine*, Type*, String*, Package*);

    void            Initialize(LiteralPool*, Context*, Function*);

    LiteralPool*    literals;
    Context*        context;
    Function*       entryPoint;
    Array*          arguments;
    bool            firstRun;
    bool            running;
public:
    virtual         ~Script();
    virtual Context* GetContext() { return context; }
    virtual void     MarkRefs(Collector* c);
    virtual bool     Run(Array* arguments);

    static Script*  Create(Engine* eng, String* name, Package* pkg);
};

}// pika

#endif
