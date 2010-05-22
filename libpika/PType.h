/*
 *  PType.h
 *  See Copyright Notice in pika.h
 */
#ifndef PIKA_TYPE_HEADER
#define PIKA_TYPE_HEADER

#ifndef PIKA_PACKAGE_HEADER
#   include "PPackage.h"
#endif

namespace pika {

/////////////////////////////////////////////// Type ///////////////////////////////////////////////

/** Pika type object.
  * Types can be created through the class statement in scripts and by the static
  * Type::Create method from C++.
  */
class PIKA_API Type : public Package {
    PIKA_DECL(Type, Package)
public:
    Type(Engine*    eng,
         Type*      typeType, // This type's metatype
         String*    typeName, // typename
         Type*      baseType, // base type (may be null)
         Type_NewFn newFn,
         Package*   loc);
         
    Type(const Type*);
    
    virtual ~Type();
    
    virtual void    MarkRefs(Collector*);
    virtual Object* Clone();
    virtual String* ToString();
    
    virtual bool IsSubtype(Type*);
    virtual bool IsInstance(Basic*);
    virtual bool IsInstance(Value&);
    virtual Enumerator* GetEnumerator(String*);
    
    virtual bool GetGlobal(const Value& key, Value& result);
    virtual bool SetGlobal(const Value& key, Value& val, u4 attr = 0);
    
    /** Creates an new instance of this type.
      * 
      * @param inst     [out] The new instance.
      */
    virtual void CreateInstance(Value& inst);
    
    /** Returns the specified field shared by instances of this type.
      * 
      * @param key      [in]  The name of the field.
      * @param result   [out] The field's value.
      */
    virtual bool GetField(const Value& key, Value& result);
    
    /** Determines if the specified field can be set.
      *
      * @param key      [in]  The name of the field.
      */
    virtual bool CanSetField(const Value& key);
    
    /** Add properties to this type.
      *
      * @param rp       [in] Pointer to an array of type RegisterProperty.
      * @param count    [in] Number of properties to add.
      */
    virtual void EnterProperties(RegisterProperty* rp, size_t count, Package* pkg = 0);
    
    /** Add instance methods to this type.
     *
     *  @param rf       [in] Pointer to an array of type RegisterFunction.
     *  @param count    [in] Number of methods to add.
     */
    virtual void EnterMethods(RegisterFunction* rf, size_t count, Package* pkg = 0);
    
    /** Add class methods to this type.
      * 
      * @param rf       [in] Pointer to an array of type RegisterFunction.
      * @param count    [in] Number of class methods to add.
      */
    virtual void EnterClassMethods(RegisterFunction* rf, size_t count, Package* pkg = 0);
    
    /** Creates a new type.
      * 
      * @param eng          [in] Pointer to an Engine.
      * @param name         [in] Name of the new type.
      * @param base         [in] Type we are deriving from (or null).
      * @param createFn     [in] Native construction method.
      * @param pkg          [in] Location this package lives in. (may be null).
      * @result             The newly created type.
      * @note               This function will create a metatype automatically.
      */
    static Type* Create(Engine* eng, String* name, Type* base, Type_NewFn createFn, Package* pkg);

    /** Creates a new type from a script.
     * 
     * @param ctx          [in] Pointer to the Context to use.
     * @param body         [in] class body text.
     * @param base         [in] Type we are deriving from (or null).
     * @param createFn     [in] Native construction method.
     * @param pkg          [in] Location this package lives in. (may be null).
     * @result             The newly created type.
     * @note               This function will create a metatype automatically.
     */
    static Type* CreateWith(Context* ctx, String* body, String* name, Type* base, Package* pkg);
    
    /** Creates a new type.
      * 
      * @param eng          [in] Pointer to an Engine.
      * @param name         [in] Name of the new type.
      * @param base         [in] Type we are deriving from (or null.)
      * @param createFn     [in] Native construction method.
      * @param pkg          [in] Location this package lives in. (may be null.)
      * @param meta         [in] The metatype (must not be null.)
      * @result             The newly created type.
      */
    static Type* Create(Engine* eng, String* name, Type* base, Type_NewFn createFn, Package* pkg, Type* meta);
            
    INLINE Type*    GetBase()     { return baseType;   } //!< Returns the Type's base type or super type.
    INLINE Package* GetLocation() { return GetSuper(); } //!< Returns the package that the class statement was declared in.
    
    INLINE void SetFinal(bool f)    { final = f; }    //!< Makes the type final, meaning this type can have no sub-types.
    INLINE void SetAbstract(bool a) { abstract = a; } //!< Makes the type abstract, meaning this type cannot create instances.
    
    INLINE bool IsFinal()    const { return final; }    //!< Returns whether this type is final.
    INLINE bool IsAbstract() const { return abstract; } //!< Returns whether this type is abstract.
    
    /** Returns an Array containing all subtypes. */
    INLINE Array* GetSubtypes() { return subtypes; }
    
    /** Adds a type method. */
    void AddMethod(Function*);
    void AddClassMethod(Function*);
    
    Type_NewFn  GetNewFn() const { return newfn; }
protected:
    /** Adds a subtype. Called by derived types when they are created. */
    void AddSubtype(Type*);
    
    Type*      baseType; // base or super type
    Type_NewFn newfn;    // native allocation + construction function
    bool       final;    // type cannot be used as the base for another type
    bool       abstract; // type cannot create instances.
    Array*     subtypes; // all types that are direct descendence of this type
};

}// pika
#endif
