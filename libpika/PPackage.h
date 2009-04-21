/*
 *  PPackage.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_PACKAGE_HEADER
#define PIKA_PACKAGE_HEADER

#ifndef PIKA_BUFFER_HEADER
#   include "PBuffer.h"
#endif

#ifndef PIKA_OBJECT_HEADER
#   include "PObject.h"
#endif

namespace pika {

///////////////////////////////////////////// Package //////////////////////////////////////////////

class PIKA_API Package : public Object 
{
    PIKA_DECL(Package, Object)
protected:    
    Package(Engine*, Type* type, String* name, Package* super);
    Package(const Package* rhs) : ThisSuper(rhs), name(rhs->name), dotName(rhs->dotName), superPackage(rhs->superPackage) {}
public:
    virtual ~Package();
    
#   ifndef PIKA_BORLANDC
    using Basic::GetSlot;
    using Basic::SetSlot;
#   endif
    
    static Package* Create(Engine* eng, String* name, Package* super = 0);
    
    virtual void    Init(Context*);
    Package*        GetSuper();
    virtual String* GetName();
    virtual String* GetDotName();
    virtual String* ToString();
    virtual void    SetName(String*);
    virtual void    MarkRefs(Collector*);
    virtual Object* Clone();
    virtual bool    GetGlobal(const Value& key, Value& res);
    virtual bool    SetGlobal(const Value& key, Value& value, u4 attr = 0);
    virtual bool    GetSlot(const Value& key, Value& res);
        
    void AddNative(RegisterFunction* fns, size_t count);
protected:
    String*  name;
    String*  dotName;
    Package* superPackage;
};// Package

}// pika

#endif
