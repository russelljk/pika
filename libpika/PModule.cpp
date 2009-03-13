/*
 *  PModule.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PModule.h"
#include "PString.h"
#include "PPlatform.h"

namespace pika
{
PIKA_IMPL(Module)

Module::Module(Engine* eng, Type* moduleType,
        String* name, String* p, String* load_nm, String* ver)
        : ThisSuper(eng, moduleType, name, eng->GetWorld()),
        result(0),
        path(p),
        entryname(load_nm),
        entry(0),    
        handle(0)
{
    Initialize(ver);
}

Module::~Module() { ASSERT(!this->handle); }

bool Module::Finalize()
{
    this->path = 0;
    this->entryname = 0;
    this->entry = 0;
    this->result = 0;
    return false;
}

void Module::Initialize(String* ver)
{
    this->entry = 0;

    if (this->path && this->path->GetLength() != 0)
    {
        this->handle = Pika_OpenShared(this->path->GetBuffer());
    
        if (this->handle)
        {
            // TODO: VersionFn_t from an unknown dll is not safe.
            
            VersionFn_t ver_fn = (VersionFn_t)Pika_GetSymbolAddress(this->handle, ver->GetBuffer());
                        
            // Under Mac OS, Windows or any OS where filenames are case insensitive -- failure to 
            // find the version function usually means the case of the library name is wrong
            // (ie import 'FileSys' instead of import 'filesys'). 
            
            if (!ver_fn)
            {
                RaiseException("Could not determine version of module %s.\nMake sure that you provided the correct name and path (case sensitive.)", 
                name->GetBuffer());
            }
            else if (StrCmp(ver_fn(), PIKA_VERSION_STR) != 0) // TODO: limit the length of the StrCmp.
            {
                RaiseException("Incorrect version for module %s. Required version %s.", this->name->GetBuffer(), PIKA_VERSION_STR);
            }
            
            this->entry = (ModuleEntry_t)Pika_GetSymbolAddress(this->handle, this->entryname->GetBuffer());
        }
    }
}

void Module::Shutdown()
{
    if (this->handle)
    {
        Pika_CloseShared(this->handle);
        this->handle = 0;
    }
}

Module* Module::Create(Engine* eng, String* name, String* path, String* fun, String* ver)
{
    Module* nl = 0;
    PIKA_NEW(Module, nl, (eng, eng->Module_Type, name, path, fun, ver));
    eng->AddModule(nl);
    eng->AddToGC(nl);
    return nl;
}

Object* Module::Clone() { return 0; }

void Module::MarkRefs(Collector* c)
{
    ThisSuper::MarkRefs(c);

    if (path)       path->Mark(c);        
    if (entryname)  entryname->Mark(c);
    if (result)     result->Mark(c);
}

bool Module::IsLoaded() const
{
    return this->handle != 0;
}

void Module::Run()
{
    if (entry)
        result = entry(this->GetEngine(), this);
}

Package* Module::ImportResult()
{
    return result ? result : this;
}

}//namespace pika

static void Module_newFn(Engine* eng, Type* type, Value& res)
{
    Module* m = Module::Create(eng, eng->emptyString, eng->emptyString, eng->emptyString, eng->emptyString);
    res.Set(m);
}

void InitModuleAPI(Engine* eng)
{
    eng->Module_Type = Type::Create(eng,
                                    eng->AllocString("Module"),
                                    eng->Package_Type,
                                    Module_newFn, eng->GetWorld());
}


