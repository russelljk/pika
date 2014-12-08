/*
 *  PCollector.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"

#define STILL_ITERATING() (!b || NextIteration())

namespace pika {

void GCObject::Unlink()
{
    if (gcprev) gcprev->gcnext = gcnext;
    if (gcnext) gcnext->gcprev = gcprev;
    
    gcprev = gcnext = 0;
}

void GCObject::InsertAfter(GCObject* c)
{
    gccolor = c->gccolor;
    
    if (c->gcnext)
    {
        gcprev = c;
        gcnext = c->gcnext;
        c->gcnext->gcprev = this;
        c->gcnext = this;
    }
    else
    {
        gcnext = 0;
        gcprev = c;
        c->gcnext = this;
    }
}

void GCObject::InsertBefore(GCObject* c)
{
    if (c->gcprev)
    {
        gccolor = c->gcprev->gccolor;
        gcprev  = c->gcprev;
        gcnext  = c;
        c->gcprev->gcnext = this;
        c->gcprev = this;
    }
    else
    {
        gcprev = 0;
        gcnext = c;
        c->gcprev = this;
    }
}

void GCObject::Mark(Collector* c)
{
    if (gccolor == c->whites->gccolor)
    {
        Unlink();
        InsertAfter(c->scan);
        MarkRefs(c);
    }
}

void GCObject::MarkRefs(Collector*) { }

bool GCObject::Finalize() { return true; }

////////////////////////////////// RootObject //////////////////////////////////

class RootObject
{
public:
    RootObject(GCObject* t)
            : next(0), prev(0), gcobj(t)
    {}
    
    virtual ~RootObject() {}
    
    void Unlink()
    {
        if (prev) prev->next = next;
        if (next) next->prev = prev;
        
        prev = next = 0;
    }
    
    void InsertAfter(RootObject* c)
    {
        if (c->next)
        {
            prev = c;
            next = c->next;
            c->next->prev = this;
            c->next = this;
        }
        else
        {
            next = 0;
            prev = c;
            c->next = this;
        }
    }
    
    void InsertBefore(RootObject* c)
    {
        if (c->prev)
        {
            prev  = c->prev;
            next  = c;
            c->prev->next = this;
            c->prev = this;
        }
        else
        {
            prev = 0;
            next = c;
            c->prev = this;
        }
    }
    
    INLINE GCObject* GetObj() { return gcobj; }
    
protected:
    friend class Collector;
    RootObject*  next;
    RootObject*  prev;
    GCObject*    gcobj;
};

////////////////////////////////// Collector ///////////////////////////////////

Collector::Collector(Engine* eng)
        : numObjects(0),
        maxObjects(0),
        totalObjects(0),
        iteration(0),
        engine(eng),
        activeCtx(0),
        blacks(0),
        grays(0),
        whites(0),
        head(0)
{
    Initialize();
}

Collector::~Collector()
{
    FreeAll();
#if defined(PIKA_DEBUG_OUTPUT)
    std::cout << "\n*******************************\n";
    std::cout << "Number of gc objects: " << totalObjects;
    std::cout << "\n*******************************\n";
#endif
}

void Collector::Pause()
{
    ++pauseDepth;
    
    if (state != SUSPENDED)
    {
        savedState = state;
        state = SUSPENDED;
    }
}

void Collector::ResumeNoRun()
{
    if (--pauseDepth <= 0)
    {
        pauseDepth = 0;
        state = savedState;
    }
}

void Collector::Resume()
{
    if (--pauseDepth <= 0)
    {
        pauseDepth = 0;
        state = savedState;
        if (numAllocations == 0)
        {
            IncrementalRun();
        }
    }
}

void Collector::ChangeContext(GCObject* c)
{
    if (activeCtx)
    {
        // Move the previous context into the gray list after the scan marker.
        // This ensures that the previous context will be properly marked
        // regardless of the GC's state.
        
        ForceToGray(activeCtx);
    }
    activeCtx = c;
}

void Collector::AddAsRoot(GCObject* c)
{
    if (!c) return;
    c->Unlink();
    c->InsertAfter(grays);
    
    RootObject* robj = new RootObject(c);
    robj->InsertAfter(head);
}

bool Collector::RemoveAsRoot(GCObject* c)
{
    for (RootObject* curr = head->next; curr != head; curr = curr->next)
    {
        if (curr->GetObj() == c)
        {
            curr->Unlink();
            delete curr;
            c->Unlink();
            c->InsertAfter(grays);
            return true;
        }
    }
    return false;
}

void Collector::Check()
{
    if ( state != SUSPENDED )//if ((numAllocations == 0) && (state != SUSPENDED))
    {
        IncrementalRun();
    }
}

void Collector::CheckIf()
{
    if ( state != SUSPENDED && (numAllocations == 0) )
    {
        IncrementalRun();
    }    
}

void Collector::Add(GCObject* c)
{
    c->Unlink();
    
    if (numAllocations == 0)
    {
        if (state != SUSPENDED)
        {
            IncrementalRun();
        }
    }
    else
    {
        --numAllocations;
    }
    c->InsertAfter(whites);
    ++numObjects; ++totalObjects;
}

void Collector::AddNoRun(GCObject* c)
{
    c->Unlink();
    c->InsertAfter(whites);
    
    if (numAllocations != 0)
    {
        --numAllocations;
    }
    ++numObjects; ++totalObjects;
}

void Collector::IncrementalRun()
{
    if (state != SUSPENDED)
    {
        IncrementalMoveRoots(true);
        numAllocations = GC_NUM_ALLOCS;
    }
}

void Collector::FullRun()
{
    if (state != SUSPENDED)
    {
        IncrementalMoveRoots(false);
    }
}

void Collector::MoveToGray(GCObject* c)
{
    if (c && (c->gccolor == whites->gccolor))
    {
        c->Unlink();
        c->InsertAfter(grays);
    }
}

void Collector::ForceToGray(GCObject* c)
{
    if (!c) return;
    
    c->Unlink();
    c->InsertAfter(grays);
}

void Collector::WriteBarrier(GCObject* oldC,
                             GCObject* newC)
{
    if ((oldC->gccolor == blacks->gccolor) &&
            (newC->gccolor == whites->gccolor))
    {
        newC->Unlink();
        newC->InsertAfter(grays);
    }
}

void Collector::MoveToGray(Value& v)
{
    if (v.tag >= TAG_gcobj)
    {
        MoveToGray(v.val.gcobj);
    }
}

void Collector::Initialize()
{
    numAllocations  = GC_NUM_ALLOCS;
    pauseDepth = 0;
    savedState = state = ROOT_SCAN;
    
    blacks = Pika_new<GCObject>();
    grays  = Pika_new<GCObject>();
    whites = Pika_new<GCObject>();
    
    blacks->gcnext = blacks;
    blacks->gcprev = blacks;
    
    grays->InsertAfter(blacks);
    whites->InsertAfter(grays);
    
    blacks->SetBlack();
    grays->SetGray();
    whites->SetWhite();
    
    /**************/
    head = new RootObject(0);
    head->next = head;
    head->prev = head;
    /**************/
}

// TODO: Split ScanRoots into multiple stages (>=2).
bool Collector::IncrementalMoveRoots(bool b)
{
    if (state != ROOT_SCAN)
    {
        return IncrementalScan(b);
    }
    //
    // Stage 1
    //
    scan = grays;
    engine->ScanRoots(this);
    //
    // Stage 2
    // scan our root objects.
    //
    RootObject* robj = head->next;
    while (robj != head)
    {
        GCObject* t = robj->GetObj();
        if (t) MoveToGray(t);
        
        robj = robj->next;
    }
    //
    // Time to start the scanning the grays.
    //
    state = GRAY_SCAN;
    scan  = grays->gcnext;
    iteration = 0;
    if (!b)
    {
        return IncrementalScan(b);
    }
    return true;
}

bool Collector::IncrementalScan(bool b)
{
    if (state != GRAY_SCAN)
        return IncrementalSweep(b);
    
    while ((scan->gccolor == grays->gccolor) && // Loop while there are still objs in the gray list
            STILL_ITERATING())                  // and while we still have some iterations left.
    {        
        GCObject* next = scan->gcnext; // Save the next object.
        scan->MarkRefs(this);          // Mark it.
        scan->Unlink();                // Remove it from the gray list.
        scan->InsertAfter(blacks);     // Add it to the black list.
        scan = next;                   // Move to the next object.
    }

    // If its time to start sweeping.
    if ((scan->gccolor != grays->gccolor))
    {
        state = SWEEP;
        sweep = whites->gcnext;
        return IncrementalSweep(b);
    }
    // Otherwise we will finish scanning at a later time.
    iteration = 0;
    return false;
}

void Collector::FreeAll()
{
    // Delete all RootObjects and
    // add the GCObject to the black list.
    RootObject* robj = head->next;
    while (robj != head)
    {
        RootObject* next = robj->next;
        GCObject* t = robj->GetObj();
        if (t)
        {
            t->Unlink();
            t->InsertAfter(blacks);
        }
        delete robj;
        robj = next;
    }
    
    // Delete the head.
    
    delete head;
    head = 0;
    
    // Free each list.
    FreeList(whites);
    FreeList(blacks);
    FreeList(grays);
    
    // Free each list marker.
    FreeObject(whites);
    FreeObject(blacks);
    FreeObject(grays);
    
    whites = blacks = grays = scan = sweep = 0;
}

void Collector::FreeList(GCObject *list)
{
    GCObject* curr = list->gcnext;
    GCObject* next = 0;
    
    while (curr->gccolor == list->gccolor)
    {
        next = curr->gcnext;
        curr->Unlink();
        
        if (curr->Finalize())
        {
            FreeObject(curr);
        }
        curr = next;
    }
}

void Collector::Reset()
{
    Swap(blacks, whites);
    
    state = ROOT_SCAN;
    numAllocations = GC_NUM_ALLOCS;
    iteration = 0;
}

bool Collector::IncrementalSweep(bool b)
{
    if (state != SWEEP)
        return false;    
    
    // Rescan the active context(s) before sweeping.
    // This needs to be atomic.
    
    ForceToGray(activeCtx);
    scan = grays->gcnext;
    while (scan->gccolor == grays->gccolor)
    {
        GCObject* next = scan->gcnext;
        scan->MarkRefs(this);
        scan->Unlink();
        scan->InsertAfter(blacks);
        scan = next;
    }
    
    // Start sweeping from the first white object.
    
    sweep = whites->gcnext;
    maxObjects = Max<size_t>(maxObjects, numObjects);
    
    while ((sweep->gccolor == whites->gccolor) && STILL_ITERATING())
    {
        GCObject* next = sweep->gcnext;
        sweep->Unlink();
        --numObjects;
        
        if (sweep->IsPersistent()) // Don't free a persistent object
        {
            sweep->InsertAfter(blacks);
        }
        else if (sweep->Finalize()) // Free this object if it finalizes
        {
            FreeObject(sweep);
        }
        sweep = next;
    }
    
    // If we finished reset the collector
    if (sweep->gccolor != whites->gccolor)
    {
        engine->SweepStringTable();
        state = ROOT_SCAN;
        Reset();
        return true;
    }
    // Otherwise we will continue at a later time.
    iteration = 0;
    return false;
}

void Collector::FreeObject(GCObject *t)
{
    Pika_delete(t);
}

}// pika

