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
//
// Script
//
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
    Package*        import_value;
    bool            firstRun;
    bool            running;    
public:
    static Script*  CreateWithBuffer(Engine*, String*, String*, Package*);
public:
    Package*        GetImportResult() { return import_value ? import_value : this; }
    void            SetImportResult(Package* impres);
    virtual         ~Script();
    virtual Context* GetContext() { return context; }
    virtual void     MarkRefs(Collector* c);
    virtual bool     Run(Array* arguments);
    
    static Script*  Create(Engine* eng, String* name, Package* pkg);
};// Script

}// pika

#endif
