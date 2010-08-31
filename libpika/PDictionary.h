/*
 *  PDictionary.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_DICTIONARY_HEADER
#define PIKA_DICTIONARY_HEADER

namespace pika {
    class PIKA_API Dictionary : public Object
    {
        PIKA_DECL(Dictionary, Object)
    public:
        Dictionary(Engine*, Type*);
        virtual ~Dictionary();
        virtual void MarkRefs(Collector* c);
        
        virtual bool BracketRead(const Value& key, Value& res);
        virtual bool BracketWrite(const Value& key, Value& value, u4 attr=0);
        
        Array* Keys();
        Array* Values();
        
        static void Constructor(Engine* eng, Type* type, Value& res);
        static void StaticInitType(Engine*);
    protected:
        Table elements;
    };
}// pika
#endif
