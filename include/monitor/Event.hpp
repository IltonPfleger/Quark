#ifndef __QUARK_MONIOR_EVENT__
#define __QUARK_MONIOR_EVENT__

#include <types.hpp>

namespace QUARK {

class Event {
  public:
    enum {
        HARDWARE   = 0,
        SOFTWARE   = 1,
        TRACEPOINT = 2,
        CACHE      = 3,
        RAW        = 4,
        BREAKPOINT = 5,
        KPROBE     = 6,
        UPROBE     = 7,
    };

    enum {
        CACHE_L1D  = 0,
        CACHE_L1I  = 1,
        CACHE_LL   = 2,
        CACHE_DTLB = 3,
        CACHE_ITLB = 4,
        CACHE_BPU  = 5,
        CACHE_NODE = 6,
    };

    enum {
        CACHE_OP_READ     = 0,
        CACHE_OP_WRITE    = 1,
        CACHE_OP_PREFETCH = 2,
    };

    enum {
        CACHE_RESULT_ACCESS = 0,
        CACHE_RESULT_MISS   = 1,
    };

    constexpr Event(uint64_t t, uint64_t c, uint64_t c1 = 0, uint64_t c2 = 0)
        : type(t),
          config(c),
          config1(c1),
          config2(c2) {}

    constexpr bool operator==(const Event &) const = default;

    static constexpr uint64_t cache(uint64_t cache, uint64_t operation, uint64_t result) { return cache | (operation << 8) | (result << 16); }

    uint64_t type;
    uint64_t config;
    uint64_t config1;
    uint64_t config2;
};

} // namespace QUARK

#endif
