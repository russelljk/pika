/*
 *  PLocalsObject.h
 *  See Copyright Notice in Pika.h
 */ 
#ifndef PIKA_LOCALSOBJECT_HEADER
#define PIKA_LOCALSOBJECT_HEADER

namespace pika 
{
/*  LocalsObject
 *
 *  Provides script access to local variables of a given function and its parents at a given position in its bytecode.
 *
 *  Because of the nature of function closures, outer-variables are accessible only when the 
 *  inner-function uses one or more outer-variables from a parent-function.
 */
class PIKA_API LocalsObject : public Object 
{
    PIKA_DECL(LocalsObject, Object)
protected:
    friend class LocalsObjectEnumerator;
    
    LocalsObject(Engine* eng, Type* obj_type, Function* func, LexicalEnv* env, ptrdiff_t p)
        : Object(eng, obj_type), 
        lexEnv(env),
        function(func),            
        parent(0), 
        pos(p)            
    {
        BuildIndices();
    }
    
    void BuildIndices();
public:
    virtual ~LocalsObject() {}

#   ifndef PIKA_BORLANDC
    using Basic::GetSlot;
    using Basic::SetSlot;
#   endif

    virtual void        MarkRefs(Collector* c);
    virtual Enumerator* GetEnumerator(String*);

    // read or write a local-variable by name.
    virtual bool GetSlot(const Value& key, Value& result);
    virtual bool SetSlot(const Value& key, Value& value, u4 attr = 0);
    virtual bool HasSlot(const Value& key);
    virtual LocalsObject* GetParent();

    static LocalsObject* Create(Engine*, Type*, Function*, LexicalEnv*, ptrdiff_t);    
private:
    Table         indices;  // Local variable lookup table.
    LexicalEnv*   lexEnv;   // Function's lexical environment.
    Function*     function; // Function whose lexEnv we represent.
    LocalsObject* parent;   // Parent's locals (Lazily created).
    ptrdiff_t     pos;      // Bytecode position that locals is valid for.
};

}// pika

#endif
