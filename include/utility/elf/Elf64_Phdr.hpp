#ifndef __QUARK_ELF64_PHDR__
#define __QUARK_ELF64_PHDR__

namespace QUARK {

class Elf64_Phdr {
  public:
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;

  public:
    static constexpr int PT_LOAD = 1;
};

} // namespace QUARK

#endif
