// core/memory.cpp

#include "memory.hpp"
#include <cstdlib>
#include <cstring>
#include <new>

namespace core {

// ------------------------------------------------------------------
// LinearAllocator
// ------------------------------------------------------------------

LinearAllocator::LinearAllocator(usize sizeBytes)
    : m_capacity(sizeBytes), m_offset(0)
{
    m_base = static_cast<u8*>(std::malloc(sizeBytes));
    ASSERT_ALWAYS(m_base != nullptr, "LinearAllocator: out of memory");
}

LinearAllocator::~LinearAllocator() {
    std::free(m_base);
}

void* LinearAllocator::Allocate(usize bytes, usize alignment) {
    usize current = reinterpret_cast<usize>(m_base) + m_offset;
    usize aligned = (current + (alignment - 1)) & ~(alignment - 1);
    usize padding = aligned - current;

    if (m_offset + padding + bytes > m_capacity) {
        return nullptr; // arena exhausted
    }

    m_offset += padding + bytes;
    return reinterpret_cast<void*>(aligned);
}

void LinearAllocator::Reset() {
    m_offset = 0;
}

// ------------------------------------------------------------------
// PoolAllocator
// ------------------------------------------------------------------

PoolAllocator::PoolAllocator(usize blockSize, usize blockCount)
    : m_blockSize(blockSize < sizeof(FreeNode) ? sizeof(FreeNode) : blockSize)
    , m_blockCount(blockCount)
    , m_freeCount(blockCount)
{
    m_base = static_cast<u8*>(std::malloc(m_blockSize * m_blockCount));
    ASSERT_ALWAYS(m_base != nullptr, "PoolAllocator: out of memory");

    // Thread every block into a singly-linked free list.
    m_freeList = reinterpret_cast<FreeNode*>(m_base);
    for (usize i = 0; i < m_blockCount - 1; ++i) {
        FreeNode* node = reinterpret_cast<FreeNode*>(m_base + i * m_blockSize);
        node->next = reinterpret_cast<FreeNode*>(m_base + (i + 1) * m_blockSize);
    }
    reinterpret_cast<FreeNode*>(m_base + (m_blockCount - 1) * m_blockSize)->next = nullptr;
}

PoolAllocator::~PoolAllocator() {
    std::free(m_base);
}

void* PoolAllocator::Allocate() {
    if (!m_freeList) return nullptr; // pool exhausted

    FreeNode* node = m_freeList;
    m_freeList = node->next;
    --m_freeCount;
    return node;
}

void PoolAllocator::Free(void* block) {
    if (!block) return;
    FreeNode* node = reinterpret_cast<FreeNode*>(block);
    node->next = m_freeList;
    m_freeList = node;
    ++m_freeCount;
}

} // namespace core
