/*
 *  PMemPool.h
 *  See Copyright Notice in Pika.h
 */
#ifndef PIKA_MEMPOOL_HEADER
#define PIKA_MEMPOOL_HEADER

#define PIKA_MIN_POOL_SIZE 128
#define PIKA_AVG_POOL_SIZE 2048

namespace pika {

class MemPool;

struct AllocBlock { size_t     Size; };
struct FreeBlock  { FreeBlock* Next; };

#if 1
// MemPoolMixed, MemObjPoolMixed and PoolObject are untested and unused!

// MemPoolMixed /////////////////////////////////////////////////////////////////////////

struct MemPoolMixed
{
    union MixedBlock
    {
        MixedBlock* Next;
        // The rest is alignment.
        char        Padding[PIKA_ALIGN];
        s1          _s1_;
        s2          _s2_;
        s4          _s4_;
        s8          _s8_;
        float       _f_;
        double      _d_;
        size_t      _st_;
    };
    
    MemPoolMixed(size_t blockSize = 1024);
    
    virtual        ~MemPoolMixed();
    
    void            Destroy();
    void*           NewBlock(size_t size);
    void*           Alloc(size_t size);
    
    MixedBlock*     Blocks;
    char*           NextFree;
    char*           End;
    size_t const    BlockSize;
};

// MemObjPoolMixed<T> ///////////////////////////////////////////////////////////////////

template<typename T>
struct MemObjPoolMixed : MemPoolMixed
{
    MemObjPoolMixed(size_t blockSize) : MemPoolMixed(blockSize) {}
    
    virtual        ~MemObjPoolMixed() { Destructors(); }
    
    void Destructors()
    {
        MixedBlock* curr = Blocks;
        while (curr)
        {
            MixedBlock* next = curr->Next;
            curr++;
            T* pT = (T*)(curr);
            Pika_destruct<T>(pT);
            curr = next;
        }
    }
};

// PoolObject ///////////////////////////////////////////////////////////////////////////

struct PoolObject
{
    inline          PoolObject() {}
    virtual        ~PoolObject() {}
    
    void*           operator new(size_t size, MemObjPoolMixed<PoolObject>& pool) {return pool.Alloc(size);}
    void            operator delete(void*, MemObjPoolMixed<PoolObject>&) {}
    
    void*           operator new[](size_t size, MemObjPoolMixed<PoolObject>& pool) {return pool.Alloc(size);}
    void            operator delete[](void*, MemObjPoolMixed<PoolObject>&) {}
    
    void            operator delete(void*, size_t) {}
    void            operator delete[](void*) {}};
    
#endif
    
/** An arena class.
  * @note All blocks are align to the value of PIKA_ALIGN.
  */
class PIKA_API MemArena
{
public:
    MemArena(size_t blockSize, size_t numBlocks, FreeBlock** FreeList, MemArena* next);
    
    virtual        ~MemArena();
    
    static inline size_t HeaderSize() { return PIKA_ALIGN; }
    
    MemArena*       Next;       // Next arena in this list
    size_t          Length;     // Number of blocks.
    size_t          Size;       // Individual Block Size (aligned)
    size_t          TotalSize;  // Individual Block Size + Header Size (aligned)
    void*           Start;      // The blocks size=Length*TotalSize
    
};

class PIKA_API MemPool
{
public:

    explicit        MemPool(size_t blockSize, size_t arenaLength = PIKA_MIN_POOL_SIZE);
    virtual        ~MemPool();
    
    void*           Alloc();
    void            Free(void* v);
    
    inline size_t   GetNumArenas() const { return Count; }
    inline size_t   RamUsage() const { return Count *(BlockSize + MemArena::HeaderSize()) * ArenaLength; }
private:
    void            Destroy();
    void            CreateNewArena();
    
    size_t          Count;      // Number of MemArena's.
    size_t const    BlockSize;
    size_t const    ArenaLength;
    MemArena*       Arenas;
    FreeBlock*      FreeList;
};

/** Arena for c++ objects.
  * Similar to MemArena except space is not reserved at the beginning of a block for a header.
  * Meant to be used by MemObjPool.
  *
  * @note All blocks are aligned to the value of PIKA_ALIGN.
  */
class PIKA_API MemObjArena
{
public:
    MemObjArena(size_t       objSize,
                size_t       objCount,
                FreeBlock**  FreeList,
                MemObjArena* next);
                
    virtual ~MemObjArena();
    
    size_t          Size;   // aligned size
    size_t          Count;  // number of objects
    void*           Start;  // pointer to start of the objects.
    MemObjArena*    Next;   // next arena
};

/** A pool for objects. */

template<typename T>
class MemObjPool
{
    static inline size_t GetCommonSize()
    {
        size_t objectSize  = sizeof(T);
        size_t objectAlign = alignof(T);
        size_t headerSize  = sizeof(FreeBlock);
        size_t headerAlign = alignof(FreeBlock);
        size_t commonSize  = 0;
        size_t commonAlign = 0;
        
        // need a block size big enough and aligned for both
        // T and FreeBlock/AllocBlock
        
        CommonAlign(objectSize,
                    objectAlign,
                    headerSize,
                    headerAlign,
                    commonSize,
                    commonAlign);
                    
        return commonSize;
    }
public:
    MemObjPool(size_t length = PIKA_MIN_POOL_SIZE)
            : BlockSize( GetCommonSize() ),
            ArenaLength(length),
            Count(0),
            Arenas(0),
            FreeList(0)
    {}
    
    virtual ~MemObjPool()
    {
        MemObjArena* curr = Arenas;
        MemObjArena* next;
        
        while (curr)
        {
            next = curr->Next;
            Pika_delete<MemObjArena>(curr);
            curr = next;
        }
    }
    
    INLINE void* RawAlloc()
    {
        if (!FreeList)
            CreateNewArena();
            
        // Remove it from the freelist
        void* v = (void*)FreeList;
        FreeList = FreeList->Next;
        return v;
    }
    
    INLINE void RawFree(void* v)
    {
        if (!v)
            return;
            
        // add it to the freelist.
        FreeBlock* fb = (FreeBlock*)v;
        fb->Next = FreeList;
        FreeList = fb;
    }
    
    INLINE T* New()
    {
        T* t = (T*)this->RawAlloc();
        t = Pika_construct<T>(t); // construct and return it.
        return t;
    }
    
    INLINE void Delete(T* t)
    {
        if (!t)
            return;
            
        Pika_destruct<T>(t);
        this->RawFree((void*)t);
    }
    
private:

    void CreateNewArena()
    {
        MemObjArena* newBlock = 0;
        PIKA_NEW(MemObjArena, newBlock, (BlockSize, ArenaLength, &FreeList, Arenas));
        Arenas = newBlock;
        ++Count;
    }
    
    size_t const BlockSize;
    size_t const ArenaLength;
    size_t       Count;
    MemObjArena* Arenas;
    FreeBlock*   FreeList;
};

}// pika

#endif
