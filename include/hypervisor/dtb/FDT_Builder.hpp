#ifndef __QUARK_FDT_BUILDER__
#define __QUARK_FDT_BUILDER__

#include <architecture/CPU.hpp>
#include <hypervisor/dtb/FDT_Header.hpp>
#include <hypervisor/dtb/FDT_ReservedEntry.hpp>
#include <libraries/libc/string.h>

namespace QUARK {

class FDT_Builder {
    static constexpr uint32_t FDT_MAGIC      = 0xd00dfeed;
    static constexpr uint32_t FDT_BEGIN_NODE = 0x1;
    static constexpr uint32_t FDT_END_NODE   = 0x2;
    static constexpr uint32_t FDT_PROP       = 0x3;
    static constexpr uint32_t FDT_END        = 0x9;

  public:
    FDT_Builder(void *buffer, size_t capacity)
        : buffer_(reinterpret_cast<uint8_t *>(buffer)),
          capacity_(capacity),
          cursor_(sizeof(FDT_Header)),
          strings_cursor_(capacity) {
        write64(0);
        write64(0);
    }

    void begin(const char *name) {
        write32(FDT_BEGIN_NODE);
        while (*name) {
            assert(cursor_ < strings_cursor_);
            buffer_[cursor_++] = *name++;
        }

        assert(cursor_ < strings_cursor_);
        buffer_[cursor_++] = 0;

        align(4);
    }

    void add(const char *name) { add_property(name, nullptr, 0); }
    void add(const char *name, const char *value) { add_property(name, value, strlen(value) + 1); }
    void add(const char *name, uint32_t value) { add_property(name, &((value = CPU::htobe32(value))), sizeof(value)); }

    void add(const char *name, uint32_t *values, size_t length) {
        for (size_t i = 0; i < length; i++)
            values[i] = CPU::htobe32(values[i]);
        add_property(name, values, length * sizeof(uint32_t));
    }

    void end() { write32(FDT_END_NODE); }

    size_t finish() {
        write32(FDT_END);
        align(4);

        size_t end          = cursor_;
        size_t strings_size = capacity_ - strings_cursor_;
        size_t p            = sizeof(FDT_Header) + sizeof(FDT_ReservedEntry);

        while (p + 4 < end) {
            uint32_t tag = CPU::be32toh(*reinterpret_cast<uint32_t *>(buffer_ + p));
            if (tag == FDT_BEGIN_NODE) {
                p += 4;
                while (p < end && buffer_[p] != 0) {
                    p++;
                }
                p++;
                p = (p + 3) & ~3;
            } else if (tag == FDT_END_NODE) {
                p += 4;
            } else if (tag == FDT_PROP) {
                assert(p + 12 <= end);
                uint32_t length           = CPU::be32toh(*reinterpret_cast<uint32_t *>(buffer_ + p + 4));
                uint32_t string           = CPU::be32toh(*reinterpret_cast<uint32_t *>(buffer_ + p + 8));
                uint32_t be_string_offset = CPU::htobe32(strings_size - string);
                memcpy(buffer_ + p + 8, &be_string_offset, 4);
                p += 12 + length;
                p = (p + 3) & ~3;
            } else if (tag == FDT_END) {
                break;
            } else {
                break;
            }
        }

        memcpy(buffer_ + end, buffer_ + strings_cursor_, strings_size);

        FDT_Header *header        = reinterpret_cast<FDT_Header *>(buffer_);
        header->magic             = CPU::htobe32(FDT_MAGIC);
        header->totalsize         = CPU::htobe32(end + strings_size);
        header->off_dt_struct     = CPU::htobe32(sizeof(FDT_Header) + sizeof(FDT_ReservedEntry));
        header->off_dt_strings    = CPU::htobe32(end);
        header->off_mem_rsvmap    = CPU::htobe32(sizeof(FDT_Header));
        header->version           = CPU::htobe32(17);
        header->last_comp_version = CPU::htobe32(16);
        header->boot_cpuid_phys   = 0;
        header->size_dt_strings   = CPU::htobe32(strings_size);
        header->size_dt_struct    = CPU::htobe32(end - sizeof(FDT_Header) + sizeof(FDT_ReservedEntry));

        return end + strings_size;
    }

  private:
    uint8_t *buffer_;
    size_t capacity_;
    size_t cursor_;
    size_t strings_cursor_;

    void align(size_t n) {
        while (cursor_ % n && cursor_ < strings_cursor_) {
            buffer_[cursor_++] = 0;
        }
    }

    void write32(uint32_t v) {
        assert(cursor_ + 4 <= strings_cursor_);
        uint32_t be = CPU::htobe32(v);
        memcpy(buffer_ + cursor_, &be, 4);
        cursor_ += 4;
    }

    void write64(uint64_t v) {
        assert(cursor_ + 8 <= strings_cursor_);
        uint64_t be = CPU::htobe64(v);
        memcpy(buffer_ + cursor_, &be, 8);
        cursor_ += 8;
    }

    uint32_t add_string(const char *string) {
        size_t length = strlen(string) + 1;
        assert(cursor_ + length <= strings_cursor_);
        strings_cursor_ -= length;
        memcpy(buffer_ + strings_cursor_, string, length);
        return static_cast<uint32_t>(capacity_ - strings_cursor_);
    }

    void add_property(const char *name, const void *data, uint32_t length) {
        write32(FDT_PROP);
        write32(length);

        write32(add_string(name));

        assert(cursor_ + length <= strings_cursor_);
        memcpy(buffer_ + cursor_, data, length);
        cursor_ += length;

        align(4);
    }
};

} // namespace QUARK

#endif
