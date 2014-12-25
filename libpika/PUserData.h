/*
 *  PUserData.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_USERDATA_HEADER
#define PIKA_USERDATA_HEADER

namespace pika {
class UserData;
// UserData_fn: Return false if and only if you want to delete the UserData object, NOT THE DATA, yourself! 
//              If the data is unmanaged then free it and return true.
typedef bool (*UserData_fn)  (UserData* ud); 
typedef bool (*UserData_get) (UserData* ud, const Value& key, Value& result);
typedef bool (*UserData_set) (UserData* ud, const Value& key, Value& value, u4 attr);
typedef void (*UserData_mark)(Collector* c, UserData* ud);

struct UserDataInfo
{
    const char*   name;      // Name for the userdata object.
    UserData_mark mark;      // Function to call when the userdata is marked.
    UserData_fn   finalize;  // Function to call when the userdata is collected.
    UserData_get  get;       // Function to call when read from.
    UserData_set  set;       // Function to call when written to.
    size_t        length;    // Size of the data.
    size_t        alignment; // Alignment of the data.
};

/** An object that allows native data (including classes and structs) from an external 
  * module to be used in pika. The module can provide its own heap allocated pointer 
  * with CreateWithPointer. It can also let pika 'manage' the memory with CreateManaged.  
  * Pika will allocate an aligned block at the end of the UserData object.
  *
  * ------------------------------------------------------------------------------------
  *
  * C++ notes: If your data needs the C++ operator's new & delete either
  * A1. Call new and then create the userdata with CreateWithPointer, passing the newly created object
  * 
  * B1. Create the userdata object with CreateManaged then override "opNew" and call placement new with the pointer returned from GetData().
  *     Remember overriding "opNew" will cause "init" not to be called.
  *
  * In both cases you should provide a function for UserDataInfo::finalize that will be called when the GC is about to delete the object. 
  * Remember to return true otherwise the UserData object will not be deleted.
  * A2. If you use CreateWithPointer you may delete/free the pointer returned from GetData().
  *
  * B2. If you used CreateManaged DO NOT call delete or free on the data pointer, instead call 
  *     the destructor manually: ((T*)GetData())->~T(); for data of type T.
  *
  */
PIKA_CLASS_ALIGN(PIKA_ALIGN, UserData) : public Basic
{
    PIKA_DECL(UserData, Basic)
public:
    UserData(Engine*, Type*, void*, UserDataInfo*);
protected:    
    Type*         type; // Type of this userdata
    UserDataInfo* info; //
    void*         ptr;  // Pointer to User supplied data.
public:
    virtual        ~UserData();
    virtual Type*   GetType() const;
    virtual bool    Finalize();
    virtual void    MarkRefs(Collector* c);
    virtual bool    GetSlot(const Value& val, Value& res);
    virtual bool    SetSlot(const Value& key, Value& value, u4 attr = 0);
    virtual Value   ToValue();
    INLINE UserDataInfo* GetInfo() const    { return this->info; }    
    INLINE size_t        GetSize() const    { return this->info ? (this->info->length + sizeof(UserData)) : (sizeof(UserData)); }    
    INLINE void          SetData(void* p)   { ptr = p; }    
    INLINE void*         GetData() const    { return this->ptr; }
    
    INLINE void*         GetData(const UserDataInfo* infoType) const { return (infoType == this->info) ? this->ptr : 0; }    
    
    /** Creates a new UserData object using the given pointer (may be a C++ class or struct.)
      * @param eng      [in] Pointer to a valid Engine.
      * @param type     [in] The script type of the UserData object.
      * @param data     [in] Typeless pointer to an user allocated block of data.
      * @param info     [in] Pointer to the UserDataInfo.
      *
      * @result The UserData object ready to be used inside a script.
      */
    static UserData* CreateWithPointer(Engine* eng, Type* type, void* data, UserDataInfo* info);
    
    /** Creates a new UserData object with a block at the end that contains the data (may be for a C++ class or struct.)
      * @param eng      [in] Pointer to a valid Engine.
      * @param type     [in] The script type of the UserData object.
      * @param length   [in] The length of the data required.
      * @param info     [in] Pointer to the UserDataInfo.
      *
      * @result The UserData object ready to be used inside a script.
      */
    static UserData* CreateManaged(Engine* eng, Type* type, size_t length, UserDataInfo* info);    
};

INLINE bool Value::HasUserData(UserDataInfo* info) const
{
    return tag == TAG_userdata && (val.userdata->GetInfo() == info);
}

INLINE void* Value::GetUserData(UserDataInfo* info) const
{
    if (HasUserData(info))
    {
        return val.userdata->GetData();
    }
    else
    {
        return 0;
    }
}

INLINE void* Value::GetUserDataFast() const
{
    return val.userdata->GetData();
}

// Used for registering Type's for UserData classes.
struct PIKA_API UserDataClass {
    static ClassInfo* StaticCreateClass();
    PIKA_REG(UserDataClass)
};

}// pika

#endif
