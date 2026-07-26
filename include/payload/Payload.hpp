#pragma once

#include <BootInformation.hpp>
#include <Thread.hpp>
#include <Traits.hpp>
#include <libraries/libc/string.h>
#include <memory/Heap.hpp>
#include <memory/Memory.hpp>
#include <utility/ELF/Elf_Ehdr.hpp>
#include <utility/ELF/Elf_Phdr.hpp>

namespace QUARK {

class Payload : Traits<Payload> {
  public:
    static void alloc() {
        Elf_Ehdr *header = reinterpret_cast<Elf_Ehdr *>(Image);
        bool valid       = validate(header);
        assert(valid);
        entry_ = header->e_entry;
        load(header);
    }

    static bool validate(Elf_Ehdr *header) {
        if (!header->valid()) return false;

        Elf_Phdr *list = reinterpret_cast<Elf_Phdr *>(Image + header->e_phoff);

        uintptr_t start = ~0ULL;
        uintptr_t end   = 0;

        for (size_t i = 0; i < header->e_phnum; ++i) {
            Elf_Phdr &phdr = list[i];
            if (phdr.p_type != Elf_Phdr::PT_LOAD) continue;
            if (phdr.p_vaddr < start) start = phdr.p_vaddr;
            if (phdr.p_vaddr + phdr.p_memsz > end) end = phdr.p_vaddr + phdr.p_memsz;
        }

        size_t size = end - start;

        new (&__pmm) Chunk(reinterpret_cast<void *>(start), size);

        return !__pmm.overlaps(BootInformation::kernel());
    }

    static void load(Elf_Ehdr *header) {
        Elf_Phdr *list = reinterpret_cast<Elf_Phdr *>(Image + header->e_phoff);
        for (size_t i = 0; i < header->e_phnum; ++i) {
            Elf_Phdr &phdr = list[i];
            if (phdr.p_type == Elf_Phdr::PT_LOAD) {
                void *destination  = reinterpret_cast<void *>(phdr.p_vaddr);
                const void *source = reinterpret_cast<const void *>(Image + phdr.p_offset);
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

    static void init() {
        auto main = reinterpret_cast<Thread::Return (*)(Thread::Argument)>(entry_);
        new Thread(main, 0, Thread::Criterion::NORMAL);
    };

  private:
    __attribute__((section(".payload"), used)) static inline uint8_t Image[Size];
    static inline uintptr_t entry_;
};

} // namespace QUARK
