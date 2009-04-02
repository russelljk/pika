/*
 *  PClassInfo.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_CLASSINFO_HEADER
#define PIKA_CLASSINFO_HEADER

namespace pika 
{

class PIKA_API ClassInfo 
{
public:
    ClassInfo(const char*, ClassInfo*);
    ~ClassInfo();

    bool IsDerivedFrom(const ClassInfo* other) const;

    INLINE const char*      GetName()  const { return name;  }
    INLINE const ClassInfo* GetSuper() const { return super; }
    INLINE ClassInfo*       GetNext()  const { return next;  }

    static  ClassInfo* GetFirstClass() { return firstClass; }
    static  ClassInfo* Create(const char* name, ClassInfo* super);

private:
    const char*       name;
    const ClassInfo*  super;
    ClassInfo*        next;
    static ClassInfo* firstClass;
};

}//pika

#define PIKA_REG(ACLASS)                                                                            \
    public:  static ClassInfo* StaticGetClass();                                                    \
    public:  static ClassInfo* ACLASS##ClassInfo;


#define PIKA_DECL(ACLASS, ASUPER)                                                                   \
 PIKA_REG(ACLASS)                                                                                   \
 public:                                                                                            \
    typedef ASUPER ThisSuper;                                                                       \
                                                                                                    \
    static inline ClassInfo* StaticCreateClass()                                                    \
    {                                                                                               \
        if (!ACLASS##ClassInfo)                                                                     \
        {                                                                                           \
            ACLASS##ClassInfo = ClassInfo::Create(#ACLASS, ASUPER::StaticGetClass());               \
        }                                                                                           \
        return ACLASS##ClassInfo;                                                                   \
    }                                                                                               \
    virtual ClassInfo* GetClassInfo() { return StaticGetClass(); }


#define PIKA_IMPL(ACLASS)                                                                           \
    ClassInfo* ACLASS::StaticGetClass() { return StaticCreateClass(); }                             \
    ClassInfo* ACLASS::ACLASS##ClassInfo = StaticGetClass();

#endif
