/*
 *  PIterator.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PIterator.h"

namespace pika {

IterateHelper::IterateHelper(Iterator* iter, Context* ctx) :
    iterator(iter),
    context(ctx)
{

}
    
IterateHelper::~IterateHelper()
{

}

IterateHelper::operator bool()
{
    Value iter(iterator);
    
    context->SafePush(iter);
    bool b = context->GetEngine()->ToBoolean(context, iter);
    context->Pop();
    return b;
}

Value IterateHelper::Next()
{
    Value iter(iterator);
    Value res(NULL_VALUE);
    if (GetOverrideFrom(context->GetEngine(), iter.val.basic, OVR_next, res))
    {
        context->CheckStackSpace(3);
        context->Push(iter);
        context->Push(res);
        if (context->SetupCall(0, 1))
        {
            context->Run();
        }
        return context->PopTop();
    }
    return res;
}

PIKA_IMPL(Iterator)

Iterator::Iterator(Engine* eng, Type* typ) : ThisSuper(eng, typ)
{
}

Iterator::Iterator(Iterator* rhs) : ThisSuper(rhs)
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
    res.Set(iter);
}

namespace {

PIKA_DOC(Iterator_Type, "Iterators are objects that can enumerate over a range \
or sequence of values. Iterators are most commonly used by '''for''' loop.\n\
[[[for x in object\n\
  # ...\n\
end]]]\n\
Types can provide thier own Iterators by overriding the [Object.iterate] method."
)

PIKA_DOC(Iterator_toBoolean, "/()\
\n\
Returns true as long as the iterator is valid.\
")

PIKA_DOC(Iterator_next, "/()\
\n\
Moves the iterator to the next value in the sequence it covers. \
This function should be used in conjunction with [toBoolean] to ensure that \
the values returned are part of the sequence."
)

int Iterator_next(Context* ctx, Value& self)
{
    Iterator* iter = (Iterator*)self.val.object;
    return iter->Next(ctx);
}

}// namespace

void Iterator::StaticInitType(Engine* engine)
{
    GCPAUSE_NORUN(engine);
    Package* Pkg_World = engine->GetWorld();
    String* Iterator_String = engine->AllocString("Iterator");
    engine->Iterator_Type = Type::Create(engine, Iterator_String, engine->Object_Type, Iterator::Constructor, Pkg_World);
    engine->Iterator_Type->SetAbstract(true);
    
    SlotBinder<Iterator>(engine, engine->Iterator_Type)
    .Method(&Iterator::ToBoolean, "toBoolean", PIKA_GET_DOC(Iterator_toBoolean))
    .RegisterMethod(Iterator_next, "next", 0, false, true, PIKA_GET_DOC(Iterator_next))
    ;
    
    engine->Iterator_Type->SetDoc(engine->GetString(PIKA_GET_DOC(Iterator_Type)));
    Pkg_World->SetSlot(Iterator_String, engine->Iterator_Type);
}

}// pika
