
#include "Pika.h"
#include "PIterator.h"

namespace pika {
PIKA_IMPL(Iterator)

Iterator::Iterator(Engine* eng, Type* typ) : ThisSuper(eng, typ)
{
}

Iterator::~Iterator()
{
}

bool Iterator::ToBoolean()
{
    return false;
}

int Iterator::Next(Context*)
{
    return 0;
}

Iterator* Iterator::Create(Engine* eng, Type* typ)
{
    Value res(NULL_VALUE);
    Iterator::Constructor(eng, typ, res);
    return (Iterator*)res.val.object;
}

void Iterator::Constructor(Engine* eng, Type* type, Value& res)
{
    Iterator* iter = 0;
    PIKA_NEW(Iterator, iter, (eng, type));
    eng->AddToGCNoRun(iter);
}
namespace {

int Iterator_next(Context* ctx, Value& self)
{
    Iterator* iter = (Iterator*)self.val.object;
    return iter->Next(ctx);
}

}
void Iterator::StaticInitType(Engine* engine)
{
    GCPAUSE_NORUN(engine);
    Package* Pkg_World = engine->GetWorld();
    String* Iterator_String = engine->AllocString("Iterator");
    engine->Iterator_Type = Type::Create(engine, Iterator_String, engine->Object_Type, Iterator::Constructor, Pkg_World);
    engine->Iterator_Type->SetAbstract(true);
    
    SlotBinder<Iterator>(engine, engine->Iterator_Type, Pkg_World)
    .Method(&Iterator::ToBoolean, "toBoolean")
    .Register(Iterator_next, "next", 0, false, true)
    ;
    
    Pkg_World->SetSlot(Iterator_String, engine->Iterator_Type);
}

}// pika
