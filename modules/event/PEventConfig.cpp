#include "PEventModule.h"

namespace pika {

EventConfig::EventConfig(Engine* engine, Type* type): Object(engine, type), config(0)
{
    this->config = event_config_new();
}

EventConfig::~EventConfig()
{
    if (this->config)
    {
        event_config_free(this->config);
    }
}
    
void EventConfig::Init(Context* ctx)
{
    
}

EventConfig* EventConfig::StaticNew(Engine* eng, Type* type)
{
    EventConfig* eventConfig = 0;
    GCNEW(eng, EventConfig, eventConfig, (eng, type));
    return eventConfig;
}

void EventConfig::Constructor(Engine* eng, Type* obj_type, Value& res)
{
    Object* eventConfig = EventConfig::StaticNew(eng, obj_type);
    res.Set(eventConfig);
}

pint_t EventConfig::RequireFeature(pint_t feat)
{
    return event_config_require_features(this->config, feat);
}

pint_t EventConfig::SetFlag(pint_t flag)
{
    return event_config_set_flag(this->config, flag);
}

pint_t EventConfig::SetNumCpusHint(pint_t hint)
{
    return event_config_set_num_cpus_hint(this->config, hint);
}

pint_t EventConfig::AvoidMethod(String* meth)
{
    return event_config_avoid_method(this->config, meth->GetBuffer());
}

pint_t EventConfig::SetMaxDispatchInterval(preal_t interval, pint_t max_cb, pint_t min_priority)
{
#if LIBEVENT_V21
    timeval tv;
    Pika_MakeTimeval(interval, tv);
    
    return event_config_set_max_dispatch_interval(this->config, &tv, max_cb, min_priority);
#else
    return -1;
#endif
}

PIKA_IMPL(EventConfig)

}// pika

using namespace pika;

void Initialize_EventConfig(Package* event, Engine* eng)
{
    String* EventConfig_String = eng->AllocString("EventConfig");
    Type*   EventConfig_Type   = Type::Create(eng, EventConfig_String, eng->Object_Type, EventConfig::Constructor, event);
    event->SetSlot(EventConfig_String, EventConfig_Type);
    eng->AddBaseType(EventConfig_String, EventConfig_Type);
        
    SlotBinder<EventConfig>(eng, EventConfig_Type)
    .Method(&EventConfig::RequireFeature,           "requireFeature")
    .Method(&EventConfig::SetFlag,                  "setFlag")
    .Method(&EventConfig::SetNumCpusHint,           "setNumCpusHint")
    .Method(&EventConfig::AvoidMethod,              "avoidMethod")
    .Method(&EventConfig::SetMaxDispatchInterval,   "setMaxDispatchInterval")
    ;
}