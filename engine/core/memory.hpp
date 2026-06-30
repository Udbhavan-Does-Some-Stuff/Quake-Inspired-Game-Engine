// core/memory.hpp
//
// Custom allocators. The default heap (malloc/new) is fine for
// one-off, infrequent allocations (loading a level), but is too slow
// and too fragmenting for anything that happens every frame.
//
// Two allocators to start:
//   - LinearAllocator: bump-pointer arena. Allocates are O(1) and
//     can't be individually freed — you Reset() the whole thing at
//     once. Perfect for per-frame scratch memory.
//   - PoolAllocator: fixed-size block allocator. Good for entities,
//     particles, or anything where you have many same-sized objects
//     that get created/destroyed constantly.

#pragma once

#include "types.hpp"

namespace core {

// ------------------------------------------------------------------
// LinearAllocator
//
// Usage pattern:
//   LinearAllocator frameArena(1 * 1024 * 1024); // 1 MB
//   ... each frame ...
//   void* scratch = frameArena.Allocate(256);
//   ... at end of frame ...
//   frameArena.Reset();
// ------------------------------------------------------------------
class LinearAllocator {
public:
    explicit LinearAllocator(usize sizeBytes);
    ~LinearAllocator();

    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;

    // Returns nullptr if the arena is exhausted — caller decides
    // whether that's fatal (it usually is, for per-frame arenas;
    // it means you sized the arena too small).
    void* Allocate(usize bytes, usize alignment = alignof(std::max_align_t));

    template <typename T, typename... Args>
    T* AllocateObject(Args&&... args) {
        void* mem = Allocate(sizeof(T), alignof(T));
        if (!mem) return nullptr;
        return new (mem) T(static_cast<Args&&>(args)...);
    }

    void Reset();
    usize BytesUsed() const { return m_offset; }
    usize Capacity()  const { return m_capacity; }

private:
    u8*   m_base;
    usize m_capacity;
    usize m_offset;
};

// ------------------------------------------------------------------
// PoolAllocator
//
// Allocates fixed-size blocks from a preallocated buffer using a
// free list. O(1) allocate and free, zero fragmentation, but every
// block is the same size (sizeof(T) rounded up).
// ------------------------------------------------------------------
class PoolAllocator {
public:
    PoolAllocator(usize blockSize, usize blockCount);
    ~PoolAllocator();

    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    // Returns nullptr if the pool is full.
    void* Allocate();
    void  Free(void* block);

    usize BlocksInUse() const { return m_blockCount - m_freeCount; }
    usize BlockCount()  const { return m_blockCount; }

private:
    struct FreeNode { FreeNode* next; };

    u8*       m_base;
    usize     m_blockSize;
    usize     m_blockCount;
    usize     m_freeCount;
    FreeNode* m_freeList;
};

} // namespace core
