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
#define NEW_CSTR    "new"
#define OPSLICE_STR "opSlice"
#define IMPORTS_STR "imports"

#include "PBuffer.h"

#ifndef PIKA_OBJECT_HEADER
#   include "PObject.h"
#endif

#if defined(PIKA_USE_TABLE_POOL) || defined(PIKA_USE_ARRAY_POOL)
#   ifndef PIKA_MEMPOOL_HEADER
#       include "PMemPool.h"
#   endif
#endif

namespace pika {
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
class PathManager;

#if defined(PIKA_DLL) && defined(_WIN32)
template class PIKA_API Buffer<char>;
template class PIKA_API Buffer<Module*>;
template class PIKA_API Buffer<Script*>;
template class PIKA_API Buffer<Value>;
#   if defined(PIKA_USE_TABLE_POOL)
template class PIKA_API  MemObjPool<Table>;
#   endif
#   if defined(PIKA_USE_ARRAY_POOL)
template class PIKA_API  MemObjPool<Array>;
#   endif
#endif

typedef Buffer<char> TStringBuffer;


/* Engine
 *
 * TODO: A root Context may be needed. It would be the parent of any other
 *       Contexts. This would cause problems when yielding though.
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
      * @see IHook 
      */
    void AddHook(HookEvent he, hook_t h);
    
    /** Remove a HookEvent handler. 
      * @see IHook 
      */
    bool RemoveHook(HookEvent he, hook_t h);
    
    // Path management /////////////////////////////////////////////////////////
    
    /** Adds a search path to the PathManager from the given environment variable. */
    void AddEnvPath(const char* var);
    
    /** Adds a search path to the PathManager. */
    void AddSearchPath(const char* path);

    /** Adds a search path to the PathManager. */
    void AddSearchPath(String* path);
    
    INLINE PathManager* Paths() const { return paths; }
    
    Script* Compile(const char* name);
    Script* Compile(String* name, Context* parent = 0);
    void    ReadExecutePrintLoop();
    
    /** Compiles and executes a pika script in the given buffer. 
      *
      * @param buff             [in] The script buffer
      * @param buff_sz          [in] The size of the buffer or 0 if not known.
      * @param ctx              [in] The Context to execute from, if NULL then the active context is used.
      * @param globalScope      [in] The global scope for variables. If NULL then 
      *                              Pkg_World is used.
      */
    void Exec(const char* buff, size_t buff_sz=0, Context* ctx=0, Package* globalScope=0);
    
    /** Compiles the given pika script into a Function that can be run from any Context. 
      *
      * @param buff             [in] The script buffer
      * @param buff_sz          [in] The size of the buffer or 0 if not known.
      * @param globalScope      [in] The global scope for variables. If NULL then 
      *                              Pkg_World is used. 
      * @result The package located at the destination of the dot-path.
      */
    Function* CompileString(const char* buff, size_t buff_sz=0, Package* globalScope=0);
    
    INLINE Package*   GetWorld() { return Pkg_World; }
    INLINE Collector* GetGC()    { return gc; }
    
    INLINE void AddToGC(GCObject* gcobj)      { gc->Add(gcobj); }
    INLINE void AddToGCNoRun(GCObject* gcobj) { gc->AddNoRun(gcobj); }
    INLINE void AddToRoots(GCObject *gcobj)   { gc->AddAsRoot(gcobj); }
    
    INLINE void PauseGC()       { gc->Pause(); }
    INLINE void ResumeGC()      { gc->Resume(); }
    INLINE void ResumeGCNoRun() { gc->ResumeNoRun(); }
    
    void AddModule(Module* so);
    
    /** Open of a package from the given dot-path.
      * @param name             [in] The path of the package to open (ie world.Foo.Bar)
      * @param where            [in] The root package of the path. If NULL 'world' is used instead.
      * @param overwrite_always [in] Overwrite any non-package derived name clashes.
      * 
      * @result The package located at the destination of the dot-path.
      */
    Package* OpenPackageDotPath(String*  name,
                                Package* where = 0,
                                bool overwrite_always = false);
    
    Package* OpenPackage(String* name, Package* where, bool overwrite_always, u4 flags = 0);
    
    String* AllocString(const char*);
    String* AllocString(const char*, size_t);
    
    // No check versions of AllocString. Assures that the Collector will not run during the function call.
    String* GetString(const char*);
    String* GetString(const char*, size_t);
    
    String* AllocStringFmt(const char*, ...);
    
    /** Make a String persistent. */
    void PersistentString(String*);
    Type* GetTypeOf(Value& v);
    String* GetTypenameOf(Value const&);
        
    INLINE String* GetOverrideString(OpOverride ovr) { return override_strings[ovr]; }
    bool GetFieldFromValue(Value& v, String* method, Value& res);
        
    void CallConversionFunction(Context* ctx, String* name, Object* c, Value& res);
            
    /** Convert a value to boolean. If the context is null then only implicit conversions will be performed. */
    bool ToBoolean(Context* ctx, const Value& v);
    
    bool ToInteger(Value& v);
    bool ToIntegerExplicit(Context* ctx, Value& v);
    
    bool ToReal(Value& v);
    bool ToRealExplicit(Context* ctx, Value& v);
    
    String* ToString(Context* ctx, const Value& v);
    
    /** Use this for arrays, dictionaries and other containers that may contain circular references which
      * would result in an infinite loop. */
    String* SafeToString(Context* ctx, const Value& v); 
        
    void CreateRoots();
    void ScanRoots(Collector* c);
    void SweepStringTable();
    
    Debugger* GetDebugger()            { return dbg; }
    Debugger* SetDebugger(Debugger* d) { Debugger* old = dbg; dbg = d; return old; }
    
    INLINE Context* GetActiveContext()     const { return active_context; }
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
    
    String* names_String;
    String* values_String;
    String* elements_String;
    String* keys_String;
    String* attrs_String;
    String* Enumerator_String;
    String* Property_String;
    String* userdata_String;
    String* Array_String;
    String* true_String;
    String* false_String;
    String* message_String;
    String* dot_String;
    String* OpDispose_String;
    String* OpSubclass_String;
    String* OpUse_String;
    String* loading_String;
    
    Type*   Value_Type;
    Type*       Basic_Type;
    Type*           Object_Type;
    Type*               Iterator_Type;
    Type*               PathManager_Type;
    Type*               Dictionary_Type;
    Type*               Function_Type;
    Type*                   InstanceMethod_Type;
    Type*                   ClassMethod_Type;
    Type*                   BoundFunction_Type;
    Type*                   NativeFunction_Type;
    Type*                   NativeMethod_Type;
    Type*               Generator_Type;
    Type*               Array_Type;
    Type*               Context_Type;
    Type*               Package_Type;
    Type*                   Module_Type;
    Type*                   Script_Type;
    Type*               ByteArray_Type;
    Type*               LocalsObject_Type;
    Type*               Type_Type;
    Type*               Error_Type;
    Type*                   RuntimeError_Type;
    Type*                       AssertError_Type;
    Type*                   TypeError_Type;
    Type*                   ArithmeticError_Type;
    Type*                   OverflowError_Type;
    Type*                   UnderflowError_Type;
    Type*                   DivideByZeroError_Type;
    Type*                   SyntaxError_Type;
    Type*                   IndexError_Type;
    Type*                   SystemError_Type;  
    Type*           Enumerator_Type;
    Type*           Property_Type;    
    Type*           String_Type;
    Type*       Null_Type;
    Type*       Boolean_Type;
    Type*       Integer_Type;
    Type*       Real_Type;
    Object*     ArgNotDefined;
    
    Function* null_Function;
    
    Type*   GetBaseType(String*);
    void    AddBaseType(String*, Type*);
    
    static String* NumberToString(Engine* eng, const Value& v);

#if defined(PIKA_USE_TABLE_POOL)
public:
    Table*  NewTable();
    Table*  NewTable(const Table&);
    void    DelTable(Table*);
private:
    MemObjPool<Table> Table_Pool;
#endif
#if defined(PIKA_USE_ARRAY_POOL)
public:
    void*   ArrayRawAlloc();
    void    DelArray(Array*);
private:
    MemObjPool<Array> Array_Pool;
#endif
public:
    Type*           GetTypeFor(ClassInfo*);
    void            SetTypeFor(ClassInfo*, Type*);
private:
    HookEntry*      hooks[HE_max];  //!< Debug hooks into the interpreter
    PathManager*    paths;          //!< Paths used for importing
    String*         override_strings[NUM_OVERRIDES]; //!<
    StringTable*    string_table;   //!< String table
    Package*        Pkg_World;      //!< Parent Package of all Packages
    Package*        Pkg_Imports;    //!< Package containing base types
    Package*        Pkg_Types;      //!< Package containing 
    Context*        active_context; //!< The current active Context
    Debugger*       dbg;            //!< The Debugger
    Collector*      gc;             //!< The Garbage Collector
    Buffer<Module*> modules;        //!< All The Modules imported
    Buffer<Script*> scripts;        //!< All The Scripts imported
    Table           types_table;
};

}// pika

#define GCPAUSE(eng)       Engine::CollectorPause       pauser(eng)
#define GCPAUSE_NORUN(eng) Engine::CollectorPauseNoRun  pauser(eng)


#endif

