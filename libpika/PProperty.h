/*
 *  PProperty.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_PROPERTY_HEADER
#define PIKA_PROPERTY_HEADER

namespace pika {
/** Properties provide access to class members through the use of getter and setter functions.
  * 
  * A property may we Read-Write, Read only or Write only. When written to or read from the
  * accessor function is called automagically without the users intervention, the property will
  * then appear to be a normal field.
  * 
  * Properties will always be slower than direct access but can be useful if certian
  * operations need to performed on the value before it is set or retrieved.
  */
class PIKA_API Property : public Basic
{
    PIKA_DECL(Property, Basic)
public:
    Property(Engine* eng, String* name, Function* getter, Function* setter);

    static Property* CreateReadWrite(Engine* eng, String* name, Function* getter, Function* setter);
    static Property* CreateRead(Engine* eng, String* name, Function* getter);
    static Property* CreateWrite(Engine* eng, String* name, Function* setter);
    
    virtual ~Property();

    virtual void  MarkRefs(Collector* c);
    virtual Type* GetType() const;
    virtual bool  GetSlot(const Value& key, Value& result);

    virtual bool  CanSet();
    virtual bool  CanGet();

    virtual Function* GetGetter();
    virtual Function* GetSetter();
    virtual String*   GetName();

protected:
    Function* getter;
    Function* setter;
    String*   name;
};

}// pika

#endif
