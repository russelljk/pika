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
#define PIKA_MODULE(NAME, ENG, MOD)                                                          \
    PIKA_MODULE_EXPORT const char* pikalib_version_##NAME(void) { return PIKA_VERSION_STR; } \
    PIKA_MODULE_EXPORT Package*    pikalib_enter_##NAME(Engine* ENG, Module* MOD)

namespace pika {
class String;
class Engine;
class Module;

typedef Package*   (*ModuleEntry_t)(Engine*, Module*);
typedef const char*  (*VersionFn_t)(void);

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

    void     Initialize(String*);
public:
    virtual ~Module();
    
    virtual void     Shutdown();
    virtual bool     Finalize();
    virtual void     MarkRefs(Collector* c);
    virtual Object*  Clone();
    
    static Module*   Create(Engine* eng, String* name, String* path, String* entryname, String* versionName);
    
    virtual String*  GetDotName() { return GetName(); }
    
    virtual bool     IsLoaded() const;
    virtual void     Run();
    virtual Package* GetImportResult();
protected:
    Package*      result;    //!< The package return from the module's entry function.
    String*       path;      //!< The system path where this module is located.
    String*       entryname; //!< Name of the entry function.
    ModuleEntry_t entry;     //!< Pointer to the entry function.
    intptr_t      handle;    //!< Handle for the module.
};

}// pika

#endif
