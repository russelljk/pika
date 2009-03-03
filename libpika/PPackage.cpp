/*
 *  PPackage.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PValue.h"
#include "PString.h"
#include "PDef.h"
#include "PPackage.h"
#include "PFunction.h"
#include "PEngine.h"
#include "PContext.h"
#include "PNativeBind.h"

namespace pika
{

PIKA_IMPL(Package)

Package::Package(Engine* eng, Type* pkgType, String* n, Package* superPkg)
        : Object(eng, pkgType),
        name(n),
        dotName(0),
        superPackage(superPkg)
{
    SetName(n);
    SetSlot("__package", this, Slot::ATTR_internal);
}

Package::~Package() {}

Package* Package::GetSuper() { return superPackage; }

bool Package::GetSlot(const Value& key, Value& res)
{
    if (ThisSuper::GetSlot(key, res))
        return true;
        
    if (superPackage && superPackage->GetSlot(key, res))
        return true;
    return false;
}

bool Package::GetGlobal(const Value& key, Value& res)
{
    if (members.Get(key, res))
        return true;

    if (superPackage && superPackage->GetGlobal(key, res))
        return true;
    return false;
}

bool Package::SetGlobal(const Value& key, Value& val, u4 attr)
{
    if (!members.CanSet(key))
    {
        if (!(attr & Slot::ATTR_forcewrite))
        {
            return false;
        }
        else
        {
            attr &= ~Slot::ATTR_forcewrite;
        }
    }
    if (key.IsCollectible())
        WriteBarrier(key);
    if (val.IsCollectible())
        WriteBarrier(val);
    if (!members.Set(key, val, attr))
    {
        return false;
    }
    return true;
}

void Package::AddNative(RegisterFunction* fns, size_t count)
{
    GCPAUSE_NORUN(engine);
    for (size_t i = 0; i < count; ++i)
    {
        String* funname = engine->AllocString(fns[i].name);
        
        Def* fn = Def::CreateWith(engine, funname, fns[i].code, fns[i].argc,
                                                  fns[i].varargs, fns[i].strict, 0);
                                                  
        Value vname(funname);
        Value vfn(fn);
        
        Function* closure = Function::Create(engine, fn, this);
        Value vframe;
        vframe.Set(closure);
        
        SetSlot(vname, vframe);
    }
}

String* Package::GetName() { return name ? name : engine->emptyString; }

String* Package::GetDotName() { return dotName ? dotName : engine->emptyString; }

String* Package::ToString()
{
    return String::ConcatSpace(type->GetName(), this->GetDotName());
}

Package* Package::Create(Engine* eng, String* name, Package* superPkg)
{
    Package *pkg = 0;
    PIKA_NEW(Package, pkg, (eng, eng->Package_Type, name, superPkg));
    eng->AddToGC(pkg);
    return pkg;
}

void Package::MarkRefs(Collector* c)
{
    ThisSuper::MarkRefs(c);
    
    if (name) name->Mark(c);
    if (dotName) dotName->Mark(c);
    if (superPackage) superPackage->Mark(c);
}

void Package::Init(Context* ctx)
{
    u2 argc = ctx->GetArgCount();
    
    if (argc == 1)
    {
        String* nstr = ctx->GetStringArg(0);
        SetName(nstr);
    }
    else if (argc == 2)
    {
        String* nstr = ctx->GetStringArg(0);
        Object* super = ctx->GetObjectArg(1);
        
        if (!super->IsDerivedFrom(Package::StaticGetClass()))
        {
            RaiseException(Exception::ERROR_type, "super must be a package");
        }
        superPackage = (Package*)super;
        WriteBarrier(superPackage);
        this->SetName(nstr);
    }
    else if (argc != 0)
    {
        ctx->WrongArgCount();
    }
}

void Package::SetName(String* n)
{
    this->name = n;
    this->dotName = name;
    WriteBarrier(this->name);
    
    if (superPackage)
    {
        String*  supername = superPackage->dotName;
        String*  superdot  = String::Concat(supername, engine->AllocString("."));
        
        this->dotName = String::Concat(superdot, this->name);
        WriteBarrier(this->dotName);
    }
}

Object* Package::Clone()
{
    Package* superpkg = Package::GetSuper();
    Package* pkg = Create(engine, name, superpkg);    
    return pkg;
}

}// pika

void Package_NewFn(Engine* eng, Type* obj_type, Value& res)
{
    Package* pkg = Package::Create(eng, eng->emptyString, 0);
    res.Set(pkg);
}

int Pkg_getParent(Context* ctx, Value& self)
{
    GETSELF(Package, pkg, "Package")
    ctx->Push(pkg->GetSuper());
    return 1;
}

int Pkg_getName(Context* ctx, Value& self)
{
    GETSELF(Package, pkg, "Package")
    ctx->Push(pkg->GetName());
    return 1;
}

int Pkg_getPath(Context* ctx, Value& self)
{
    GETSELF(Package, pkg, "Package")
    ctx->Push(pkg->GetDotName());
    return 1;
}

static int Package_getGlobal(Context* ctx, Value& self)
{
    Package* pkg = self.val.package;
    Value& arg0  = ctx->GetArg(0);
    Value res(NULL_VALUE);

     // TODO: call properties

    if (pkg->GetGlobal(arg0, res))
    {
        ctx->Push(res);
    }
    else
    {
        ctx->PushNull();
    }
    return 1;
}

static int Package_setGlobal(Context* ctx, Value& self)
{
    Package* pkg  = self.val.package;
    Value&   arg0 = ctx->GetArg(0);
    Value&   arg1 = ctx->GetArg(1);

     // TODO: call properties

    if (pkg->GetGlobal(arg0, arg1))
    {
        ctx->PushTrue();
    }
    else
    {
        ctx->PushFalse();
    }
    return 1;
}

static int Package_hasGlobal(Context* ctx, Value& self)
{
    Package* pkg = (Package*)self.val.object;
    Value&  arg0 = ctx->GetArg(0);
    Value   res(NULL_VALUE);
    
    if (pkg->GetSlot(arg0, res))
    {
        ctx->PushTrue();
    }
    else
    {
        ctx->PushFalse();
    }
    return 1;
}

static int Package_getName(Context* ctx, Value& self)
{
    Package* pkg = self.val.package;
    ctx->Push(pkg->GetName());
    return 1;
}

static int Package_openPath(Context* ctx, Value&)
{
    String*  path  = 0;
    Package* where = 0;
    bool     write = false;
    u4       argc  = ctx->GetArgCount();
    Engine*  eng   = ctx->GetEngine();
    
    GCPAUSE(eng);
    
    switch (argc)
    {
    case 3: write = ctx->GetBoolArg(2);
    case 2: where = ctx->GetArg(1).IsNull() ? 0 : ctx->GetArgT<Package>(1);
    case 1: path  = ctx->GetStringArg(0);
            break;
    default:
        ctx->WrongArgCount();
        return 0;
    }
    
    Package* pkg = eng->OpenPackageDotPath(path, where, write);
    ctx->Push(pkg);
    return 1;
}

void InitPackageAPI(Engine* eng)
{
    Package* Pkg_World = eng->GetWorld();
    String* pkgstr = eng->AllocString("Package");
    
    Pkg_World->SetType(eng->Package_Type);
    eng->Type_Type->SetType(eng->Package_Type->GetType());
        
    eng->Module_Type = Type::Create(eng,
                                    eng->AllocString("Module"),
                                    eng->Package_Type,
                                    0, Pkg_World);
                                    
    eng->Script_Type = Type::Create(eng,
                                    eng->AllocString("Script"),
                                    eng->Package_Type,
                                    0, Pkg_World);                                    
    
    static RegisterFunction Package_methods[] =
    {
        { "getGlobal", Package_getGlobal, 1, 0, 1 },
        { "setGlobal", Package_setGlobal, 2, 0, 1 },
        { "hasGlobal", Package_hasGlobal, 1, 0, 1 },
        { "getName",   Package_getName,   0, 0, 1 },
    };
    
    static RegisterFunction Package_classMethods[] =
    {
        { "openPath", Package_openPath, 0, 1, 0 },
    };
    
    static RegisterProperty Pkg_Properties[] =
    {
        { "parent", Pkg_getParent, "getParent", 0, 0 },
        { "name",   Pkg_getName,   "getName",   0, 0 },
        { "path",   Pkg_getPath,   "getPath",   0, 0 },
    };
    
    eng->Package_Type->EnterMethods(Package_methods, countof(Package_methods));
    eng->Package_Type->EnterClassMethods(Package_classMethods, countof(Package_classMethods));
    eng->Package_Type->EnterProperties(Pkg_Properties, countof(Pkg_Properties));
    
    Pkg_World->SetSlot(pkgstr, eng->Package_Type);
}


