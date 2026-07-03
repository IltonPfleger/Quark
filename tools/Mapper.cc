#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <elf.h>

#include "BootInformation.hpp"

struct SectionRange {
    uintptr_t start  = 0;
    uintptr_t end    = 0;
    bool initialized = false;

    void expand(uint64_t addr, uint64_t size) {
        if (!initialized || addr < start) start = addr;
        if (!initialized || (addr + size) > end) end = addr + size;
        initialized = true;
    }

    uintptr_t size() const { return end - start; }
};

int main(int argc, char *argv[]) {
    using Chunk   = QUARK::Chunk;
    using Payload = QUARK::BootInformation::Payload;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <ELF> <output file>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    Elf64_Ehdr ehdr;
    if (fread(&ehdr, 1, sizeof(ehdr), f) != sizeof(ehdr)) {
        fclose(f);
        return 1;
    }

    // --- 1. Calcular o limite real de memória (max_addr) ---
    fseek(f, ehdr.e_phoff, SEEK_SET);
    Elf64_Phdr *phdrs = (Elf64_Phdr *)malloc(ehdr.e_phnum * sizeof(Elf64_Phdr));
    fread(phdrs, sizeof(Elf64_Phdr), ehdr.e_phnum, f);

    uintptr_t max_addr = 0;
    for (int i = 0; i < ehdr.e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            max_addr = std::max((uintptr_t)max_addr, (uintptr_t)(phdrs[i].p_vaddr + phdrs[i].p_memsz));
        }
    }
    free(phdrs);

    fseek(f, ehdr.e_shoff, SEEK_SET);
    Elf64_Shdr *shdrs = (Elf64_Shdr *)malloc(ehdr.e_shnum * sizeof(Elf64_Shdr));
    fread(shdrs, sizeof(Elf64_Shdr), ehdr.e_shnum, f);

    Elf64_Shdr strtab_hdr = shdrs[ehdr.e_shstrndx];
    char *shstrtab        = (char *)malloc(strtab_hdr.sh_size);
    fseek(f, strtab_hdr.sh_offset, SEEK_SET);
    fread(shstrtab, 1, strtab_hdr.sh_size, f);

    SectionRange text, data, rodata, bss;

    for (int i = 0; i < ehdr.e_shnum; i++) {
        const char *name = shstrtab + shdrs[i].sh_name;
        uintptr_t addr   = shdrs[i].sh_addr;
        uintptr_t size   = shdrs[i].sh_size;

        if (strcmp(name, ".text") == 0) {
            text.expand(addr, size);
        } else if (strcmp(name, ".data") == 0 || strcmp(name, ".sdata") == 0) {
            data.expand(addr, size);
        } else if (strcmp(name, ".rodata") == 0) {
            rodata.expand(addr, size);
        } else if (strcmp(name, ".bss") == 0 || strcmp(name, ".sbss") == 0) {
            bss.expand(addr, size);
        }
    }

    Payload payload{};

    if (text.initialized) new (&payload.text) Chunk(text.start, text.size());
    if (data.initialized) new (&payload.data) Chunk(data.start, data.size());
    if (rodata.initialized) new (&payload.rodata) Chunk(rodata.start, rodata.size());
    if (bss.initialized) new (&payload.bss) Chunk(bss.start, bss.size());

    payload.main = ehdr.e_entry;

    FILE *file = fopen(argv[2], "wb");
    if (file) {
        fwrite(&payload, sizeof(Payload), 1, file);
        fclose(file);
    }

    free(shstrtab);
    free(shdrs);
    fclose(f);
    return 0;
}
