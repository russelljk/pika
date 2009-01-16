/*
 * PDef.cpp
 * See Copyright Notice in Pika.h
 */
#include "PConfig.h"
#include "PValue.h"
#include "PCollector.h"
#include "PBasic.h"
#include "PDef.h"
#include "PEngine.h"
#include "PString.h"
#include "PLiteralPool.h"

namespace pika
{

Bytecode::Bytecode(code_t* cd, u2 len)
    : code(0),
    length(0)
{
    code = (code_t*)Pika_malloc(sizeof(code_t) * len);
    Pika_memcpy(code, cd, sizeof(code_t) * len);
    length = len;
}

Bytecode::~Bytecode()
{
    Pika_free(code);
}

void Def::SetSource(Engine* eng, const char* buff, size_t len)
{
    source = eng->AllocString(buff, len);
    eng->GetGC()->WriteBarrier(this, source);
}

Def* Def::Create(Engine* eng)
{
    Def* fn = 0;
    PIKA_NEW(Def, fn, ());
    
    eng->AddToGC(fn);
    return fn;
}

Def* Def::CreateWith(Engine* eng, String* name, Nativecode_t fn, u2 argc, bool varargs, bool strict, Def* parent)
{
    Def* def = 0;
    PIKA_NEW(Def, def, (name, fn, argc, varargs, strict, parent));
    eng->AddToGC(def);
    return def;
}

Def::~Def()
{
    Pika_delete(bytecode);
}

int Def::OffsetOfLocal(const String* str) const
{
    Buffer<LocalVarInfo>::ConstIterator enditer = localsInfo.End();
    for (Buffer<LocalVarInfo>::ConstIterator iter = localsInfo.Begin(); iter != enditer; ++iter)
    {
        if ((*iter).name == str)
            return (int)localsInfo.IndexOf(iter);
    }
    return -1;
}

void Def::MarkRefs(Collector* c)
{
    if (parent)     parent->Mark(c);
    if (name)       name->Mark(c);
    if (source)     source->Mark(c);
    if (literals)   literals->Mark(c);
    
    for (size_t i = 0; i < localsInfo.GetSize(); ++i)
    {
        if (localsInfo[i].name)
        {
            localsInfo[i].name->Mark(c);
        }
    }
}

void Def::SetBytecode(code_t* bc, u2 len)
{
    if (bytecode)
        Pika_delete(bytecode);        
    PIKA_NEW(Bytecode, bytecode, (bc, len));
}

void Def::AddLocalVar(Engine* eng, const char* name)
{
    LocalVarInfo lv;
    lv.name = eng->AllocString(name);
    lv.beg  = 0;
    lv.end  = 0;
    localsInfo.Push(lv);
    eng->GetGC()->WriteBarrier(this, lv.name);
}

void Def::SetLocalRange(size_t local, size_t start, size_t end)
{
    ASSERT(local < localsInfo.GetSize());
    ASSERT(end >= start);
    
    localsInfo[local].beg = (ptrdiff_t)start;
    localsInfo[local].end = (ptrdiff_t)end;
}

}// pika

