/*
 *  PDictionary.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_DICTIONARY_HEADER
#define PIKA_DICTIONARY_HEADER

namespace pika {
    class PIKA_API Dictionary : public Object {
        PIKA_DECL(Dictionary, Object)
    public:
        Dictionary(Engine*, Type*);
        Dictionary(const Dictionary*);        
        
        virtual ~Dictionary();
        
        virtual void MarkRefs(Collector* c);
        virtual Object* Clone();
        
        virtual bool BracketRead(const Value& key, Value& res);
        virtual bool BracketWrite(const Value& key, Value& value, u4 attr=0);
        virtual Iterator* Iterate(String* kind);
        virtual String* ToString();
        
        Array* Keys();
        Array* Values();
        
        virtual bool HasSlot(const Value& key);
        static Dictionary* Create(Engine* eng, Type* type);
        static void Constructor(Engine* eng, Type* type, Value& res);
        static void StaticInitType(Engine*);
        size_t GetLength() const { return elements.Count(); }
        virtual bool ToBoolean();
        
        Table& Elements() { return elements; }
        const Table& Elements() const { return elements; }
    protected:
        Table elements;
    };
}// pika
#endif
