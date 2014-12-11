/*
 *  PEventModule.cpp
 *  See Copyright Notice in Pika.h
 */
#include "PEventModule.h"

using namespace pika;

extern void Initialize_Event(Package*, Engine*);
extern void Initialize_EventBase(Package*, Engine*);
extern void Initialize_EventConfig(Package*, Engine*);

#if defined(PIKA_WIN)
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH: break;
    }
    return TRUE;
}

#endif

// event_get_version()

int event_supportedMethods(Context* ctx, Value&)
{
    const char** methods = event_get_supported_methods();
    Engine* engine = ctx->GetEngine();
    GCPAUSE(engine);
    int retc = 0;
    for (size_t i = 0; methods[i]; ++i)
    {
        ctx->Push(engine->AllocString(methods[i]));
        ++retc;
    }
    return retc;
}

int event_getVersion(Context* ctx, Value&)
{
    Engine* engine = ctx->GetEngine();
    const char* version = event_get_version();
    
    if (version) {
        ctx->Push(engine->AllocString(version));
    } else {
        ctx->Push(engine->emptyString);
    }
    return 1;
}

PIKA_MODULE(event, eng, event)
{    
    // event_set_mem_functions(Pika_malloc, Pika_realloc, Pika_free);
    GCPAUSE_NORUN(eng);
    
    static NamedConstant EventFeature_Constants[] = {
        { "EV_FEATURE_ET", EV_FEATURE_ET },
        { "EV_FEATURE_O1", EV_FEATURE_O1 },
        { "EV_FEATURE_FDS", EV_FEATURE_FDS },
    };
    
    Basic::EnterConstants(event, EventFeature_Constants, countof(EventFeature_Constants));
    
    static NamedConstant Event_Constants[] = {
        { "EV_TIMEOUT", EV_TIMEOUT },
        { "EV_READ", EV_READ },
        { "EV_WRITE", EV_WRITE },
        { "EV_SIGNAL", EV_SIGNAL },
        { "EV_PERSIST", EV_PERSIST },
        { "EV_ET", EV_ET },
    };
    
    Basic::EnterConstants(event, Event_Constants, countof(Event_Constants));
    
    static NamedConstant EventLoop_Constants[] = {
        { "EVLOOP_ONCE", EVLOOP_ONCE },
        { "EVLOOP_NONBLOCK", EVLOOP_NONBLOCK },
#if LIBEVENT_V21
        { "EVLOOP_NO_EXIT_ON_EMPTY", EVLOOP_NO_EXIT_ON_EMPTY },
#endif
    };
    
    Basic::EnterConstants(event, EventLoop_Constants, countof(EventLoop_Constants));
    
    static NamedConstant EventBaseConfig_Constants[] = {
        { "EVENT_BASE_FLAG_NOLOCK", EVENT_BASE_FLAG_NOLOCK },
        { "EVENT_BASE_FLAG_IGNORE_ENV", EVENT_BASE_FLAG_IGNORE_ENV },
        { "EVENT_BASE_FLAG_STARTUP_IOCP", EVENT_BASE_FLAG_STARTUP_IOCP },
        { "EVENT_BASE_FLAG_NO_CACHE_TIME", EVENT_BASE_FLAG_NO_CACHE_TIME },
        { "EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST", EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST },
#if LIBEVENT_V21
        { "EVENT_BASE_FLAG_PRECISE_TIMER", EVENT_BASE_FLAG_PRECISE_TIMER },
#endif
    };    
    
    Basic::EnterConstants(event, EventBaseConfig_Constants, countof(EventBaseConfig_Constants));    
    
    static RegisterFunction event_Functions[] = {
        { "supportedMethods",   event_supportedMethods, 0, DEF_STRICT, 0 },
        { "getVersion",         event_getVersion,       0, DEF_STRICT, 0 },
    };
    
    event->EnterFunctions(event_Functions, countof(event_Functions));
    
    Initialize_Event(event, eng);
    Initialize_EventBase(event, eng);
    Initialize_EventConfig(event, eng);
    return event;
}