/*
 *  PNativeBind.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PEngine.h"
namespace pika {

// NativeDef

NativeDef::NativeDef(NativeMethodBase* d) : def(d) { retc = def->GetRetCount(); }

NativeDef::~NativeDef() { delete def; }

// HookedFunction

PIKA_IMPL(HookedFunction)

HookedFunction::HookedFunction(Engine* eng, Type* ptype, Def* mdef,
                               NativeDef* ndef, ClassInfo* info, Package* pkg)
        : ThisSuper(eng, ptype, mdef, pkg, 0),
        ndef(ndef),
        info(info)
{}

HookedFunction::HookedFunction(const HookedFunction* rhs) : 
        ThisSuper(rhs),
        ndef(rhs->ndef),
        info(rhs->info)
{
}

HookedFunction::~HookedFunction() {}

void HookedFunction::Invoke(void* obj, Context* ctx) { ndef->def->Invoke(obj, ctx); }

HookedFunction* HookedFunction::Create(Engine* eng,
                                       Type* type,
                                       Def* def,
                                       NativeMethodBase* ndef,
                                       ClassInfo* info,
                                       Package* pkg)
{
    HookedFunction* bm = 0;
    NativeDef* cnf;
    PIKA_NEW(NativeDef, cnf, (ndef));
    PIKA_NEW(HookedFunction, bm, (eng, type, def, cnf, info, pkg));
    eng->AddToGCNoRun(cnf);
    eng->AddToGCNoRun(bm);
    return bm;
}

Object* HookedFunction::Clone()
{
    HookedFunction* bf = 0;    
    GCNEW(engine, HookedFunction, bf, (this));        
    return bf;
}

void HookedFunction::MarkRefs(Collector* c)
{
    Function::MarkRefs(c);
    if (ndef) ndef->Mark(c);
}

Function* HookedFunction::BindWith(Value& withobj)
{
    Function* c = 0;
    GCNEW(engine, BoundHookedFunction, c, (this, withobj));
    return c;
}

String* HookedFunction::GetDotPath()
{
    if (!info)
        return Function::GetDotPath();
        
    GCPAUSE_NORUN(engine);
    String* res = String::Concat(location->GetDotName(), engine->dot_String);
    res = String::Concat(res, engine->AllocString( info->GetName() ));
    res = String::Concat(res, engine->dot_String);
    res = String::Concat(res, def->name);
    return res;
}

int HookedFunction::Hook(Context* ctx, Value& self)
{
    HookedFunction* bm = static_cast<HookedFunction*>(ctx->GetFunction());
    
    if (!self.IsDerivedFrom(bm->info)) {
        RaiseException(Exception::ERROR_type,
                       "%s: %s requires self object derived from type %s.",
                       bm->GetType()->GetName()->GetBuffer(),
                       bm->GetDotPath()->GetBuffer(),
                       bm->info->GetName());
    }
    
    Basic* obj = self.val.basic;
    bm->Invoke(obj, ctx);
    return bm->GetRetCount();
}

BoundHookedFunction::BoundHookedFunction(const BoundHookedFunction* f) : ThisSuper(f), selfobj(f->selfobj) {}

Object* BoundHookedFunction::Clone()
{
    BoundHookedFunction* bf = 0;    
    GCNEW(engine, BoundHookedFunction, bf, (this));        
    return bf;
}

int HookedFunction::StaticHook(Context* ctx, Value& self)
{
    HookedFunction* bm = static_cast<HookedFunction*>(ctx->GetFunction());
    bm->Invoke(0, ctx);
    return bm->GetRetCount();
}

PIKA_IMPL(BoundHookedFunction)

void BoundHookedFunction::MarkRefs(Collector* c)
{
    ThisSuper::MarkRefs(c);
    MarkValue(c, selfobj);
}

void BoundHookedFunction::BeginCall(Context* ctx)
{
    ctx->SetSelf(selfobj);
    Function::BeginCall(ctx);
}

}// pika
