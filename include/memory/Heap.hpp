#pragma once

#include <Traits.hpp>
#include <memory/Heap.hpp>
#include <memory/Memory.hpp>
#include <utility/Debug.hpp>

namespace QUARK {

class Heap {
  public:
    struct Header {
        size_t size;
    };
    enum Location { APPLICATION, SYSTEM };
};

} // namespace QUARK

inline void *operator new(QUARK::size_t s, QUARK::Heap::Location) {
    using Header   = QUARK::Heap::Header;
    Header *header = reinterpret_cast<Header *>(QUARK::Memory::alloc(s + sizeof(Header)));
    assert(header);
    header->size = s + sizeof(Header);
    return header + 1;
}

inline void operator delete(void *p) {
    using Header = QUARK::Heap::Header;
    assert(p);
    Header *header = reinterpret_cast<Header *>(p) - 1;
    QUARK::Memory::free(header, header->size);
}

inline void *operator new(QUARK::size_t s) { return ::operator new(s, QUARK::Heap::APPLICATION); }
inline void *operator new[](QUARK::size_t s) { return ::operator new(s, QUARK::Heap::APPLICATION); }
inline void *operator new[](QUARK::size_t s, QUARK::Heap::Location l) { return ::operator new(s, l); }
inline void operator delete(void *p, QUARK::size_t) { ::operator delete(p); }
inline void operator delete[](void *p) { ::operator delete(p); }
inline void operator delete[](void *p, QUARK::size_t) { ::operator delete(p); }
