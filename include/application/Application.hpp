#pragma once

#include <BootInformation.hpp>
#include <Process.hpp>
#include <Thread.hpp>
#include <Traits.hpp>
#include <libraries/libc/string.h>
#include <memory/Heap.hpp>
#include <memory/Memory.hpp>
#include <utility/elf/Elf_Ehdr.hpp>
#include <utility/elf/Elf_Phdr.hpp>

namespace QUARK {

class Application {
public:
  static void reserve() {
    Elf_Ehdr *header = reinterpret_cast<Elf_Ehdr *>(image());

    assert(header->valid());

    Elf_Phdr *list = reinterpret_cast<Elf_Phdr *>(image() + header->e_phoff);

    uintptr_t start = ~0ULL;
    uintptr_t end = 0;

    for (size_t i = 0; i < header->e_phnum; ++i) {
      Elf_Phdr &phdr = list[i];
      if (phdr.p_type != Elf_Phdr::PT_LOAD)
        continue;
      if (phdr.p_vaddr < start)
        start = phdr.p_vaddr;
      if (phdr.p_vaddr + phdr.p_memsz > end)
        end = phdr.p_vaddr + phdr.p_memsz;
    }

    size_t size = end - start;

    new (&__emm) Chunk(header, header->length());
    new (&__pmm) Chunk(start, size);

    assert(!__pmm.overlaps(BootInformation::all()));
  }

  static void direct(Elf_Ehdr *header) {
    Elf_Phdr *list = reinterpret_cast<Elf_Phdr *>(image() + header->e_phoff);
    for (size_t i = 0; i < header->e_phnum; ++i) {
      Elf_Phdr &phdr = list[i];
      if (phdr.p_type == Elf_Phdr::PT_LOAD) {
        void *destination = reinterpret_cast<void *>(phdr.p_vaddr);
        const void *source =
            reinterpret_cast<const void *>(image() + phdr.p_offset);
        if (phdr.p_filesz > 0) {
          memcpy(destination, source, phdr.p_filesz);
        }

        if (phdr.p_memsz > phdr.p_filesz) {
          void *bss = reinterpret_cast<void *>(phdr.p_vaddr + phdr.p_filesz);
          memset(bss, 0, phdr.p_memsz - phdr.p_filesz);
        }
      }
    }
  }

  static void indirect(Process *process) {
    Elf_Ehdr *header = reinterpret_cast<Elf_Ehdr *>(image());
    Elf_Phdr *list = reinterpret_cast<Elf_Phdr *>(image() + header->e_phoff);

    for (size_t i = 0; i < header->e_phnum; ++i) {
      Elf_Phdr &phdr = list[i];

      if (!(phdr.p_type == Elf_Phdr::PT_LOAD))
        continue;

      if (phdr.p_filesz <= 0)
        continue;

      size_t pagesize = Traits<Memory>::PageSize;
      size_t length = phdr.p_memsz;
      length = (length + pagesize - 1) & ~(pagesize - 1);

      const uint8_t *source = image() + phdr.p_offset;
      const uint8_t *ua = reinterpret_cast<const uint8_t *>(phdr.p_vaddr);
      uint8_t *ka = reinterpret_cast<uint8_t *>(Memory::alloc(length));
      uintptr_t pa = Memory::virt2phys(reinterpret_cast<uintptr_t>(ka));

      memcpy(ka, source, phdr.p_filesz);

      if (phdr.p_memsz > phdr.p_filesz)
        memset(ka + phdr.p_filesz, 0, phdr.p_memsz - phdr.p_filesz);

      process->attach(Chunk(ua, length), Chunk(pa, length));
    }
  }

  static uint8_t *image() {
    return reinterpret_cast<uint8_t *>(BootInformation::all().end());
  }

  static void init() {
    TraceIn();

    using Function = Thread::Return (*)(Thread::Argument);

    Elf_Ehdr *header = reinterpret_cast<Elf_Ehdr *>(image());
    auto main = reinterpret_cast<Function>(header->e_entry);

    if constexpr (Traits<Kernel>::Mode == Traits<Kernel>::LIBRARY) {
      direct(header);
      new (Heap::SYSTEM) Thread(main, 0, Thread::Criterion::NORMAL);
    } else {
      Process *process = new (Heap::SYSTEM) Process();
      indirect(process);
      new (Heap::SYSTEM)
          Thread(main, 0, Thread::Criterion::NORMAL, Thread::USER, process);
      process->activate();
    }

    TraceOut();
  };
};

} // namespace QUARK
