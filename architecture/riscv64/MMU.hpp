#ifndef __QUARK_RISCV64_MMU__
#define __QUARK_RISCV64_MMU__

#include <architecture/CPU.hpp>
#include <architecture/riscv64/Modes.hpp>
#include <architecture/riscv64/csrs.hpp>
#include <memory/Memory.hpp>

namespace QUARK {

class MMU {
public:
  static_assert(Traits<Memory>::PageSize == 4096);

  class TLB {
  public:
    static auto flush() { asm volatile("sfence.vma zero, zero" ::: "memory"); }
  };

  class PageTable {
    friend MMU;

  public:
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
      UserRWX = V | R | W | X | U | A | D,

      KernelRO = V | R | A | D,
      KernelRX = V | R | X | A | D,
      KernelRW = V | R | W | A | D,
      KernelRWX = V | R | W | X | A | D,
    };

    constexpr PageTable() : entries_() {};

    static PageTable *clone() {
      PageTable *pt = alloc();

      Meta::Execute(base_, [&](auto &base) { *pt = base; });

      return pt;
    }

    static PageTable *alloc() {
      return new (Memory::alloc(sizeof(PageTable))) PageTable();
    }

    void activate() const {
      uintptr_t physical = Memory::virt2phys(reinterpret_cast<uintptr_t>(this));
      load(physical);
    }

    static uintptr_t virt2phys(uintptr_t va) {
      uintptr_t satp = csrr<SupervisorMode::SATP>();

      if ((satp >> 60) == 0)
        return va;

      uintptr_t vpn2 = (va >> 30) & 0x1FF;
      uintptr_t vpn1 = (va >> 21) & 0x1FF;
      uintptr_t vpn0 = (va >> 12) & 0x1FF;

      uintptr_t root = (satp & 0xFFFFFFFFFFF) << 12;
      PageTable *current = reinterpret_cast<PageTable *>(root);

      uintptr_t pte2 = current->entries_[vpn2];
      assert(pte2 & V);
      if (pte2 & (R | W | X)) {
        return ((pte2 >> 10) << 12) | (va & 0x3FFFFFFF);
      }

      PageTable *l1 = reinterpret_cast<PageTable *>((pte2 >> 10) << 12);
      uintptr_t pte1 = l1->entries_[vpn1];
      assert(pte1 & V);
      if (pte1 & (R | W | X)) {
        return ((pte1 >> 10) << 12) | (va & 0x1FFFFF);
      }

      PageTable *l0 = reinterpret_cast<PageTable *>((pte1 >> 10) << 12);
      uintptr_t pte0 = l0->entries_[vpn0];
      assert(pte0 & V);

      return ((pte0 >> 10) << 12) | (va & 0xFFF);
    }

    bool find(int level, uintptr_t base, size_t size, uintptr_t &start,
              size_t &free) const {
      const size_t shift = 12 + (level * 9);
      const size_t capacity = 1ULL << shift;

      for (size_t vpn = 0; vpn < 512; vpn++) {
        uintptr_t entry = entries_[vpn];
        uintptr_t va = base | (vpn << shift);

        if (!entry) {
          if (free == 0)
            start = va;
          free += capacity;
          if (free >= size)
            return true;
        } else if (level == 0 || (entry & (R | W | X))) {
          free = 0;
        } else {
          auto *next =
              reinterpret_cast<PageTable *>(Memory::phys2virt(walk(vpn)));
          if (next->find(level - 1, va, size, start, free)) {
            return true;
          }
        }
      }

      return false;
    }

    uintptr_t find(size_t size) const {
      uintptr_t start = 0;
      size_t free = 0;

      if (find(2, 0, size, start, free))
        return start;

      return 0;
    }

    bool page(uintptr_t va, uintptr_t pa, Flags flags) {
      uintptr_t vpn2 = (va >> 30) & 0x1FF;
      uintptr_t vpn1 = (va >> 21) & 0x1FF;
      uintptr_t vpn0 = (va >> 12) & 0x1FF;

      PageTable *l1;
      PageTable *l0;

      if (!entries_[vpn2]) {
        l1 = PageTable::alloc();
        set(vpn2, l1->physical(), V);
      } else {
        l1 = reinterpret_cast<PageTable *>(Memory::phys2virt(walk(vpn2)));
      }
      if (!l1->entries_[vpn1]) {
        l0 = PageTable::alloc();
        l1->set(vpn1, l0->physical(), V);
      } else {
        l0 = reinterpret_cast<PageTable *>(Memory::phys2virt(l1->walk(vpn1)));
      }

      return l0->set(vpn0, pa, flags);
    }

    bool megapage(uintptr_t va, uintptr_t pa, Flags flags) {
      assert((va % Mega == 0) && (pa % Mega == 0));

      uintptr_t vpn2 = (va >> 30) & 0x1FF;
      uintptr_t vpn1 = (va >> 21) & 0x1FF;

      PageTable *l1;

      if (!entries_[vpn2]) {
        l1 = PageTable::alloc();
        set(vpn2, l1->physical(), V);
      } else {
        l1 = reinterpret_cast<PageTable *>(Memory::phys2virt(walk(vpn2)));
      }

      return l1->set(vpn1, pa, flags);
    }

    bool gigapage(uintptr_t va, uintptr_t pa, Flags flags) {
      assert((va % Giga == 0) && (pa % Giga == 0));

      uintptr_t vpn2 = (va >> 30) & 0x1FF;

      return set(vpn2, pa, flags);
    }

    bool map(uintptr_t va, uintptr_t pa, size_t size, Flags flags) {
      while (size) {
        if ((va % Giga == 0) && (pa % Giga == 0) && size >= Giga) {
          if (!gigapage(va, pa, flags))
            return false;

          va += Giga;
          pa += Giga;
          size -= Giga;
        } else if ((va % Mega == 0) && (pa % Mega == 0) && size >= Mega) {
          if (!megapage(va, pa, flags))
            return false;

          va += Mega;
          pa += Mega;
          size -= Mega;
        } else {
          if ((va % Size != 0) || (pa % Size != 0))
            return false;

          if (!page(va, pa, flags))
            return false;

          va += Size;
          pa += Size;
          size -= Size;
        }
      }

      TLB::flush();
      return true;
    }

    bool empty() const {
      for (size_t i = 0; i < EntriesNumber; i++) {
        if (entries_[i] & V)
          return false;
      }
      return true;
    }

    void ungiga(uintptr_t va) {
      assert(va % Giga == 0);

      uintptr_t vpn2 = (va >> 30) & 0x1FF;
      entries_[vpn2] = 0;

      TLB::flush();
    }

  private:
    uintptr_t physical() {
      return Memory::virt2phys(reinterpret_cast<uintptr_t>(this));
    }

    bool set(int vpn, uintptr_t addr, Flags flags) {
      if (entries_[vpn])
        return false;
      entries_[vpn] = ((addr >> 12) << 10) | flags;
      return true;
    }

    uintptr_t walk(int vpn) const {
      uintptr_t pte = entries_[vpn];
      uintptr_t address = (pte >> 10) << 12;
      return address;
    }

  private:
    static constexpr auto Size = 4096;
    static constexpr auto EntriesNumber = 512;
    alignas(Size) uintptr_t entries_[EntriesNumber];
  };

  static void load(uintptr_t pt) {
    uintptr_t satp = Mode | pt >> 12;
    csrw<SupervisorMode::SATP>(satp);
    TLB::flush();
  }

public:
  static void prologue() {
    Meta::Execute(base_, [](auto &base) {
      new (&base) PageTable();

      for (size_t i = 0; i < 256; i++) {
        uintptr_t va = Traits<MemoryMap>::RamStart - i * Giga;
        uintptr_t pa = __amm.start() + i * Giga;
        base.gigapage(va, pa, PageTable::KernelRWX);
      }

      uintptr_t mmio = Traits<MemoryMap>::MMIO;
      base.gigapage(mmio, mmio, PageTable::KernelRWX);
      base.gigapage(__amm.start(), __amm.start(), PageTable::KernelRWX);
    });
  }

  static void init() { load(reinterpret_cast<uintptr_t>(&base_)); }

  static void epilogue() {
    Meta::Execute(base_, [](auto &base) { base.ungiga(__amm.start()); });
  }

private:
  static constexpr bool Enable = Traits<Kernel>::Multitask;
  static constexpr uintmax_t Mode = 8UL << 60;
  static constexpr size_t Mega = 1024 * 1024;
  static constexpr size_t Giga = Mega * 1024;
  static inline Meta::IF<Enable, PageTable, Meta::Empty>::Result base_;
};

} // namespace QUARK

#endif
