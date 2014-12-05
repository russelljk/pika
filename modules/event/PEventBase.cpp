#include "PEventModule.h"

namespace pika {

EventBase::EventBase(Engine* engine, Type* type): Object(engine, type), base(0), events(0)
{
}

EventBase::~EventBase()
{
    if (this->base)
    {
        event_base_free(this->base);
    }
}

void EventBase::AddEvent(Event* event)
{
    if (!events) {
        GCPAUSE_NORUN(engine);
        events = Array::Create(engine, engine->Array_Type, 0, 0);
        WriteBarrier(events);
    }
    
    Value val(event);
    events->Push(val);
    WriteBarrier(event);
}

void EventBase::MarkRefs(Collector* c)
{
    ThisSuper::MarkRefs(c);
    if (events) {
        events->Mark(c);
    }
}

void EventBase::Init(Context* ctx)
{
    trace0("EventBase Init\n");
    u2 argc = ctx->GetArgCount();
    
    if (argc == 1) {
        EventConfig* config = ctx->GetArgT<EventConfig>(0);
        this->base = event_base_new_with_config(config->_GetConfig());
    }
    else if (argc == 0) {
        this->base = event_base_new();
    } else {
        ctx->WrongArgCount();
    }
}

EventBase* EventBase::StaticNew(Engine* eng, Type* type)
{
    EventBase* eventBase = 0;
    GCNEW(eng, EventBase, eventBase, (eng, type));
    return eventBase;
}

void EventBase::Constructor(Engine* eng, Type* obj_type, Value& res)
{
    Object* eventBase = EventBase::StaticNew(eng, obj_type);
    res.Set(eventBase);
}

String* EventBase::GetMethod()
{
    if (this->base) {
        const char* method = event_base_get_method(this->base);
        if (method) {
            return this->engine->GetString(method);
        }
    }
    return this->engine->emptyString;
}

pint_t EventBase::GetFeatures()
{
    if (!this->base)
        return 0;
    return event_base_get_features(this->base);    
}

bool EventBase::PriorityInit(pint_t n)
{
    if (!this->base)
        return false;
    return event_base_priority_init(this->base, n) == 0;
}

bool EventBase::Reinit()
{
    if (!this->base)
        return false;
    return event_reinit(this->base) == 0;
}

pint_t EventBase::LoopRun(pint_t flags)
{
    if (!this->base)
        return 1;
    return event_base_loop(this->base, flags);
}

pint_t EventBase::Dispatch()
{
    if (!this->base)
        return 1;
    int res = event_base_dispatch(this->base);
    return res;
}

pint_t EventBase::LoopBreak()
{
    if (!this->base)
        return 1;
    return event_base_loopbreak(this->base);
}

pint_t EventBase::LoopExit(preal_t secs)
{
    if (!this->base)
        return 1;
    timeval tv;
    Pika_MakeTimeval(secs, tv);
    return event_base_loopexit(this->base, &tv);
}

bool EventBase::GotExit()
{
    if (!this->base)
        return false;
    return event_base_got_exit(this->base) != 0;
}

bool EventBase::GotBreak()
{
    if (!this->base)
        return false;
    return event_base_got_break(this->base) != 0;
}

pint_t EventBase::LoopContinue()
{
    if (!this->base)
        return 1;
#if LIBEVENT_V212
    return event_base_loopcontinue(this->base);
#else
    RaiseException(Exception::ERROR_runtime,
        "EventBase.loopContinue is only availible in Libevent 2.1.2-alpha and up.");
    return 1;
#endif    
}

PIKA_IMPL(EventBase)

}// pika

using namespace pika;

void Initialize_EventBase(Package* event, Engine* eng)
{
    String* EventBase_String = eng->AllocString("EventBase");
    Type*   EventBase_Type   = Type::Create(eng, EventBase_String, eng->Object_Type, EventBase::Constructor, event);
    event->SetSlot(EventBase_String, EventBase_Type);
    eng->AddBaseType(EventBase_String, EventBase_Type);
    
    SlotBinder<EventBase>(eng, EventBase_Type)
    .PropertyR("events",
            &EventBase::GetEvents,      "getEvents")
    .Method(&EventBase::GetMethod,      "getMethod")
    .Method(&EventBase::GetFeatures,    "getFeatures")
    .Method(&EventBase::PriorityInit,   "priorityInit")
    .Method(&EventBase::Reinit,         "reinit")
    .Method(&EventBase::LoopRun,        "loopRun")
    .Method(&EventBase::Dispatch,       "dispatch")
    .Method(&EventBase::LoopBreak,      "loopBreak")
    .Method(&EventBase::LoopExit,       "loopExit")
    .Method(&EventBase::LoopContinue,   "loopContinue")
    .Method(&EventBase::GotBreak,       "gotBreak")
    .Method(&EventBase::GotExit,        "gotExit")
    ;
}
