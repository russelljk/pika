/*
 *  PHooks.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_HOOKS_HEADER
#define PIKA_HOOKS_HEADER

namespace pika {
class String;
class Engine;
class Context;
class Function;
class Object;
class Package;

struct IHook;

enum HookEvent {
    HE_call,        //!< Bytecode function is called
    HE_return,      //!< Return from a bytecode function call.
    HE_yield,       //!< Context yields.
    HE_nativeCall,  //!< C/C++ native function is called
    HE_instruction, //!< A bytecode instruction is about to be executed.
    HE_except,      //!< Exception has been raised.
    HE_import,      //!< Call to import has been made.
    HE_max,
};

struct HookEntry {
    IHook*      hook;
    HookEntry*  next;
};

/**
 *  Hook interface used by Pika. Hooks are called at certain points during a programs execution
 *  and can be used to provide debuggers, tracing, statistics and importing as well as other things.
 *  Hooks are registered by calling Engine::AddHook with the HookEvent the hook responds to.
 *  Hooks can be registered to multiple events, just beware that release is called once (in order)
 *  for every event the hook is registered to (see IHook::Release).
 */
struct PIKA_API IHook {
    virtual ~IHook() = 0;
    
    /**
     *  Called in response to an event. 
     *
     *  @param ev       [in] The event type
     *  @param data     [in] Event specific data
     *  @result         true if the event was handled and the next hook should not be called. false The 
     *                  next hook should be called.
     */
    virtual bool OnEvent(HookEvent, void* data) = 0;
    
    /**
     *  Called when the Engine is finished with the hook. 
     *  If the hook is registered for multiple events you should wait until the last Release is
     *  called before you delete this hook. Events are released in order so that the HE_call hook 
     *  event is released first followed by HE_return all the way to the last event HE_import. 
     *  Release will only be called for events this hook was registered with.
     */
    virtual void Release(HookEvent) = 0;
};

/*  Certain hook events have a specialized structure that is passed to the Hook::OnEvent method.
 *  Below are the structures passed along with descriptions of their fields.
 */

/** Data for the HE_instruction event. */
/*
 *  Information such as the current package, argc, self object, locals can
 *  be obtained from InstructionData's context field.
 */
struct InstructionData {
    code_t*   pc;       //!< [in] Program counter.
    Function* function; //!< [in] Def being executed.
    Context*  context;  //!< [in] Context originated from.
};

/** Data for the HE_import event. */

struct ImportData {
    String*  name;    //!< [in]  Name of the module we want to import.
    Engine*  engine;  //!< [in]  Engine the import originated from.
    Context* context; //!< [in]  Context the import originated from.
    Package* result;  //!< [out] The result of the import.
};

}
#endif
