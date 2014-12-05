#ifndef PIKA_EVENTBASE_HEADER
#define PIKA_EVENTBASE_HEADER

namespace pika {
    struct EventBase : Object {
        PIKA_DECL(EventBase, Object);
        
        EventBase(Engine* enging, Type* type);
        
        virtual ~EventBase();
        
        static void Constructor(Engine*, Type*, Value&);
        static EventBase* StaticNew(Engine* eng, Type* type);
        virtual void MarkRefs(Collector*);
        void    AddEvent(Event*);
        Array*  GetEvents() { return this->events; }
        String* GetMethod();
        pint_t  GetFeatures();
        bool    PriorityInit(pint_t);
        bool    Reinit();
        pint_t    LoopRun(pint_t);
        pint_t    Dispatch();
        pint_t    LoopBreak();
        pint_t    LoopExit(preal_t);
        pint_t    LoopContinue();
        bool    GotBreak();
        bool    GotExit();
        
        virtual void Init(Context*);
        
        virtual event_base* _GetBase() const { return base; }
protected:
        Array* events;
        event_base* base;
    };
}// pika
#endif
