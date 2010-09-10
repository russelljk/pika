/*
 *  PImport.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"

extern const char* Pika_rindex(const char * str, int idx); // needed by: Pika_ConvertDotName

namespace pika {
namespace {

String* Pika_ConvertDotName(Engine* eng, String* path)
{
    const char* fullpath  = path->GetBuffer();
    const char* extension = Pika_rindex(fullpath, '.');    
    const char* last      = 0;
    
    // No need to convert because the path-separator is a dot not a slash.
    if (PIKA_PATH_SEP_CHAR == '.')
        return path;
    
    // a dot-path should not contain any path separators.
    if (Pika_rindex(fullpath, PIKA_PATH_SEP_CHAR))
        return path; 
    
    // If the file given has a known extension for Pika.        
    if (extension && ((Pika_strcasecmp(extension, PIKA_EXT) == 0) || (Pika_strcasecmp(extension, PIKA_EXT_ALT) == 0)))
    {
        // if the extension does not occupy the entire string
        if (extension > fullpath)
        {
            // Mark the end of the path to be the start of the extension.
            last = extension;
        }
        // Otherwise we were passed an extension or a (Unix) hidden file.
        else
        {            
            return path;
        }
    }
    else
    {
        // No known extension; we will just use the entire string.
        last = fullpath + path->GetLength();
    }
    
    char* buff = (char*)Pika_malloc(path->GetLength()); // A new buffer to hold the conversion.
    size_t len = last - fullpath;                        // The length we are converting.
    for (size_t i = 0; i < len; ++i)
    {
        int x = fullpath[i];
        
        // Convert dots into the default system path separator.
        if (x == '.') 
        {
            buff[i] = PIKA_PATH_SEP_CHAR;
        }
        else
        {
            buff[i] = x;
        }
    }
    Pika_memcpy(buff + len, extension, path->GetLength() - len);
    Pika_free(buff);
    return eng->AllocString(buff, path->GetLength());
}

/** Constructs a system dependent Module name. */
String* Pika_ConstructModuleName(Engine* eng, String* name)
{
    String* shared_prefix = eng->AllocString(PIKA_LIB_PREFIX);
    String* shared_ext    = eng->AllocString(PIKA_LIB_EXT);
    String* full_namestr  = String::Concat(shared_prefix, name);
    String* res           = String::Concat(full_namestr, shared_ext);
    return res;
}

/** Constructs the symbol name of a Module's entry point. */
String* Pika_ConstructModuleFnName(Engine* eng, String* name, const char* libPrefix)
{
    String* prefix = eng->AllocString(libPrefix);
    String* res    = String::Concat(prefix, name);
    return res;
}

/** Loads a native module from disk. 
  * No check is made to ensure that the module was already loaded. */
Module* Pika_importModule(Context* ctx, String* name)
{
    Engine* eng = ctx->GetEngine();
    GCPAUSE_NORUN(eng);
    
    String* file_name = Pika_ConstructModuleName(eng, name);
    String* load_name = Pika_ConstructModuleFnName(eng, name, PIKALIB_PREFIX_ENTER);
    String*  ver_name = Pika_ConstructModuleFnName(eng, name, PIKALIB_PREFIX_VER);
    String* full_path = 0;
    
    if ((full_path = eng->GetPathManager()->FindFile(file_name)))
    {
        file_name = full_path;
    }
    
    Module* module = Module::Create(eng, name, file_name, load_name, ver_name);
    
    if (!module || !module->IsLoaded())
        return 0;
    
    module->Run();
    
    return module;
}

/** Loads a script from disk.
  * No check is made to ensure that the script was already loaded. */
Package* Pika_importScript(Context* ctx, String* name)
{
    Engine* engine = ctx->GetEngine();
    Script* script = 0;
   // size_t sp = ctx->GetStackSize();
    name = Pika_ConvertDotName(engine, name);
    if ((script = engine->Compile(name, ctx)))
    {       
        ctx->Push(script);
        script->Run(0);
        
        ctx->Pop(1); // pop script                
        return script->GetImportResult();
    }
    return 0;
}

}// namespace

// ModuleImportHook ////////////////////////////////////////////////////////////////////////////////

struct ModuleImportHook : IHook
{
    ModuleImportHook() { }
    
    virtual ~ModuleImportHook() { }
    
    virtual bool OnEvent(HookEvent ev, void* data)
    {
        if (ev == HE_import && data)
        {
            ImportData* importData = (ImportData*)data;
            Engine*     eng  = importData->engine;
            String*     name = importData->name;
            
            Value val(eng->loading_String);
            eng->PutImport(name, val);
            
            Module* module = Pika_importModule(importData->context, name);
            
            if (module)
            {
                Value vmod(module);
                eng->PutImport(name, vmod);
                
                importData->result = module->GetImportResult();
                return true;
            }
        }
        return false;
    }
    
    virtual void Release(HookEvent) { Pika_delete(this); }
};

// ScriptImportHook ////////////////////////////////////////////////////////////////////////////////

struct ScriptImportHook : IHook
{
    ScriptImportHook() { }
    
    virtual ~ScriptImportHook() {}
    
    virtual bool OnEvent(HookEvent ev, void* data)
    {
        if (ev == HE_import && data)
        {
            ImportData* importData = (ImportData*)data;
            Package* result = Pika_importScript(importData->context, importData->name);
            
            if (result)
            {
                importData->result = result;
                return true;
            }
        }
        return false;
    }
    
    virtual void Release(HookEvent) { Pika_delete(this); }
};

void Initialize_ImportAPI(Engine* eng)
{
    ScriptImportHook* scriptImport = Pika_new<ScriptImportHook>();
    ModuleImportHook* moduleImport = Pika_new<ModuleImportHook>();
    
    eng->AddHook(HE_import, moduleImport);
    eng->AddHook(HE_import, scriptImport);
}

// General purpose module and script loader, intended to be used from inside a script.

int Global_import(Context* ctx, Value& self)
{
    Engine* eng = ctx->GetEngine();
        
    u4 argc = ctx->GetArgCount();
    ctx->CheckStackSpace(argc);
    
    for (u4 a = 0; a < argc; ++a)
    {
        String* name = ctx->GetStringArg(a);
        
        if (!name->GetLength())
        {
            RaiseException("Attempt to call import with zero length string.");
            return 0;
        }
        
        Value res;
        if (eng->GetImport(name, res))
        {
             // module was imported already or is in the process of being imported
            if (eng->Module_Type->IsInstance(res))
            {
                Module* module = (Module*)res.val.object;
                ctx->SafePush(module->GetImportResult());
                continue;
            }
            else if (eng->Package_Type->IsInstance(res))
            {
                // already loaded                
                ctx->SafePush(res);
                continue;
            }
            else if (eng->Function_Type->IsInstance(res)) // Function loader.
            {
                ctx->CheckStackSpace(2);
                ctx->PushNull();
                ctx->Push(res);
                if (ctx->SetupCall(0))
                {
                    ctx->Run();
                }
                Value libres = ctx->PopTop();
                if (eng->Package_Type->IsInstance(libres))
                {                    
                    ctx->SafePush(libres);
                }
                else
                {
                    RaiseException("Attempt to import %s failed.", name->GetBuffer());
                    return 0;                    
                }
            }
            else if (res.IsString() && (res.val.str == eng->loading_String || res.val.str == eng->AllocString("compiling")))
            {
                RaiseException("Attempt to import '%s' failed. Circular dependency detected.", name->GetBuffer());
            }
            else
            {                
                RaiseException("Attempt to import %s failed.", name->GetBuffer());
                return 0;
            }
        }
        else
        {
            ImportData data = { name, eng, ctx, 0 };
            eng->CallHook(HE_import, &data);
            
            Package* lib = data.result;
            
            if (lib)
            {
                ctx->SafePush(lib);
                continue;
            }
            else
            {
                RaiseException("Attempt to import %s failed.", name->GetBuffer());
                return 0;
            }
        }
    }
    return argc;
}

}// pika
