/*
 *  PEventConfig.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_EVENTCONFIG_HEADER
#define PIKA_EVENTCONFIG_HEADER

namespace pika {
    struct EventConfig : Object {
        PIKA_DECL(EventConfig, Object);
        
        EventConfig(Engine* enging, Type* type);
        
        virtual ~EventConfig();
        
        event_config* _GetConfig() const { return this->config; }
        
        pint_t RequireFeature(pint_t);
        pint_t SetFlag(pint_t);
        pint_t SetNumCpusHint(pint_t);
        pint_t SetMaxDispatchInterval(preal_t interval, pint_t max_cb, pint_t min_priority);
        pint_t AvoidMethod(String*);
        
        static void Constructor(Engine*, Type*, Value&);
        static EventConfig* StaticNew(Engine* eng, Type* type);
        
        virtual void Init(Context*);
protected:
        event_config* config;
    };
}// pika
#endif
