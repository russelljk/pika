/*
 *  PProperty.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_PROPERTY_HEADER
#define PIKA_PROPERTY_HEADER

namespace pika {
/** Properties provide access to class members through the use of getter and setter functions.
  * 
  * A property may we Read-Write, Read only or Write only. When written to / read from the
  * accessor function is auto-magically called without the users intervention, the property will
  * then appear as a normal field.
  * 
  * Properties will always be slower than direct access but can be useful under certain
  * conditions.
  *
  * TODO { Property should really be an Object, like Function. However, this should
  *        only happen if we restructure the heirarchy to optionally support instance variables. 
  *        Since properties cannot be modified directly there is little reason for
  *        them to have a Table of instance variables 99.9% of the time. }
  */
class PIKA_API Property : public Basic
{
    PIKA_DECL(Property, Basic)
public:
    Property(Engine* eng, String* name, Function* getter, Function* setter, String* doc);
    
    virtual ~Property();
    
    static Property* CreateReadWrite(Engine* eng, String* name, Function* getter, Function* setter, const char* doc = 0);
    static Property* CreateRead     (Engine* eng, String* name, Function* getter, const char* doc = 0);
    static Property* CreateWrite    (Engine* eng, String* name, Function* setter, const char* doc = 0);
    
    virtual void  MarkRefs(Collector* c);
    virtual Type* GetType() const;
    virtual bool  GetSlot(const Value& key, Value& result);
    virtual Value ToValue();
    virtual bool  CanWrite();
    virtual bool  CanRead();
    
    virtual Function* Reader();
    virtual Function* Writer();
    virtual String*   Name();
    virtual String*   GetDoc();
    virtual void      SetDoc(String*);
    virtual void      SetDoc(const char*);
    virtual void SetWriter(Function* s);
    virtual void SetRead(Function* g);    
protected:
    Function* getter;
    Function* setter;
    String*   name;
    String*   __doc;
};

}// pika

#endif
