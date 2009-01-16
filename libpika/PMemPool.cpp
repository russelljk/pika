/*
 *  PMemPool.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PMemory.h"
#include "PMemPool.h"

namespace pika
{

#if 1

MemPoolMixed::MemPoolMixed(size_t blockSize)
        : Blocks(0),
        NextFree(0),
        End(0),
        BlockSize(blockSize) {}
        
MemPoolMixed::~MemPoolMixed() { Destroy(); }

void MemPoolMixed::Destroy()
{
    MixedBlock* curr = Blocks;
    while (curr)
    {
        MixedBlock* next = curr->Next;
        Pika_free(curr);
        curr = next;
    }
    Blocks = 0;
}

void* MemPoolMixed::NewBlock(size_t size)
{
    MixedBlock* curr = (MixedBlock*)(Pika_malloc(sizeof(MixedBlock) + size));
    curr->Next = Blocks;
    Blocks = curr;
    ++curr;
    return (void *)curr;
}

void* MemPoolMixed::Alloc(size_t size)
{
    size = size + (PIKA_ALIGN - 1) & -PIKA_ALIGN;
    char* p = NextFree;
    size_t freeBytes = (size_t)(End - p);
    
    if (size > freeBytes)
    {
        if ((freeBytes << 2) >= BlockSize || size >= BlockSize)
            return NewBlock(size);
            
        p = (char*)(NewBlock(BlockSize));
        End = p + BlockSize;
    }
    NextFree = p + size;
    return p;
}

#endif

/* MemArena's block layout.
 *      A single block begins and ends on an 8-byte boundary. The header
 * occupies 8-bytes total with the last 4 serving as padding on 32-bit
 * machines. Following the header is the actual block itself. Depending
 * on the requested block size padding will be added so that the next
 * block starts on an 8-byte boundary.
 *
 * +--------------------------------------------------------+
 * | Prev Block                                             |
 * | ...                                                    |
 * +--------------------------------------------------------+----+
 * | FreeBlock\AllocBlock: 4 or 8 bytes for 32|64 bit arch. |    |
 * +--------------------------------------------------------+    +-Header 8-bytes
 * | Padding: 4 bytes for 32 bit arch.                      |    |
 * +--------------------------------------------------------+----+
 * | Block: Size                                            |
 * +--------------------------------------------------------+
 * | Padding to align to 8-byte boundary. Amount            |
 * | depends on the block size.                             |
 * +--------------------------------------------------------+
 * | Next Block                                             |
 * | ...                                                    |
 * +--------------------------------------------------------+
 */
MemArena::MemArena(size_t blockSize, size_t numBlocks, FreeBlock** FreeList, MemArena* next)
{
    size_t blockPlusHeaderSize;
    u1* p;
    u1* limit;
    FreeBlock* fb = 0;
    
    // Align the block size.
    
    blockSize = MemAlign(blockSize, PIKA_ALIGN);
    
    // add 8-bytes for the header.
    
    blockPlusHeaderSize = blockSize + HeaderSize();
    
    Next      = next;
    Start     = 0;
    Size      = blockSize;
    Length    = numBlocks;
    TotalSize = blockPlusHeaderSize;
    
    Start = Pika_malloc(TotalSize * Length);
    
    p = (u1*)Start;
    limit = p + (TotalSize * Length);
    
    while (p < limit)
    {
        fb = (FreeBlock*)p;
        p += blockPlusHeaderSize;
        fb->Next = (FreeBlock*)p;
    }
    
    ASSERT(fb);
    
    fb->Next = *FreeList;
    *FreeList = (FreeBlock *)Start;
}

MemArena::~MemArena() { if (Start) Pika_free(Start); }

MemPool::MemPool(size_t blockSize, size_t arenaLength): Count(0), BlockSize(blockSize), ArenaLength(arenaLength), Arenas(0), FreeList(0)
{}

void MemPool::CreateNewArena()
{
    MemArena* newBlock = 0;
    PIKA_NEW(MemArena, newBlock, (BlockSize, ArenaLength, &FreeList, Arenas));
    Arenas = newBlock;
    ++Count;
}

void* MemPool::Alloc()
{
    if (!FreeList)
        CreateNewArena();
        
    AllocBlock * ab = (AllocBlock *)FreeList;
    FreeList = FreeList->Next;
    ab->Size = BlockSize;
    
    return (void*)((u1*)ab + MemArena::HeaderSize());
}

void MemPool::Free(void* v)
{
    if (!v) return;
    
    FreeBlock * fb = (FreeBlock *)((u1*)v - MemArena::HeaderSize());
    fb->Next = FreeList;
    FreeList = fb;
}

void MemPool::Destroy()
{
    MemArena *curr, *next;
    curr = Arenas;
    
    while (curr)
    {
        next = curr->Next;
        Pika_delete<MemArena>(curr);
        curr = next;
    }
    
    FreeList = 0;
    Arenas = 0;
    Count = 0;
}

MemPool::~MemPool() { Destroy(); }

MemObjArena::MemObjArena(size_t objSize, size_t objCount, FreeBlock** FreeList, MemObjArena* next):
    Size(objSize),
    Count(objCount),
    Next(next)
{
    Start = Pika_malloc(Size * Count);
    
    char* ptr = (char*)Start;
    char* limit = (char*)Start + (Size * Count);
    FreeBlock* fb = 0;
    
    while (ptr < limit)
    {
        fb = (FreeBlock*)ptr;
        ptr += Size;
        fb->Next = (FreeBlock*)ptr;
    }
    
    fb->Next = *FreeList;
    *FreeList = (FreeBlock*)Start;
}

MemObjArena::~MemObjArena()
{
    Pika_free(Start);
}

}
