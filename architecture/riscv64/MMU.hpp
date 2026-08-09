#pragma once

#include <architecture/CPU.hpp>
#include <architecture/riscv64/Modes.hpp>
#include <architecture/riscv64/csrs.hpp>
#include <memory/Memory.hpp>

namespace QUARK {

class MMU {
  public:
    class TLB {
      public:
        static auto flush() { asm("sfence.vma zero, zero"); }
    };

    class PageTable {
        friend MMU;

      public:
        PageTable() = default;

        enum Flags {
            V = 1 << 0, // Valid
            R = 1 << 1, // Readable
            W = 1 << 2, // Writable
            X = 1 << 3, // Executable
            U = 1 << 4, // User accessible
            G = 1 << 5, // Global
            A = 1 << 6, // Accessed
            D = 1 << 7, // Dirty

            UserRO = V | R | U | A | D,
            UserRX = V | R | X | U | A | D,
            UserRW = V | R | W | U | A | D,

            KernelRO  = V | R | A | D,
            KernelRX  = V | R | X | A | D,
            KernelRW  = V | R | W | A | D,
            KernelRWX = V | R | W | X | A | D,
        };

        static PageTable *alloc() {
            PageTable *pg = reinterpret_cast<PageTable *>(Memory::alloc(sizeof(PageTable)));
            *pg           = PageTable();
            return pg;
        }

        void load() const {
            csrw<SupervisorMode::SATP>(Mode | reinterpret_cast<uintptr_t>(this) >> 12);
            TLB::flush();
        }

        static uintptr_t virt2phys(uintptr_t va) {
            uintptr_t satp = csrr<SupervisorMode::SATP>();

            if ((satp >> 60) == 0) return va;

            uintptr_t vpn2 = (va >> 30) & 0x1FF;
            uintptr_t vpn1 = (va >> 21) & 0x1FF;
            uintptr_t vpn0 = (va >> 12) & 0x1FF;

            uintptr_t root     = (satp & 0xFFFFFFFFFFF) << 12;
            PageTable *current = reinterpret_cast<PageTable *>(root);

            uintptr_t pte2 = current->entries_[vpn2];
            assert(pte2 & V);
            if (pte2 & (R | W | X)) {
                return ((pte2 >> 10) << 12) | (va & 0x3FFFFFFF);
            }

            PageTable *l1  = reinterpret_cast<PageTable *>((pte2 >> 10) << 12);
            uintptr_t pte1 = l1->entries_[vpn1];
            assert(pte1 & V);
            if (pte1 & (R | W | X)) {
                return ((pte1 >> 10) << 12) | (va & 0x1FFFFF);
            }

            PageTable *l0  = reinterpret_cast<PageTable *>((pte1 >> 10) << 12);
            uintptr_t pte0 = l0->entries_[vpn0];
            assert(pte0 & V);

            return ((pte0 >> 10) << 12) | (va & 0xFFF);
        }

        bool map(uintptr_t va, uintptr_t pa, Flags flags) {
            uintptr_t vpn2 = (va >> 30) & 0x1FF;
            uintptr_t vpn1 = (va >> 21) & 0x1FF;
            uintptr_t vpn0 = (va >> 12) & 0x1FF;

            PageTable *l1;
            PageTable *l0;

            if (!entries_[vpn2]) {
                l1 = PageTable::alloc();
                set(vpn2, reinterpret_cast<uintptr_t>(l1), V);
            } else {
                l1 = walk(vpn2);
            }
            if (!l1->entries_[vpn1]) {
                l0 = PageTable::alloc();
                l1->set(vpn1, reinterpret_cast<uintptr_t>(l0), V);
            } else {
                l0 = l1->walk(vpn1);
            }

            return l0->set(vpn0, reinterpret_cast<uintptr_t>(pa), flags);
        }

        void map(uintptr_t va, uintptr_t pa, size_t size, Flags flags) {
            if ((size % Giga == 0) && (va % Giga == 0) && (pa % Giga == 0)) {
                size_t pages = size / Giga;

                for (size_t i = 0; i < pages; i++) {
                    uintptr_t vpn2 = ((va + i * Giga) >> 30) & 0x1FF;
                    set(vpn2, pa + i * Giga, flags);
                }

                return;
            }

            if ((size % Mega == 0) && (va % Mega == 0) && (pa % Mega == 0)) {
                size_t pages = size / Mega;

                for (size_t i = 0; i < pages; i++) {
                    uintptr_t cva = va + i * Mega;
                    uintptr_t cpa = pa + i * Mega;

                    uintptr_t vpn2 = (cva >> 30) & 0x1FF;
                    uintptr_t vpn1 = (cva >> 21) & 0x1FF;

                    PageTable *l1;

                    if (!entries_[vpn2]) {
                        l1 = PageTable::alloc();
                        set(vpn2, reinterpret_cast<uintptr_t>(l1), V);
                    } else {
                        l1 = walk(vpn2);
                    }

                    l1->set(vpn1, cpa, flags);
                }

                return;
            }

            uintptr_t end = pa + size;

            while (pa < end) {
                map(va, pa, flags);
                va += Size;
                pa += Size;
            }
        }

        bool empty() {
            for (size_t i = 0; i < EntriesNumber; i++) {
                if (entries_[i] & V) return false;
            }
            return true;
        }

        void unmap(uintptr_t va, size_t size) {
            if ((size % Giga == 0) && (va % Giga == 0)) {
                size_t pages = size / Giga;

                for (size_t i = 0; i < pages; i++) {
                    uintptr_t vpn2 = ((va + i * Giga) >> 30) & 0x1FF;
                    entries_[vpn2] = 0;
                }

                TLB::flush();
                return;
            }

            if ((size % Mega == 0) && (va % Mega == 0)) {
                size_t pages = size / Mega;

                for (size_t i = 0; i < pages; i++) {
                    uintptr_t cva = va + i * Mega;

                    uintptr_t vpn2 = (cva >> 30) & 0x1FF;
                    uintptr_t vpn1 = (cva >> 21) & 0x1FF;

                    if (!(entries_[vpn2] & V)) continue;

                    PageTable *l1 = walk(vpn2);

                    l1->entries_[vpn1] = 0;

                    if (l1->empty()) {
                        Memory::free(l1, sizeof(PageTable));
                        entries_[vpn2] = 0;
                    }
                }

                TLB::flush();
                return;
            }

            size_t pages = (size + Size - 1) / Size;

            for (size_t i = 0; i < pages; i++) {
                uintptr_t cva = va + i * Size;

                uintptr_t vpn2 = (cva >> 30) & 0x1FF;
                uintptr_t vpn1 = (cva >> 21) & 0x1FF;
                uintptr_t vpn0 = (cva >> 12) & 0x1FF;

                if (!(entries_[vpn2] & V)) continue;

                PageTable *l1 = walk(vpn2);

                if (!(l1->entries_[vpn1] & V)) continue;

                PageTable *l0 = l1->walk(vpn1);

                l0->entries_[vpn0] = 0;

                if (l0->empty()) {
                    Memory::free(l0, sizeof(PageTable));
                    l1->entries_[vpn1] = 0;

                    if (l1->empty()) {
                        Memory::free(l1, sizeof(PageTable));
                        entries_[vpn2] = 0;
                    }
                }
            }

            TLB::flush();
        }

      private:
        bool set(int vpn, uintptr_t addr, Flags flags) {
            if (entries_[vpn]) return false;
            entries_[vpn] = ((addr >> 12) << 10) | flags;
            return true;
        }

        PageTable *walk(int vpn) {
            uintptr_t pte  = entries_[vpn];
            uintptr_t addr = (pte >> 10) << 12;
            return reinterpret_cast<PageTable *>(addr);
        }

      private:
        static constexpr auto Size          = 4096;
        static constexpr auto EntriesNumber = 512;
        alignas(Size) uintptr_t entries_[EntriesNumber];
    };

  public:
    static void prologue() {
        base_ = PageTable();

        for (size_t i = 0; i < 256; i++) {
            uintptr_t va = Traits<MemoryMap>::RamStart - i * Giga;
            uintptr_t pa = __amm.start() + i * Giga;
            base_.map(va, pa, Giga, PageTable::KernelRWX);
        }

        base_.map(__amm.start(), __amm.start(), Giga, PageTable::KernelRWX);
        base_.map(Traits<MemoryMap>::MMIO, Traits<MemoryMap>::MMIO, Giga, PageTable::KernelRWX);
    }

    static void init() { base_.load(); }

    static void epilogue() { base_.unmap(__amm.start(), Giga); }

  private:
    static constexpr uintmax_t Mode = 8UL << 60;
    static constexpr size_t Mega    = 1024 * 1024;
    static constexpr size_t Giga    = Mega * 1024;
    static inline PageTable base_;
};

} // namespace QUARK
