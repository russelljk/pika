/*
*  gEngine.cpp
*  See Copyright Notice in Pika.h
*/
#include "Pika.h"
#include "PPlatform.h"
#include "PAst.h"
#include "PParser.h"
#include "PStringTable.h"

typedef Buffer<Module*>::Iterator ModuleIterator;
typedef Buffer<Script*>::Iterator ScriptIterator;

extern String* NumberToString(Engine* eng, const Value& v);

String* FindPackageName(Engine* eng, String* path)
{
    const char* fullpath  = path->GetBuffer();
    const char* extension = Pika_rindex(fullpath, '.');
    const char* filename  = Pika_rindex(fullpath, PIKA_PATH_SEP_CHAR);
    
    if (!filename)
        filename = fullpath;
    else
        filename++;
        
    if (extension &&
            (extension > filename) &&
            ((Pika_strcasecmp(extension, PIKA_EXT) == 0) ||
             (Pika_strcasecmp(extension, PIKA_EXT_ALT) == 0)))
    {
        return eng->AllocString(filename, extension - filename);
    }
    else
    {
        return eng->AllocString(filename);
    }
    return 0;
}

bool FileHasExt(const char* filename, const char* filext)
{
    if (!filename)
        return false;
        
    const char* ext = Pika_rindex(filename, '.');
    
    if (!ext)
        return false;
        
    return Pika_strcasecmp(ext, filext) == 0;
}

namespace pika
{

bool EndsWithSeperator(const char* str, size_t len)
{
    if (len > 0 && str[len - 1] == PIKA_PATH_SEP_CHAR)
        return true;
    return false;
}

PathManager::PathManager(Engine* eng)
        : engine(eng)
{
    char* const buff = Pika_GetCurrentDirectory();
    String* startingPath = 0;
    
    if (buff)
    {
        startingPath = engine->AllocString(buff);
        Pika_free(buff);
    }
    else
    {
        startingPath = engine->AllocString("./");
    }
    
    if (startingPath)
        AddPath(startingPath);
}

PathManager::~PathManager() {}

String* PathManager::FindFile(String* file)
{
    char* temp = (char*)Pika_malloc(sizeof(char) * (PIKA_MAX_PATH + 1));
    int oldErrno = errno;
    errno = 0;
    for (size_t i = 0; i < searchPaths.GetSize(); ++i)
    {
        String* currpath = searchPaths[i];
        
        if (Pika_snprintf(temp, PIKA_MAX_PATH, "%s%s", currpath->GetBuffer(), file->GetBuffer()) < 0)
        {
            continue;
        }
        temp[PIKA_MAX_PATH] = '\0';
        
        if (Pika_FileExists(temp))
        {
            String* res = engine->AllocString(temp);
            Pika_free(temp);
            errno = oldErrno;
            return res;
        }
    }
    Pika_free(temp);
    
    if (Pika_FileExists(file->GetBuffer()))
    {
        return file;
    }
    errno = oldErrno;
    return 0;
}

void PathManager::MarkRefs(Collector* c)
{
    size_t sz = searchPaths.GetSize();
    for (size_t i = 0; i < sz; ++i)
        c->MoveToGray(searchPaths[i]);
}

void Engine::InitHooks()
{
    Pika_memzero(hooks, sizeof(HookEntry*)*HE_max);
}

void Engine::RemoveAllHooks()
{
    for (size_t i = 0; i < HE_max; ++i)
    {
        HookEntry* curr = hooks[i];
        
        while (curr)
        {
            IHook* h = curr->hook;
            
            if (h)
            {
                h->Release((HookEvent)i);
            }
            
            HookEntry* nxt = curr->next;
            Pika_free(curr);
            curr = nxt;
        }
        hooks[i] = 0;
    }
}

bool Engine::CallHook(HookEvent he, void* data)
{
    HookEntry* curr = hooks[he];
    
    while (curr)
    {
        IHook* h = curr->hook;
        
        if (h)
        {
            if (h->OnEvent(he, data))
                return true;
        }
        curr = curr->next;
    }
    return false;
}

void Engine::AddHook(HookEvent he, IHook* h)
{
    HookEntry* hook = (HookEntry*)Pika_malloc(sizeof(HookEntry));
    hook->hook = h;
    hook->next = hooks[he];
    hooks[he] = hook;
}

bool Engine::RemoveHook(HookEvent he, IHook* h)
{
    // TODO: untested
    HookEntry** pointerTo = &(hooks[he]);
    HookEntry*  current   =   hooks[he];
    
    while (current)
    {
        if (h == current->hook)
        {
            *pointerTo = current->next;
            
            if (h)
                h->Release(he);
                
            Pika_free(current);
            
            return true;
        }
        pointerTo = &(current->next);
        current = current->next;
    }
    return false;
}

Engine* Engine::Create()
{
    Engine* eng = 0;
    PIKA_NEW(Engine, eng, ());
    return eng;
}

Engine::Engine()
        : length_String(0), null_String(0), emptyString(0), Object_String(0), toNumber_String(0), toString_String(0),
        toInteger_String(0), toReal_String(0), toBoolean_String(0), values_String(0), elements_String(0), keys_String(0),
        indices_String(0), Enumerator_String(0), Property_String(0), userdata_String(0), Array_String(0), true_String(0),
        false_String(0), message_String(0), dot_String(0), missing_String(0), OpDispose_String(0), OpUse_String(0), loading_String(0),
        Basic_Type(0), Object_Type(0), Function_Type(0), InstanceMethod_Type(0), ClassMethod_Type(0), BoundFunction_Type(0), NativeFunction_Type(0),
        NativeMethod_Type(0), Array_Type(0), Context_Type(0), Package_Type(0), Module_Type(0), Script_Type(0), ByteArray_Type(0),
        LocalsObject_Type(0), Type_Type(0), Error_Type(0), RuntimeError_Type(0), TypeError_Type(0), ReferenceError_Type(0), 
        ArithmeticError_Type(0), OverflowError_Type(0), SyntaxError_Type(0), IndexError_Type(0), SystemError_Type(0), AssertError_Type(0),
        Locals_Type(0), String_Type(0), Null_Type(0), Boolean_Type(0), Integer_Type(0), Real_Type(0), Enumerator_Type(0), Property_Type(0),
        paths(0),
        string_table(0),
        Pkg_World(0), Pkg_Imports(0), Pkg_Types(0),
        active_context(0),
        dbg(0),
        gc(0)
{
    InitHooks();
    PIKA_NEW(StringTable, string_table, (this));
    PIKA_NEW(Collector, gc, (this));
    InitializeWorld();
}

Engine::~Engine()
{
    RemoveAllHooks();
    
    Pika_delete(gc);
    Pika_delete(paths);
    Pika_delete(string_table);
    UnloadAllModules();
}

void Engine::Release()
{
    Pika_delete(this);
}

void Engine::AddModule(Module* so)
{
    modules.Push(so);
}

void Engine::UnloadAllModules()
{
    for (ModuleIterator iter = modules.Begin(); iter != modules.End(); ++iter)
    {
        (*iter)->Shutdown();
        Pika_delete(*iter);
    }
    modules.Clear();
}

Package* Engine::OpenPackage(String* name, Package* where, bool overwrite_always, u4 flags)
{
    if (!where)
    {
        where = Pkg_World;
    }
    
    Value pkgvar;
    if (!overwrite_always && where->GetSlot(name, pkgvar))
    {
        if (pkgvar.IsDerivedFrom(Package::StaticGetClass()))
        {
            return pkgvar.val.package;
        }
        else
        {
            RaiseException(Exception::ERROR_runtime, "package %s could not be created because of name conflict", name->GetBuffer());
        }
    }
    
    Package* pkg = Package::Create(this, name, where);
    where->SetSlot(name, pkg, flags);
    return pkg;
}

Package* Engine::OpenPackageUnique(String* name, Package* where)
{
    if (!where)
    {
        where = Pkg_World;
    }
    
    Value pkgvar;
    
    if (where->GetSlot(name, pkgvar))
    {
        RaiseException(Exception::ERROR_runtime, "package %s could not be opened because of name conflict", name->GetBuffer());
    }
    
    Package* pkg = Package::Create(this, name, where);
    where->SetSlot(name, pkg);
    return pkg;
}

Package* Engine::OpenPackageDotPath(String* name, Package* where, bool overwrite_always)
{
    Package*       pkg  = where ? where : this->GetWorld();
    const char*    cstr = name->GetBuffer();
    const char*    curr = cstr;
    const char*    end  = cstr + name->GetLength();
    TStringBuffer  buff;
    
    while (curr <= end)
    {
        if (*curr == '.' || curr == end)
        {
            size_t len = curr - cstr;
            buff.Resize(len + 1);
            
            Pika_memcpy(buff.GetAt(0), cstr, len * sizeof(char));
            buff[len] = 0;
            
            String* s = AllocString(buff.GetAt(0), len);
            pkg = this->OpenPackage(s, pkg, overwrite_always);
            curr++;
            cstr = curr;
        }
        else
        {
            ++curr;
        }
    }
    return pkg;
}

String* Engine::FindFullPathOf(String* name, String* ext)
{
    bool hasext = FileHasExt(name->GetBuffer(), ext->GetBuffer());
    
    String* fullpath = paths->FindFile(name);
    
    if (!fullpath)
    {
        if (!hasext)
        {
            // doesn't end in ext
            name = String::Concat(name, ext);
            fullpath = paths->FindFile(name);
        }
    }
    return fullpath;
}

bool Engine::GetImport(String* libname, Value& res)
{
    if (Pkg_Imports->GetSlot(libname, res))
    {
        return true;
    }
    res.SetFalse();
    return false;
}

bool Engine::PutImport(String* libname, Value value)
{
    if (Pkg_Imports->SetSlot(libname, value, Slot::ATTR_protected | Slot::ATTR_forcewrite))
    {
        return true;
    }
    return false;
}

void Engine::AddEnvPath(const char* var)
{
    if (!var)
        return;
        
    GCPAUSE_NORUN(this);
    
    char* path = 0;
    const char* next_pos = 0;
    int m, n;
    const char delim = ':';
    
    for (path = getenv(var); path && *path; path += m)
    {
        if ( (next_pos = Pika_index(path, delim)) )
        {
            n = next_pos - path;
            m = n + 1;
        }
        else
        {
            m = n = strlen(path);
        }
        AddSearchPath(AllocString(path, n));
    }
}

bool PathManager::Finalize()
{
    return false;
}

void PathManager::AddPath(String* path)
{
    GCPAUSE_NORUN(engine);
    if ( !EndsWithSeperator(path->GetBuffer(),
                            path->GetLength())
       )
    {
        path = String::Concat(path, engine->AllocString(PIKA_PATH_SEP));
    }
    
    engine->GetGC()->WriteBarrier(this, path);
    searchPaths.Push(path);
}

void Engine::AddSearchPath(const char* path)
{
    String* str_path = AllocString(path);
    paths->AddPath(str_path);
}

void Engine::AddSearchPath(String* path)
{
    paths->AddPath(path);
}

Script* Engine::Compile(String* name, Context* parent)
{
    GCPAUSE_NORUN(this);
    
    String* extstr   = AllocString(PIKA_EXT);
    String* fullPath = FindFullPathOf(name, extstr);
    
    if (!fullPath)
    {
        extstr   = AllocString(PIKA_EXT_ALT);
        fullPath = FindFullPathOf(name, extstr);
        
        if (!fullPath)
        {
            return 0;
        }
    }
    
    name = fullPath;
    
    String* str_dot_name = FindPackageName(this, fullPath);
    
    Value pkgDotVar(NULL_VALUE);
    
    if (GetImport(str_dot_name, pkgDotVar))
    {
        if (pkgDotVar.IsDerivedFrom(Script::StaticGetClass()))
        {
            return (Script*)pkgDotVar.val.object;
        }
        else
        {
            return 0;
        }
    }
    
    // parse the file
    PutImport(str_dot_name, Value(AllocString("compiling")));
    
    FILE* yyin = fopen(name->GetBuffer(), "rb");
    
    if (!yyin)
    {
        return 0;
    }
    
    // create context for this script
    
    LiteralPool* literals  = 0;
    u2 loadindex = 0;
    
    // Create the CompileState and Parser, use std::auto_ptr so that they are deleted in-case of exception ot early return.
    std::auto_ptr<CompileState> compinfo(new CompileState(this));
    std::auto_ptr<Parser>       parser  (new Parser(compinfo.get(), yyin));
    
    try
    {
        Program* tree = parser->DoParse();
        tree->CalculateResources(0, *compinfo);
        
        if (compinfo->HasErrors())
        {
            RaiseException(Exception::ERROR_syntax, "Attempt to compile script %s.\n", name->GetBuffer());
        }
        
        tree->GenerateCode();
        
        if (compinfo->HasErrors())
        {
            RaiseException(Exception::ERROR_syntax, "Attempt to generate code for script %s.\n", name->GetBuffer());
        }
        
        loadindex = tree->index;
        literals = compinfo->literals;
        
    }
    catch (Exception&)
    {
        compinfo->SyntaxErrorSummary();
        
        if (yyin)
        {
            fclose(yyin);
            yyin = 0;
        }
        
        PutImport(str_dot_name, Value(AllocString("invalid")));
        return 0;
    }
    
    if (yyin)
    {
        fclose(yyin);
        yyin = 0;
    }
    
    Script* script = Script::Create(this, str_dot_name, Pkg_World);
    scripts.Push(script);
    
    Context* context = Context::Create(this, this->Context_Type);
    context->state = Context::SUSPENDED;
    
    Value closure;
    const Value& f = literals->Get(loadindex);
    
    context->package = script;
    context->prev    = parent;
    context->DoClosure(f.val.def, closure);
    
    script->Initialize(literals, context, closure.val.function);
    
    gc->ForceToGray(script);
    
    PutImport(str_dot_name, Value(script));
    
    return script;
}

Script* Engine::Compile(const char* name)
{
    if (!name)
    {
        return 0;
    }
    GCPAUSE(this);
    
    String* strname = AllocString(name);
    Script* script  = Compile(strname, 0);
    return script;
}

String* Engine::AllocString(const char* str)
{
    return string_table->Get(str);
}

String* Engine::AllocString(const char* str, size_t length)
{
    return string_table->Get(str, length);
}

String* Engine::AllocStringFmt(const char* fmt, ...)
{
    static const size_t ERR_BUF_SZ = 1024;
    // TODO: We Should be able to handle any size string upto the Maximum String length.
    char buffer[ERR_BUF_SZ + 1];
    va_list args;
    
    va_start(args, fmt);
    Pika_vsnprintf(buffer, ERR_BUF_SZ, fmt, args);
    va_end(args);
    
    buffer[ERR_BUF_SZ] = '\0';
    
    return AllocString(buffer);
}

String* Engine::PersistentString(const char* str)
{
    String* s = AllocString(str);
    s->gcflags |= GCObject::Persistent;
    return s;
}

/** Convert a value to boolean. The context may be null. */
bool Engine::ToBoolean(Context* ctx, const Value& v)
{
    switch (v.tag)
    {
    case TAG_null:        return false;
    case TAG_boolean:     return v.val.index != 0;
    case TAG_integer:     return v.val.integer != 0;
    case TAG_real:        return Pika_RealToBoolean(v.val.real);
    case TAG_def:         return true;
    case TAG_string:      return v.val.str->GetLength() != 0;
    case TAG_enumerator:  return v.val.enumerator->IsValid();
    case TAG_property:    return true;    
    case TAG_userdata:    return true;
    case TAG_object:
    {
        /*
        if (ctx)
        {
        Value res;
        CallConversionFunction(ctx, toBoolean_String, v.val.object, res);
        
        if (res.tag != TAG_boolean)
        {
            RaiseException(Exception::ERROR_runtime, "Conversion operator %s failed.", toBoolean_String->GetBuffer());
        }
        }
        */
        return v.val.index != 0;
    }
    }
    return false;
}

bool Engine::ToInteger(Value& v)
{
    switch (v.tag)
    {
    case TAG_null:    v.Set((pint_t)0);                      return true;
    case TAG_boolean: v.Set((pint_t)(v.val.index ? 1 : 0));  return true;
    case TAG_integer:                                        return true;
    case TAG_real:    v.Set((pint_t)v.val.real);             return true;
    }
    return false;
}

bool Engine::ToReal(Value& v)
{
    switch (v.tag)
    {
    case TAG_null:    v.Set((preal_t)0.0);                       return true;
    case TAG_boolean: v.Set((preal_t)(v.val.index ? 1.0 : 0.0)); return true;
    case TAG_integer: v.Set((preal_t)v.val.integer);             return true;
    case TAG_real:                                               return true;
    }
    return false;
}

bool Engine::ToIntegerExplicit(Context* ctx, Value& v)
{
    switch (v.tag)
    {
    case TAG_null:    v.Set((pint_t)0);                      return true;
    case TAG_boolean: v.Set((pint_t)(v.val.index ? 1 : 0));  return true;
    case TAG_integer:                                        return true;
    case TAG_real:    v.Set((pint_t)v.val.real);             return true;
    case TAG_string:
    {
        pint_t i;
        if (v.val.str && v.val.str->ToInteger(i))
        {
            v.Set(i);
            return true;
        }
    }
    return false;
    case TAG_object:
    {
        Value res;
        CallConversionFunction(ctx, toInteger_String, v.val.object, res);
        
        if (res.tag != TAG_integer)
            RaiseException(Exception::ERROR_runtime, "Conversion operator %s failed.", toInteger_String->GetBuffer());
        v = res;    
        return true;
    }
    }
    return false;
}

bool Engine::ToRealExplicit(Context* ctx, Value& v)
{
    switch (v.tag)
    {
    case TAG_null:    v.Set((preal_t)0.0);                       return true;
    case TAG_boolean: v.Set((preal_t)(v.val.index ? 1.0 : 0.0)); return true;
    case TAG_integer: v.Set((preal_t)v.val.integer);             return true;
    case TAG_real:                                               return true;
    case TAG_string:
    {
        preal_t r;
        if (v.val.str && v.val.str->ToReal(r))
        {
            v.Set(r);
            return true;
        }
    }
    return false;
    case TAG_object:
    {
        Value res;
        CallConversionFunction(ctx, toReal_String, v.val.object, res);
        
        if (res.tag != TAG_real)
            RaiseException(Exception::ERROR_runtime, "Conversion operator %s failed.", toReal_String->GetBuffer());
            
        return true;
    }
    }
    return false;
}

String* Engine::ToString(Context* ctx, const Value& v)
{
    switch (v.tag)
    {
    case TAG_null:    return null_String;
    case TAG_boolean: return v.val.index ? true_String : false_String;
    case TAG_integer:
    case TAG_real:
    case TAG_index:  return NumberToString(this, v);
    case TAG_gcobj:  return AllocString("<gcobj>");
    case TAG_def:    return AllocString("<def>");
    case TAG_string: return v.val.str ? v.val.str : emptyString;
    
        // TODO: Fix the enumerator's and userdata's toString conversion.
        
    case TAG_enumerator: return AllocString("enumerator");
    case TAG_property:   return String::ConcatSpace(Property_String, v.val.property->GetName());
    
    case TAG_userdata:   return AllocString("userdata");
    
    case TAG_object:
    {
        Value res;
        CallConversionFunction(ctx, toString_String, v.val.object, res);
        if (res.tag != TAG_string)
            RaiseException("Conversion operator %s failed.", toString_String->GetBuffer());
            
        return res.val.str;
    }
    }
    return emptyString;
}

void Engine::CallConversionFunction(Context* ctx, String* name, Object* c, Value& res)
{
    Value key(name);
    Value funop(NULL_VALUE);
    Type* c_type = 0;
    
    if (!c || !(c_type = c->GetType()))
    {
        RaiseException(Exception::ERROR_runtime, "conversion operator %s not defined.", name->GetBuffer());
    }
    
    if (c_type->GetField(key, funop))   // XXX: Get Override
    {
        ctx->Push(Value(c));    // push self
        ctx->Push(funop);       // push closure
        
        if (ctx->SetupCall(0))
        {
            ctx->Run();
        }
        
        res = ctx->PopTop();
    }
    else
    {
        RaiseException(Exception::ERROR_runtime, "conversion operator %s not defined.", name->GetBuffer());
    }
}

Type* Engine::GetBaseType(String* name)
{
    Value key(name);
    Value res(NULL_VALUE);
    Pkg_Types->GetSlot(key, res);
    return res.val.type;
}

void Engine::AddBaseType(String* name, Type* btype)
{
    Value key(name);
    Value res(btype);
    Pkg_Types->SetSlot(key, res);
}

void Engine::CreateRoots()
{
    AddToRoots(Pkg_World);
    AddToRoots(Pkg_Imports);
    AddToRoots(Pkg_Types);
    AddToRoots(ByteArray_Type);
    AddToRoots(LocalsObject_Type);
    AddToRoots(Basic_Type);
    AddToRoots(Object_Type);
    AddToRoots(Function_Type);
    AddToRoots(Array_Type);
    AddToRoots(Context_Type);
    AddToRoots(Package_Type);
    AddToRoots(Module_Type);
    AddToRoots(Script_Type);
    AddToRoots(Locals_Type);
    AddToRoots(BoundFunction_Type);
    AddToRoots(InstanceMethod_Type);
    AddToRoots(ClassMethod_Type);
    AddToRoots(NativeFunction_Type);
    AddToRoots(NativeMethod_Type);
    AddToRoots(String_Type);
    AddToRoots(Null_Type);
    AddToRoots(Boolean_Type);
    AddToRoots(Integer_Type);
    AddToRoots(Real_Type);
    AddToRoots(Enumerator_Type);
    AddToRoots(Property_Type);
    AddToRoots(Error_Type);
    AddToRoots(RuntimeError_Type);
    AddToRoots(TypeError_Type);
    AddToRoots(ReferenceError_Type);
    AddToRoots(ArithmeticError_Type);
    AddToRoots(IndexError_Type);
    AddToRoots(SystemError_Type);
    AddToRoots(AssertError_Type);
    AddToRoots(OverflowError_Type);
    AddToRoots(SyntaxError_Type);
    AddToRoots(Type_Type);
    AddToRoots(emptyString);
    AddToRoots(dot_String);
    
    AddToRoots(missing_String);    
    AddToRoots(OpDispose_String);
    AddToRoots(OpUse_String);
    AddToRoots(loading_String);
    AddToRoots(Object_String);
    AddToRoots(length_String);
    AddToRoots(Enumerator_String);
    AddToRoots(Property_String);
    AddToRoots(userdata_String);
    AddToRoots(null_String);
    AddToRoots(values_String);
    AddToRoots(keys_String);
    AddToRoots(elements_String);
    AddToRoots(indices_String);
    AddToRoots(Array_String);
    
    AddToRoots(toNumber_String);
    AddToRoots(toString_String);
    AddToRoots(toInteger_String);
    AddToRoots(toReal_String);
    AddToRoots(toBoolean_String);
    AddToRoots(true_String);
    AddToRoots(false_String);
    AddToRoots(message_String);
    
    for (size_t i = 0 ; i < NUM_OVERRIDES ; ++i)
    {
        AddToRoots(override_strings[i]);
    }
}

void Engine::ScanRoots(Collector *c)
{
    paths->Mark(c);
    
    for (ModuleIterator iter = modules.Begin(); iter != modules.End(); ++iter)
    {
        c->ForceToGray(*iter);
    }
    
    // All script are root objects.
    
    for (ScriptIterator iter = scripts.Begin(); iter != scripts.End(); ++iter)
    {
        c->ForceToGray(*iter);
    }
}

void Engine::SweepStringTable() { string_table->Sweep(); }

void Engine::ChangeContext(Context* t)
{
    gc->ChangeContext(t);
    active_context = t;
}

String* Engine::GetTypenameOf(Value& v)
{
    switch (v.tag)
    {
    case TAG_null:       return Null_Type->GetName();
    case TAG_boolean:    return Boolean_Type->GetName();
    case TAG_integer:    return Integer_Type->GetName();
    case TAG_real:       return Real_Type->GetName();
    
    case TAG_string:     return String_Type->GetName();
    case TAG_enumerator: return AllocString("Enumerator"); // TODO: we need the subclasses of enumerator
    case TAG_property:   return Property_Type->GetName();
    case TAG_userdata:
    {
        ASSERT(v.val.userdata);
        UserData* ud = v.val.userdata;
        if (ud->GetInfo() && ud->GetInfo()->name)
            return AllocString(ud->GetInfo()->name);
        return userdata_String;
    }
    case TAG_object:
    {
        ASSERT(v.val.object);
        
        Object* obj     = v.val.object;
        Type*   objType = obj->GetType();
        if (objType)
            return objType->GetName();
    }
    }
    return this->emptyString;
}

Context* Engine::AllocContext()
{
    Context* c = Context::Create(this, this->Context_Type);
    return c;
}

}// pika
