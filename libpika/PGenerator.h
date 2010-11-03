/*
 * PGenerator.h
 * See Copyright Notice in Pika.h
 */
#ifndef PIKA_GENERATOR_HEADER
#define PIKA_GENERATOR_HEADER

#ifndef PIKA_CONTEXT_HEADER
#   include "PContext.h"
#endif

namespace pika {

enum GenState {
    GS_clean,
    GS_yielded,
    GS_resumed,
    GS_finished,
};

class PIKA_API Generator : public Object
{
    PIKA_DECL(Generator, Object)
public:
    Generator(Engine* eng, Type* typ, Function* fn);
    
    virtual ~Generator();
    virtual void Init(Context*);
    virtual bool ToBoolean();
    INLINE GenState GetState() { return state; }
    virtual void MarkRefs(Collector*);
    virtual Enumerator* GetEnumerator(String* kind);
    
    /** Returns true if this Generator is yieled and can be resumed. */
    bool IsYielded();

    /** Yields the context's current scope.
      * @param ctx The Context whose current scope will be yielded.    
      * @warning You must ensure that the Context's current closure is the same
      *          as this Generator's function.
      */    
    void Yield(Context*);

    /** Resume this generator, must be yielded. 
      * @param ctx The Context to resume with.
      * @param retc The exception number of return or yield values.
      */
    void Resume(Context* ctx, u4 retc);
    
    /** Tell the Generator that it is finished. */
    void Return();
    
    static void Constructor(Engine* eng, Type* type, Value& res);
    static Generator* Create(Engine* eng, Type* type, Function* function);
    static void StaticInitType(Engine* eng);
protected:
    size_t FindLastCallScope(Context*, ScopeIter);
    size_t FindBaseHandler(Context* ctx, size_t scopeid);
    
    size_t         scopetop;
    GenState       state;
    Function*      function;
    ExceptionStack handlers;
    ScopeStack     scopes;
    Buffer<Value>  stack;
};

}// pika

#endif
