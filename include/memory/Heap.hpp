#pragma once

#include <Traits.hpp>
#include <memory/Memory.hpp>
#include <utility/Debug.hpp>

enum class Heap { APPLICATION, SYSTEM };

struct HeapHeader {
    unsigned long size;
};

inline void *operator new(unsigned long length, std::align_val_t align, Heap) {
    unsigned long alignment = static_cast<unsigned long>(align);

    if (alignment < alignof(void *)) alignment = alignof(void *);

    const unsigned long total = sizeof(HeapHeader) + sizeof(void *) + alignment - 1 + length;

    auto *raw = static_cast<unsigned char *>(QUARK::Memory::alloc(total));

    auto *header = reinterpret_cast<HeapHeader *>(raw);
    header->size = total;

    unsigned long long pointer = reinterpret_cast<unsigned long long>(raw + sizeof(HeapHeader) + sizeof(void *));

    unsigned long long aligned = (pointer + alignment - 1) & ~(alignment - 1);

    void *result = reinterpret_cast<void *>(aligned);

    reinterpret_cast<HeapHeader **>(result)[-1] = header;

    return result;
}

inline void operator delete(void *pointer) noexcept {
    if (!pointer) return;

    auto *header = reinterpret_cast<HeapHeader **>(pointer)[-1];

    QUARK::Memory::free(header, header->size);
}

inline void operator delete(void *pointer, unsigned long, Heap) noexcept { ::operator delete(pointer); }

inline void operator delete(void *pointer, unsigned long, unsigned long, Heap) noexcept { ::operator delete(pointer); }

// -----------------------------------------------------------------------------
// Scalar New
// -----------------------------------------------------------------------------
inline void *operator new(unsigned long size, Heap selector) { return ::operator new(size, static_cast<std::align_val_t>(1), selector); }

inline void *operator new(unsigned long size) { return ::operator new(size, Heap::APPLICATION); }

inline void *operator new(unsigned long size, std::align_val_t alignment) { return ::operator new(size, alignment, Heap::APPLICATION); }

// -----------------------------------------------------------------------------
// Array New
// -----------------------------------------------------------------------------
inline void *operator new[](unsigned long size) { return ::operator new(size); }

inline void *operator new[](unsigned long size, Heap selector) { return ::operator new(size, selector); }

inline void *operator new[](unsigned long size, std::align_val_t alignment) { return ::operator new(size, alignment, Heap::APPLICATION); }

inline void *operator new[](unsigned long size, std::align_val_t alignment, Heap selector) {
    return ::operator new(size, alignment, selector);
}

// -----------------------------------------------------------------------------
// Deletes
// -----------------------------------------------------------------------------
inline void operator delete[](void *pointer) noexcept { ::operator delete(pointer); }

inline void operator delete[](void *pointer, unsigned long) noexcept { ::operator delete(pointer); }

inline void operator delete(void *pointer, unsigned long) noexcept { ::operator delete(pointer); }
