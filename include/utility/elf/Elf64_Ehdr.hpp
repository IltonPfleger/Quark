#ifndef __QUARK_ELF64_EHDR__
#define __QUARK_ELF64_EHDR__

#include <utility/elf/Elf64_Phdr.hpp>

namespace QUARK {

class Elf64_Ehdr {
  public:
    bool valid() {
        if (e_ident[0] != 0x7F || e_ident[1] != 'E' || e_ident[2] != 'L' || e_ident[3] != 'F') return false;
        return true;
    }

    size_t length() const {
        size_t size = sizeof(Elf64_Ehdr);
        auto *phdr  = reinterpret_cast<const Elf64_Phdr *>(reinterpret_cast<const char *>(this) + e_phoff);

        for (size_t i = 0; i < e_phnum; ++i) {
            size_t end = phdr[i].p_offset + phdr[i].p_filesz;
            if (end > size) size = end;
        }
        return size;
    }

  public:
    uint8_t e_ident[16];

    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;

    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;

    uint32_t e_flags;

    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;

    uint16_t e_shentsize;
    uint16_t e_shnum;

    uint16_t e_shstrndx;
};

} // namespace QUARK

#endif
