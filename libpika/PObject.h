/*
 * PObject.h
 * See Copyright Notice in Pika.h
 */
#ifndef PIKA_OBJECT_HEADER
#define PIKA_OBJECT_HEADER

#ifndef PIKA_TABLE_HEADER
#   include "PTable.h"
#endif

#ifndef PIKA_BASIC_HEADER
#   include "PBasic.h"
#endif

/////////////////////////////////////////////// pika ///////////////////////////////////////////////

namespace pika
{
/* Forward Declarations */
struct RegisterFunction;
struct RegisterProperty;
class Package;
class Array;
class Type;
class Object;

typedef void (*Type_NewFn)(Engine*, Type*, Value&);

////////////////////////////////////////////// Object //////////////////////////////////////////////

/** Base class for all Pika object's.
 *  Each object has an associated type and a slot-table where instance variables are kept.
 */
class PIKA_API Object : public Basic
{
    PIKA_DECL(Object, Basic)
public:
#   ifndef PIKA_BORLANDC
    using Basic::GetSlot;
    using Basic::SetSlot;
#   endif
    friend class ObjectEnumerator;
protected:
    INLINE Object(Engine* eng, Type* typeObj)
            : Basic(eng), type(typeObj)
    {}
    
    INLINE Object(const Object* rhs)
            : Basic(rhs->engine), type(rhs->type), members(rhs->members)
    {}
public:
    virtual ~Object();
    
    // Quick identification of tyoe commonly used in the VM (Faster than IsDerivedFrom and dynamic_cast).
    virtual bool  IsType()     const { return false; }
    virtual bool  IsFunction() const { return false; }
    virtual bool  IsArray()    const { return false; }
    
    virtual Type* GetType() const { return type; }
    virtual void  SetType(Type*);
    
    virtual void  AddFunction(Function*);
    virtual void  AddProperty(Property*);
    
    virtual bool  SetSlot(const Value& key, Value& value, u4 attr = 0);
    virtual bool  CanSetSlot(const Value& key);
    virtual bool  GetSlot(const Value& key, Value& result);
    virtual bool  HasSlot(const Value& key);
    virtual bool  DeleteSlot(const Value& key);
    
    virtual void  MarkRefs(Collector*);
    
    virtual void    CreateInstance(Value&);
    virtual Object* Clone();
    virtual String* ToString();
    virtual void    Init(Context*);
    
    virtual Enumerator* GetEnumerator(String*);
    
    /** Add native functions to an object.
     *
     *  @param rf       [in] Pointer to an array of type RegisterFunction.
     *  @param count    [in] Number of functions to add.
     *  @param pkg      [in] The package, module or script the functions live in. 
     */
    virtual void EnterFunctions(RegisterFunction* rf, size_t count, Package* pkg = 0);
    
    /** Create a new class Instance.
     *
     *  @param eng   [in] The Engine the object is created in.
     *  @param count [in] Type type of the object (must be non null).
     *  @result      The newly created object.
     */
    static Object* StaticNew(Engine* eng, Type* objType);
    
    /** Sets a slot in an object's slot-table, bypassing operator overrides and properties.
     *
     *  @param obj  [in] Pointer to the object.
     *  @param key  [in] The slot's name or key to be set.
     *  @param val  [in] The value to set the slot to.
     *  @result     If the slot was set true is returned.
     *  @note       For use in scripts since native applications can use the Object::SetSlot member 
     *              function.
     */
    static bool RawSet(Object* obj, Value& key, Value& val);
    
    /** Returns a slot in an object's slot-table, bypassing operator overrides and properties.
     *
     *  @param obj  [in] Pointer to the object.
     *  @param key  [in] The slot's name or key to be returned.
     *  @result     The value of the slot.
     *  @note       For use in scripts since native applications can use the Object::GetSlot member 
     *              function.
     */
    static Value RawGet(Object* obj, Value& key);
protected:
    Type*   type;
    Table   members; // Instance Variables
};

#define GETSELF(TYPE, OBJ, TYPENAME)                                                \
if ( !self.IsDerivedFrom(TYPE::StaticGetClass()) )                                  \
{                                                                                   \
    RaiseException(Exception::ERROR_type,                                           \
                   "Attempt to call function with incorrect type.\n expecting %s",  \
                   TYPENAME);                                                       \
}                                                                                   \
TYPE* OBJ = (TYPE*)self.val.object;

}// pika

#endif
