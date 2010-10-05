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

namespace pika {
/* Forward Declarations */
struct RegisterFunction;
struct RegisterProperty;
class Package;
class Array;
class Type;
class Object;

/** Instance creation function used by types. */
typedef void (*Type_NewFn)(Engine*, Type*, Value&);

/** Base class for all propertied and typed objects.
  * Each object has an associated type and a slot-table where instance variables are kept.
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
    INLINE Object(Engine* eng, Type* typeObj) : Basic(eng), type(typeObj), members(0) {}
    
    INLINE Object(const Object* rhs)
            : Basic(rhs->engine), type(rhs->type), members(0)
    { 
        if (rhs->members)
        {
            Members(*(rhs->members));
        }
    }
public:
    virtual ~Object();
    
    virtual Type* GetType() const;
    virtual void  SetType(Type*);
    virtual Value ToValue();
    virtual void  AddFunction(Function*);
    virtual void  AddProperty(Property*);
    
    virtual bool  SetSlot(const Value& key, Value& value, u4 attr = 0);
    virtual bool  CanSetSlot(const Value& key);
    
    virtual bool  GetSlot(const Value& key, Value& result);
    
    virtual bool  HasSlot(const Value& key);
    
    virtual bool  DeleteSlot(const Value& key);
    
    virtual bool  BracketRead(const Value& key, Value& result);
    virtual bool  BracketWrite(const Value& key, Value& value, u4 attr = 0);
    
    virtual void  MarkRefs(Collector*);

    virtual Object* Clone();
    virtual String* ToString();
    virtual void    Init(Context*);
    
    virtual Enumerator* GetEnumerator(String*);
    
    /** Add native functions to an object.
      * 
      * @param rf       [in] Pointer to an array of type RegisterFunction.
      * @param count    [in] Number of functions to add.
      * @param pkg      [in] The package, module or script the functions live in. 
      */
    virtual void EnterFunctions(RegisterFunction* rf, size_t count, Package* pkg = 0);
    
    /** Add properties to this object.
      *
      * @param rp       [in] Pointer to an array of type RegisterProperty.
      * @param count    [in] Number of properties to add.
      */
    virtual void EnterProperties(RegisterProperty* rp, size_t count, Package* pkg = 0);
        
    /** Create a new class Instance.
      * 
      * @param eng   [in] The Engine the object is created in.
      * @param count [in] Type type of the object (must be non null).
      * @result      The newly created object.
      */
    static Object* StaticNew(Engine* eng, Type* objType);
    
    /** Sets a slot in an object's slot-table, bypassing operator overrides and 
      * properties.
      *
      * @param obj  [in] Pointer to the object.
      * @param key  [in] The slot's name or key to be set.
      * @param val  [in] The value to set the slot to.
      * @result     If the slot was set true is returned.
      * @note       For use in scripts since native applications can call Object::SetSlot. 
      */
    static bool RawSet(Object* obj, Value& key, Value& val);
    
    /** Returns a slot in an object's slot-table, bypassing operator overrides 
      * and properties.
      *
      * @param obj  [in] Pointer to the object.
      * @param key  [in] The slot's name or key to be returned.
      * @result     The value of the slot.
      * @note       For use in scripts since native applications can call Object::GetSlot.
      */
    static Value RawGet(Object* obj, Value& key);
    
    static void Constructor(Engine* eng, Type* obj_type, Value& res);
    static void StaticInitType(Engine* eng);
protected:
    /** Return a reference to this object's members Table. If the table does not
      * exist it will be created.
      */
    Table& Members();
    
    /** Return a reference to this object's members Table. If the table does not
      * exist it will be cloned from the Table provided.
      *
      * @param other [in] The Table to clone is Object::members is not allocated.
      */
    Table& Members(const Table& other);
    
    /** This object's type. 
      *
      * @warning All object should have a type so make sure when you create an
      * object that you provide a valid type.
      *
      * @note During Engine's start-up there are a handful of object that are created
      * sans types. In that case the type will be set to the correct type before
      * a the start-up is finished.
      */    
    Type* type;
    
    /** Table of instance variables which are created lazily when written to.
      *
      * @note If you are deriving from Object use Object::Members to write to 
      * the members table. If you need to read from the Table check that it 
      * existing first, do not create it unless you are writing to it.
      */
    Table* members; // Instance Variables
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
