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
            TLB::flush();
            csrw<SupervisorMode::SATP>(Mode | reinterpret_cast<uintptr_t>(this) >> 12);
        }

        static uintptr_t virt2phys(uintptr_t va) {
            uintptr_t vpn2 = (va >> 30) & 0x1FF;
            uintptr_t vpn1 = (va >> 21) & 0x1FF;
            uintptr_t vpn0 = (va >> 12) & 0x1FF;

            uintptr_t satp = csrr<SupervisorMode::SATP>();

            if (satp == 0) return va;

            uintptr_t root     = satp & 0xFFFFFFFFFFF;
            PageTable *current = reinterpret_cast<PageTable *>(root << 12);

            uintptr_t pte2 = current->m_entries[vpn2];
            assert(pte2 & V);
            if (pte2 & (R | W | X)) {
                return ((pte2 >> 10) << 12) | (va & 0x3FFFFFFF);
            }

            PageTable *l1  = reinterpret_cast<PageTable *>((pte2 >> 10) << 12);
            uintptr_t pte1 = l1->m_entries[vpn1];
            assert(pte1 & V);
            if (pte1 & (R | W | X)) {
                return ((pte1 >> 10) << 12) | (va & 0x1FFFFF);
            }

            PageTable *l0  = reinterpret_cast<PageTable *>((pte1 >> 10) << 12);
            uintptr_t pte0 = l0->m_entries[vpn0];
            assert(pte0 & V);

            return ((pte0 >> 10) << 12) | (va & 0xFFF);
        }

        bool map(uintptr_t va, uintptr_t pa, Flags flags) {
            uintptr_t vpn2 = (va >> 30) & 0x1FF;
            uintptr_t vpn1 = (va >> 21) & 0x1FF;
            uintptr_t vpn0 = (va >> 12) & 0x1FF;

            PageTable *l1;
            PageTable *l0;

            if (!m_entries[vpn2]) {
                l1 = PageTable::alloc();
                set(vpn2, reinterpret_cast<uintptr_t>(l1), V);
            } else {
                l1 = walk(vpn2);
            }
            if (!l1->m_entries[vpn1]) {
                l0 = PageTable::alloc();
                l1->set(vpn1, reinterpret_cast<uintptr_t>(l0), V);
            } else {
                l0 = l1->walk(vpn1);
            }

            return l0->set(vpn0, reinterpret_cast<uintptr_t>(pa), flags);
        }

        bool map(uintptr_t va, Flags flags) { return map(va, va, flags); }

        void map(uintptr_t va, uintptr_t pa, size_t size, Flags flags) {
            if ((size % Giga == 0) && ((va % Giga == 0) && (pa % Giga == 0))) {
                uintptr_t vpn2 = (va >> 30) & 0x1FF;
                this->set(vpn2, pa, flags);
            } else {
                uintptr_t end = pa + size;
                for (; pa < end;) {
                    map(va, pa, flags);
                    va += Size;
                    pa += Size;
                }
            }
        }

      private:
        bool set(int vpn, uintptr_t addr, Flags flags) {
            if (m_entries[vpn]) return false;
            m_entries[vpn] = (addr >> 2) | flags;
            return true;
        }

        PageTable *walk(int vpn) {
            uintptr_t pte  = m_entries[vpn];
            uintptr_t addr = (pte >> 10) << 12;
            return reinterpret_cast<PageTable *>(addr);
        }

      private:
        static constexpr auto Size          = 4096;
        static constexpr auto EntriesNumber = 512;
        alignas(Size) uintptr_t m_entries[EntriesNumber];
    };

  public:
    static void init() {
        auto mm = __amm.start();

        if (CPU::id() == Traits<CPU>::BSP) {
            s_kernel_page_table = PageTable();
            s_kernel_page_table.map(Traits<MemoryMap>::RamStart, mm, Giga, PageTable::KernelRWX);
            s_kernel_page_table.map(mm, mm, Giga, PageTable::KernelRWX);
            s_kernel_page_table.map(Traits<MemoryMap>::MMIO, Traits<MemoryMap>::MMIO, Giga, PageTable::KernelRWX);
        }

        CPU::barrier();

        s_kernel_page_table.load();
    }

  private:
    static constexpr unsigned long Mode = 8UL << 60;
    static constexpr unsigned long Giga = (1 << 30);
    static inline PageTable s_kernel_page_table;
};

} // namespace QUARK
