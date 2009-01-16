/*
 *  PModule.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_MODULE_HEADER
#define PIKA_MODULE_HEADER
/*

Native modules can provide 2 exported methods:
-----------------------------------------------------------------------------
void        pikalib_enter_Foo(Engine* ENG, Module* MOD)
const char* pikalib_version_Foo()

*/

#define PIKALIB_PREFIX_ENTER            "pikalib_enter_"
#define PIKALIB_PREFIX_VER              "pikalib_version_"
#define PIKA_MODULE(NAME, ENG, MOD)                                                            \
    PIKA_MODULE_EXPORT const char* pikalib_version_##NAME(void) { return PIKA_VERSION_STR; } \
    PIKA_MODULE_EXPORT void pikalib_enter_##NAME(Engine* ENG, Module* MOD)

namespace pika
{
class String;
class Engine;
class Module;

typedef void (*ModuleEntry_t)(Engine*, Module*);
typedef const char* (*Pika_VersionFn)(void);
// Module //////////////////////////////////////////////////////////////////////////////////////////

class PIKA_API Module : public Package
{
    PIKA_DECL(Module, Package)

    friend class Engine;
protected:
    Module(Engine* eng, 
           Type*   moduleType, 
           String* name, 
           String* path, 
           String* entryname,            
           String* verName);

    void            Initialize(String*);
public:
    virtual ~Module();

    virtual void    Shutdown();
    virtual bool    Finalize();
    virtual void    MarkRefs(Collector* c);
    virtual Object* Clone();

    static Module*  Create(Engine* eng, String* name, String* path, String* entryname, String* versionName);

    virtual String* GetDotName() { return GetName(); }

    bool            IsLoaded() const;

    String*         path;
    String*         entryname;
    ModuleEntry_t   entry;
    intptr_t        handle;
};

}//namespace pika

#endif
