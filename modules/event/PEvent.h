#ifndef PIKA_EVENT_HEADER
#define PIKA_EVENT_HEADER

namespace pika {
    struct Event : Object {
        PIKA_DECL(Event, Object);
        
        Event(Engine* enging, Type* type);
        
        virtual ~Event();
        
        static void Constructor(Engine*, Type*, Value&);
        static Event* StaticNew(Engine* eng, Type* type);
        void    Add(preal_t secs=0.0);
        void    Destroy();
        void    OnCallback(evutil_socket_t fd, short what);
                
        Value   GetCallback() { return this->callback; }
        void    SetCallback(Value);
        void    Active(pint_t);
        bool    Pending(pint_t events, preal_t timeout=0.0);
        bool    PrioritySet(pint_t);
                
        virtual void Init(Context*);
        virtual void MarkRefs(Collector* c);
        void SetFile(Value f);
        
        bool    inRoots;
        Value   callback;
        Value   file;
        pint_t  fd;
        event*  event;
    };
}// pika
#endif
