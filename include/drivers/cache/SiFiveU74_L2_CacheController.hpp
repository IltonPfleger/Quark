#pragma once

#include <Traits.hpp>

namespace QUARK {

template <typename Tag> class SiFiveU74_L2_CacheController {
    static constexpr struct {
        int fetch;
        int dcache[2];
        int mmio;
        int prefetcher;
    } Masters[] = {
        {.fetch = 1, .dcache = {-1, -1}, .mmio = 2, .prefetcher = -1},
        {.fetch = 3, .dcache = {4, 5}, .mmio = 6, .prefetcher = 7},
        {.fetch = 8, .dcache = {9, 10}, .mmio = 11, .prefetcher = 12},
        {.fetch = 13, .dcache = {14, 15}, .mmio = 16, .prefetcher = 17},
        {.fetch = 18, .dcache = {19, 20}, .mmio = 21, .prefetcher = 22},
    };

  public:
    static void color(uint64_t color, size_t core) {
        auto &master = Masters[core];
        if (master.fetch >= 0) WayMask[master.fetch] = color;
        if (master.dcache[0] >= 0) WayMask[master.dcache[0]] = color;
        if (master.dcache[1] >= 0) WayMask[master.dcache[1]] = color;
        if (master.mmio >= 0) WayMask[master.mmio] = color;
        if (master.prefetcher >= 0) WayMask[master.prefetcher] = color;
    }

    static void flush(const void *const ptr, size_t size) {
        if (size == 0) return;

        uintptr_t start = reinterpret_cast<uintptr_t>(ptr);
        uintptr_t line  = start & ~(static_cast<uintptr_t>(Traits<Tag>::CacheLineSize) - 1);
        uintptr_t end   = start + size;

        for (; line < end; line += Traits<Tag>::CacheLineSize) {
            *Flush64 = line;
        }
    }

    static void invalidate(const void *const pointer, size_t size) { flush(pointer, size); }

    static void barrier() { asm volatile("fence iorw, iorw" ::: "memory"); }

    static void init() {
        *WayEnable |= 0xFF;
        if constexpr (Traits<Tag>::CoreIsolation) {
            size_t core        = CPU::id() + Traits<CPU>::Offset;
            size_t ways        = Traits<Tag>::NumberOfWays / Traits<CPU>::Active;
            uint64_t activated = (((1ULL << ways) - 1) << (core * ways));
            color(activated, core);
        }
    }

  private:
    static constexpr uintptr_t Address     = 0x2010000;
    static inline uint64_t *const WayMask  = reinterpret_cast<uint64_t *>(Address + 0x800);
    static inline uint64_t *const Flush64  = reinterpret_cast<uint64_t *>(Address + 0x200);
    static inline uint64_t *const Flush32  = reinterpret_cast<uint64_t *>(Address + 0x240);
    static inline uint8_t *const WayEnable = reinterpret_cast<uint8_t *>(Address + 0x8);
};

} // namespace QUARK
