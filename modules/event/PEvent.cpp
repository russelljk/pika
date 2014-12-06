#include "PEventModule.h"

namespace pika {

void EventCallbackMethod(evutil_socket_t fd, short what, void* arg)
{    
    if (arg)
    {
        Event* event = (Event*)arg;
        event->OnCallback(fd, what);
    }
}

Event::Event(Engine* engine, Type* type): 
    Object(engine, type), 
    inRoots(false), 
    callback(NULL_VALUE),
    file(NULL_VALUE),
    fd(-1),
    event(0)
{
}

Event::~Event()
{
    if (event)
    {
        event_free(event);
    }
}

void Event::Destroy()
{
    if (this->event)
    {
        event_del(this->event);
        this->event = 0;
    }
    
    if (this->inRoots) {
        this->inRoots = false;
        this->engine->GetGC()->RemoveAsRoot(this);
    }
}

void Event::SetFile(Value f)
{
    this->file = f;
    WriteBarrier(this->file);
}

void Event::OnCallback(evutil_socket_t cbfd, short what)
{
    if (cbfd == this->fd)
    {
        Context* ctx = engine->GetActiveContext();
        ctx->CheckStackSpace(4);
        ctx->Push(file);
        ctx->Push((pint_t)what);
        ctx->PushNull();
        ctx->Push(this->callback);
        if (ctx->SetupCall(2))
        {
            ctx->Run();
        }
        ctx->Pop();
    }
}

void Event::SetCallback(Value cb)
{
    this->callback = cb;
    WriteBarrier(this->callback);
}

void Event::Init(Context* ctx)
{
    EventBase* base = ctx->GetArgT<EventBase>(0);
    this->SetFile(ctx->GetArg(1));
    pint_t what = ctx->GetIntArg(2);
    SetCallback(ctx->GetArg(3));

    this->fd = GetFilenoFrom(ctx, this->file);    
    this->event = event_new(base->_GetBase(), this->fd, what, EventCallbackMethod, (void*)this);
    base->AddEvent(this);    
    // Handle null event here.
}

void Event::MarkRefs(Collector* c)
{
    ThisSuper::MarkRefs(c);
    MarkValue(c, callback);
    MarkValue(c, file);
}

Event* Event::StaticNew(Engine* eng, Type* type)
{
    Event* ev = 0;
    GCNEW(eng, Event, ev, (eng, type));
    return ev;
}

void Event::Constructor(Engine* eng, Type* obj_type, Value& res)
{
    Object* ev = Event::StaticNew(eng, obj_type);
    res.Set(ev);
}

void Event::Add(preal_t secs)
{
    if (!this->inRoots)
    {
        this->inRoots = true;
        engine->AddToRoots(this);
        if (!secs) {
            event_add(this->event, 0);
        } else {
            timeval tv;
            Pika_MakeTimeval(secs, tv);
            event_add(this->event, &tv);
        }
    }   
}

void Event::Active(pint_t res)
{
    if (this->event) {
        event_active(this->event, res, 0);
    }
}

bool Event::PrioritySet(pint_t p)
{
    if (!this->event) {
        return false;
    }
    return event_priority_set(this->event, p) < 0;
}

bool Event::Pending(pint_t events, preal_t timeout)
{
    if (!this->event) {
        return false;
    }
    timeval tv;
    Pika_MakeTimeval(timeout, tv);
    return event_pending(this->event, events, &tv);
}

PIKA_IMPL(Event)

}// pika

using namespace pika;

int Event_add(Context* ctx, Value& self)
{
    Event* event = static_cast<Event*>(self.val.object);
    u2 argc = ctx->GetArgCount();
    preal_t secs = 0.0;
    if (argc == 1)
    {
        secs = ctx->GetRealArg(0);
    }
    else if (argc != 0)
    {
        ctx->WrongArgCount();
    }
    event->Add(secs);
    return 0;
}

int Event_pending(Context* ctx, Value& self)
{
    Event* event = static_cast<Event*>(self.val.object);
    u2 argc = ctx->GetArgCount();
    preal_t secs = 0.0;
    if (argc == 2)
    {
        secs = ctx->GetRealArg(1);
    }
    else if (argc != 1)
    {
        ctx->WrongArgCount();
    }
    pint_t events = ctx->GetIntArg(0);
    event->Pending(events, secs);
    return 0;
}

void Initialize_Event(Package* event, Engine* eng)
{
    String* Event_String = eng->AllocString("Event");
    Type*   Event_Type   = Type::Create(eng, Event_String, eng->Object_Type, Event::Constructor, event);
    event->SetSlot(Event_String, Event_Type);
    eng->AddBaseType(Event_String, Event_Type);
    
    SlotBinder<Event>(eng, Event_Type)
    .Method(&Event::Destroy,        "del")
    .Register(Event_add,            "add",      0, true, false)
    .Register(Event_pending,        "pending",  0, true, false)
    .Method(&Event::Active,         "active")
    .Method(&Event::PrioritySet,    "prioritySet")
    .PropertyRW("callback",
        &Event::GetCallback,        "getCallback",
        &Event::SetCallback,        "setCallback")
    ;
}