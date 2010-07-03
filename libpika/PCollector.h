/*
 *  PCollector.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_COLLECTOR_HEADER
#define PIKA_COLLECTOR_HEADER
#ifndef PIKA_VALUE_HEADER
#   include "PValue.h"
#endif
namespace pika {

class Collector;
class GCObject;
class Engine;
class Context;
class RootObject;

enum EGCColor
{
    COLOR_gray,
    COLOR_black,
    COLOR_white,
};

// GCObject /////////////////////////////////////////////////////////////////////////////////////

class PIKA_API GCObject 
{
public:
    enum 
    {
        NoFlags         = 0,
        ReadyToCollect  = (1 << 0),
        Persistent      = (1 << 1),
        Root            = (1 << 2),
        UserFlagsStart  = Root,
    };
    
    INLINE GCObject() : gcprev(0), gcnext(0), gcextra(0) { }
    virtual ~GCObject() {}
    
    virtual void MarkRefs(Collector*);
    
    // Can the Collector free the object? Return false only if you need to free the object manually.
    virtual bool Finalize(); 
    
	INLINE bool IsPersistent() const { return (gcflags & Persistent) ? true : false; }
    
    virtual void Mark(Collector*);
protected:   
    friend class Collector;
    
    INLINE bool IsLinked() const { return gcprev != 0 && gcnext != 0; }
    void Unlink();
    
    // If this object is already linked it MUST be unlinked before re-inserting it.
    void InsertAfter(GCObject*);
    void InsertBefore(GCObject*);
    
    INLINE void SetBlack() { gccolor = COLOR_black; }
    INLINE void SetWhite() { gccolor = COLOR_white; }
    INLINE void SetGray()  { gccolor = COLOR_gray;  }
    
    GCObject* gcprev;
    GCObject* gcnext;
    
    union 
    {
        u4 gcextra;
        struct 
        {
            u1 gccolor;
            u2 gcflags;
        };
    };
};

/** A tri-color, incremental, mark and sweep garbage collector.
  * The collector is proactive, meaning it will invoke itself as objects are added without the need
  * to explicitly call to Check.
  * 
  * The collection phases is divided into three major parts.
  * 
  * 1. move-roots stage:
  *         Currently the only atomic operation. The black-list and gray-list will be empty. The
  *         white-list will contain all objects that the collector controls. All root objects are
  *         then moved into the gray list in preparation of the scanning phase.
  * 2. scan-grays stage:
  *         All objects in the gray list are marked and moved into the black-list. Any white object
  *         that can be reached from a gray object is moved to the end of the gray list so that it
  *         can be scanned. When there are no more objects in the gray-list we move to the
  *         sweep-whites stage.
  * 3. sweep-whites stage:
  *         Ideally at this point all whites would be considered garbage an could be freed. But for
  *         performance reasons the active-context does not perform a write barrier as
  *         objects are pushed onto its stack or functions are called (etc.) Therefore before each
  *         sweep iteration we need to remark the active-context to ensure that any recently created
  *         objects that are still visible from the roots do not get destroyed. Once every white-list
  *         object is destroyed the collector resets, moves all black-list objects into the white-list and
  *         starts over with the move-roots stage.
  * 
  * @note   Root objects are destroyed liked ordinary objects; The only difference is that there is no need
  *         to mark a root object.
  */ 
class PIKA_API Collector 
{
public:
    enum 
    {
        // These values can be adjusted as needed.

        // Number of allocation between gc runs.
        // A value of 1 means the it runs on each allocation (don't do that!)
        // Higher values mean more garbage is accumulated. How that
        // affects performance depends on the script the objects in use.

        GC_NUM_ALLOCS = 256,

        // Number of iterations to perform per run.
        // Setting this too low (< 25-50 or so) can affect performance
        // more than setting it too high because of the overhead involved
        // (particularly in the sweep stage where the active context is marked
        // before each run).

        GC_NUM_ITERATIONS = 2048,
    };
    
    enum ECollectorState 
    {
        SUSPENDED,
        ROOT_SCAN,
        GRAY_SCAN,
        SWEEP,
    };
    
    Collector(Engine*);
    ~Collector();
    
    /** Pause the collector.
      * Will prevent the collector from running, and should be used whenever you create multiple
      * objects at the same time without 'rooting' them. Since the creation of the second object
      * might cause the first to be collected before any references to it are established.
      */
    void Pause();
    
    /** Resume a previously paused collector. 
      * @note May cause the collector run.
      */
    void Resume();
    
    /** Resume a previously paused collector.
      * @note The collector will <b>not</b> run. 
      */
    void ResumeNoRun();
    
    /** Sets the current active Context.
      * @note You will never need to call this directly unless you have provided
      *       a new context (coroutine) class.  
      */
    void ChangeContext(GCObject*);
    
    /** Perform an incremental gc cycle, but only if its time to do so. 
      * @note You should never need to call this directly. 
      */
    void Check();
    
    /** Add a root object to this collector. */
    void AddRoot(GCObject*);
    
    /*
    TODO: Remove root object method.
    bool RemoveRoot(GCObject*) {...}    
    */
    
    /** Add an object to this collector.
      * @note May cause the collector run.
      */
    void Add(GCObject*);
    
    /** Add an object to this collector without running the collector. */
    void AddNoRun(GCObject*);
    
    /** Perform an incremental gc cycle, even if its not time to do so. 
      * @note You should never need to call this directly. 
      */
    void IncrementalRun();
    
    /** Perform a complete gc cycle or finishes the current incremental cycle. */    
    void FullRun();
    
    /** Move the passed argument to the gray list if its in the white list. */    
    void MoveToGray(GCObject*);
    void MoveToGray(Value& v);    
    
    /** Move the passed argument to the gray list no matter what list its in. */    
    void ForceToGray(GCObject*);
    
    /** Call this method when adding a new reference (new_ref) to and existing object (old_ref).
      * This is necessary in order to maintain the invariant that no black object points to
      * any white objects. If you do not perform a write barrier new_ref might be freed while old_ref
      * still points to it.
      *   
      * @note   Is is only necessary to call WriteBarrier after the object has been added to the collector.
      *         When constructing a new object you should set all its initial references before you call
      *         Collector::Add, that way do not have to call WriteBarrier at all.
      *
      * @param old_ref  [in] The object the reference will be added to.
      * @param new_ref  [in] The reference being added.
      */
    void WriteBarrier(GCObject* old_ref, GCObject* new_ref);
    
    INLINE bool IsGray (const GCObject* c) const { return c->gccolor == grays ->gccolor; }    
    INLINE bool IsBlack(const GCObject* c) const { return c->gccolor == blacks->gccolor; }    
    INLINE bool IsWhite(const GCObject* c) const { return c->gccolor == whites->gccolor; }
    
    size_t GetNumObjects() const { return numObjects; }
    
    void FreeObject(GCObject* t);
private:
    friend class GCObject;
    
    void Initialize();
    
    INLINE bool NextIteration() { return(iteration++ < GC_NUM_ITERATIONS); }
    
    bool IncrementalMoveRoots(bool);
    bool IncrementalScan(bool);
    bool IncrementalSweep(bool);
    
    void FreeAll();
    void FreeList(GCObject* list);
    
    void Reset();
    
    size_t numObjects;
    size_t maxObjects;
    size_t totalObjects;
    
    size_t numAllocations;
    size_t iteration;
    
    int pauseDepth;
    
    Engine* engine;
    
    ECollectorState state;
    ECollectorState savedState;
    
    /** The active context.
      * It is necessary to [re]mark the current Context before the sweep stage can begin.
      * We have to do this since a write-barrier is not performed for values pushed onto 
      * the stack or stored in the activation record.
      */
    GCObject* activeCtx;    
    /*  
        Circular list of all GCObjects we are resposible for.
        
        :::::::::::::::::::::::::::::::::::::::::::::::::::::
        ::              Collector layout                   ::
        :::::::::::::::::::::::::::::::::::::::::::::::::::::
        
                             black 
                     .----> ^    v
             sweep--/      /      \
                   /      ^        v
                  /      /          \
                  '---> ^            v
                      white <--<--< gray 
                            ^     ^
                            |     |
                            '--+--'
                               |
                             scan 
        
        Scan object moves from gray list during the scan phase.
        Sweep object moves through the white list freeing each un-reachable object during the sweep phase.
        
        :::::::::::::::::::::::::::::::::::::::::::::::::::::
        ::                 Root objects                    ::
        :::::::::::::::::::::::::::::::::::::::::::::::::::::
        
                           .-head--->---.
                           |            |
                           ^            v <<:::: All root objects are contained inside a circular list.
                           |            |
                           '-----<------'
    
    */
    GCObject* blacks; //!< All objects seen and marked.
    GCObject* grays;  //!< All objects seen but not marked.
    GCObject* whites; //!< All objects unmarked and unseen; Potential garbage.
    GCObject* scan;   //!< Points to the next object to be marked. Can point anywhere between grays and whites.
    GCObject* sweep;  //!< Used during sweep phase, point to the next object to be freed.
    
    RootObject* head; // Circular list of all root objects.
};

INLINE void MarkValue(Collector* c, Value& v)
{
    ASSERT(v.tag < MAX_TAG);

    if (v.tag >= TAG_gcobj && v.val.gcobj)
        v.val.gcobj->Mark(c);
}

INLINE void MarkValues(Collector* c, Value *begin, Value* end)
{
    for (Value *curr = begin; curr < end; ++curr)
    {
        MarkValue(c, *curr);
    }
}


}// pika

using pika::Collector;
using pika::GCObject;
using pika::Engine;
using pika::Context;

#endif
