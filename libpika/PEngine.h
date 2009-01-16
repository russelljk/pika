/*
 *  PEngine.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_ENGINE_HEADER
#define PIKA_ENGINE_HEADER

#include "POpcode.h"

#ifndef PIKA_HOOKS_HEADER
#   include "PHooks.h"
#endif

#define OPINIT_CSTR "init"
#define OPNEW_CSTR  "opNew"
#define OPSLICE_STR "opSlice"
#define IMPORTS_STR "imports"

#include "PBuffer.h"

#ifndef PIKA_OBJECT_HEADER
#   include "PObject.h"
#endif

namespace pika
{
class Script;
class Object;
class String;
class Context;
class Engine;
class Debugger;
class Package;
class Collector;
class Module;
class StringTable;

#if defined(PIKA_DLL) && defined(_WIN32)
template class PIKA_API Buffer<char>;
template class PIKA_API Buffer<Module*>;
template class PIKA_API Buffer<Script*>;
template class PIKA_API Buffer<Value>;
template class PIKA_API Buffer<String*>;
#endif

typedef Buffer<char> TStringBuffer;

/* PathManager
 *
 * TODO: paths added need to be checked for consistency
 *
 */
struct PIKA_API PathManager : GCObject
{
    PathManager(Engine*);
    
    virtual ~PathManager();
    
    virtual String* FindFile(String* file);
    virtual void    MarkRefs(Collector* c);
    virtual bool    Finalize();
    virtual void    AddPath(String* str);
private:
    Buffer<String*> searchPaths;
    Engine*         engine;
};

/* Engine
 *
 * TODO: A default Context may be needed. It would be the parent of any other
 *       Contexts.
 */
class PIKA_API Engine
{
public:
#ifdef PIKA_BORLANDC
    struct CollectorPause;
    struct CollectorPauseNoRun;
#endif
    struct CollectorPause
    {
        PIKA_FORCE_INLINE CollectorPause(Engine* eng) : engine(eng)
        {
            ASSERT(engine); engine->gc->Pause();
        }
        
        PIKA_FORCE_INLINE ~CollectorPause()
        {
            engine->gc->Resume();
        }
        
    private:
        Engine* engine;
    };
    
    struct CollectorPauseNoRun
    {
        PIKA_FORCE_INLINE CollectorPauseNoRun(Engine* eng) : engine(eng)
        {
            ASSERT(engine); engine->gc->Pause();
        }
        
        PIKA_FORCE_INLINE ~CollectorPauseNoRun()
        {
            engine->gc->ResumeNoRun();
        }
    private:
        Engine* engine;
    };
    
#ifdef PIKA_BORLANDC
    friend CollectorPause;
    friend CollectorPauseNoRun;
#endif
    
protected:
    Engine();
public:
    static Engine* Create();
    
    virtual ~Engine();
    
    /** Release this Engine. */
    void Release();
private:

    // hooks ///////////////////////////////////////////////////////////////////
    
    /** Initializes the hook inteface. */
    void InitHooks();
    
    /** Removes all hook entries. */
    void RemoveAllHooks();
public:
    /** Determines if a particular HookEvent contains an entry. */
    INLINE bool HasHook(HookEvent ev) const { return hooks[ev] != 0; }
    
    /** Invokes a HookEvent with the given data. */
    bool CallHook(HookEvent he, void* data);
    
    /** Adds a HookEvent handler.
     *  @see IHook 
     */
    void AddHook(HookEvent he, IHook* h);
    
    /** Remove a HookEvent handler. 
     *  @see IHook 
     */
    bool RemoveHook(HookEvent he, IHook* h);
    
    // Path management /////////////////////////////////////////////////////////
    
    /** Adds a search path to the PathManager from the given environment variable. */
    void AddEnvPath(const char* var);
    
    /** Adds a search path to the PathManager. */
    void AddSearchPath(const char* path);

    /** Adds a search path to the PathManager. */
    void AddSearchPath(String* path);
    
    INLINE PathManager* GetPathManager() const { return paths; }
    
    Script* Compile(const char* name);
    Script* Compile(String* name, Context* parent = 0);
    
    INLINE Package*   GetWorld() { return Pkg_World; }
    INLINE Collector* GetGC()    { return gc; }
    
    INLINE void AddToGC(GCObject* gcobj)      { gc->Add(gcobj); }
    INLINE void AddToGCNoRun(GCObject* gcobj) { gc->AddNoRun(gcobj); }
    INLINE void AddToRoots(GCObject *gcobj)   { gc->AddRoot(gcobj); }
    
    INLINE void PauseGC()       { gc->Pause(); }
    INLINE void ResumeGC()      { gc->Resume(); }
    INLINE void ResumeGCNoRun() { gc->ResumeNoRun(); }
    
    void AddModule(Module* so);
    
    /** Open of a package from the given dot-path.
     *  @param name             [in] The path of the package to open (ie world.Foo.Bar)
     *  @param where            [in] The root package of the path. If NULL 'world' is used instead.
     *  @param overwrite_always [in] Overwrite any non-package derived name clashes.
     *
     *  @result The package located at the destination of the dot-path.
     */
    Package* OpenPackageDotPath(String*  name,
                                Package* where = 0,
                                bool overwrite_always = false);
    
    Package* OpenPackage(String* name, Package* where, bool overwrite_always, u4 flags = 0);
    Package* OpenPackageUnique(String* name, Package* where);
    
    String* AllocString(const char*);
    String* AllocString(const char*, size_t);
    String* AllocStringFmt(const char*, ...);
    String* PersistentString(const char*);
    
    String* GetTypenameOf(Value&);
    
    INLINE String* GetOverrideString(OpOverride ovr) { return override_strings[ovr]; }
    
    void CallConversionFunction(Context* ctx, String* name, Object* c, Value& res);
    
    bool    ToBoolean(Context* ctx, const Value& v);
    bool    ToInteger(Value& v);
    bool    ToIntegerExplicit(Context* ctx, Value& v);
    bool    ToReal(Value& v);
    bool    ToRealExplicit(Context* ctx, Value& v);
    String* ToString(Context* ctx, const Value& v);
    
    void CreateRoots();
    void ScanRoots(Collector* c);
    void SweepStringTable();
    
    Debugger* GetDebugger() { return dbg; }
    Debugger* SetDebugger(Debugger* d) { Debugger* old = dbg; dbg = d; return old; }
    
    INLINE Context* GetActiveContext() const { return active_context; }
    INLINE Context* GetActiveContextSafe() const
    {
        if (!active_context)
            RaiseException("No active Context available.");
        return active_context;
    }
    
    void ChangeContext(Context* t);
    
    bool GetImport(String* libname, Value& res);
    bool PutImport(String* libname, Value value);
private:
    void InitializeWorld();
    void UnloadAllModules();
    
    String* FindFullPathOf(String* name, String* ext);
public:
    Context* AllocContext();
        
    TStringBuffer string_buff;
    
    // Root Objects ------------------------------------------------------------
    
    String* length_String;
    String* null_String;
    String* emptyString;
    String* Object_String;
    String* toNumber_String;
    String* toString_String;
    String* toInteger_String;
    String* toReal_String;
    String* toBoolean_String;
    
    String* values_String;
    String* elements_String;
    String* keys_String;
    String* indices_String;
    String* Enumerator_String;
    String* Property_String;
    String* userdata_String;
    String* Array_String;
    String* true_String;
    String* false_String;
    String* message_String;
    String* dot_String;
    String* missing_String;
    String* OpDispose_String;
    String* OpUse_String;
    String* loading_String;
    
    Type*   Basic_Type;
    Type*   Object_Type;
    Type*       Function_Type;
    Type*           InstanceMethod_Type;
    Type*           ClassMethod_Type;
    Type*           BoundFunction_Type;
    Type*           NativeFunction_Type;
    Type*           NativeMethod_Type;
    Type*       Array_Type;
    Type*       Context_Type;
    Type*       Package_Type;
    Type*           Module_Type;
    Type*           Script_Type;
    Type*       ByteArray_Type;
    Type*       LocalsObject_Type;
    Type*       Type_Type;
    Type*       Error_Type;
    Type*           RuntimeError_Type;
    Type*           TypeError_Type;
    Type*           ReferenceError_Type;
    Type*           ArithmeticError_Type;
    Type*               OverflowError_Type;
    Type*           SyntaxError_Type;
    Type*           IndexError_Type;
    Type*           SystemError_Type;  
    Type*               AssertError_Type;
    Type*   Locals_Type;
    Type*   String_Type;
    Type*   Null_Type;
    Type*   Boolean_Type;
    Type*   Integer_Type;
    Type*   Real_Type;
    Type*   Enumerator_Type;
    Type*   Property_Type;

    Type*   GetBaseType(String*);
    void    AddBaseType(String*, Type*);
private:
    HookEntry*      hooks[HE_max];
    PathManager*    paths;
    String*         override_strings[NUM_OVERRIDES];
    StringTable*    string_table;
    Package*        Pkg_World;
    Package*        Pkg_Imports;
    Package*        Pkg_Types;
    Context*        active_context;
    Debugger*       dbg;
    Collector*      gc;
    Buffer<Module*> modules;
    Buffer<Script*> scripts;
};

}// pika

#define GCPAUSE(eng)       Engine::CollectorPause       pauser(eng)
#define GCPAUSE_NORUN(eng) Engine::CollectorPauseNoRun  pauser(eng)

using namespace pika;

#endif

