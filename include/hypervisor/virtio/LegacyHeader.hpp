#pragma once

namespace QUARK::virtio {

class LegacyHeader {
  public:
    volatile uint32_t magic_value; // 0x000 (R) 'virt'
    volatile uint32_t version;     // 0x004 (R) 1 for Legacy
    volatile uint32_t device_id;   // 0x008 (R)
    volatile uint32_t vendor_id;   // 0x00c (R) 0x554d4551

    volatile uint32_t host_features;     // 0x010 (R)
    volatile uint32_t host_features_sel; // 0x014 (W)

    volatile uint32_t reserved_1[2]; // 0x018 - 0x01c

    volatile uint32_t guest_features;     // 0x020 (W)
    volatile uint32_t guest_features_sel; // 0x024 (W)
    volatile uint32_t guest_page_size;    // 0x028 (W) Legacy only

    volatile uint32_t reserved_2; // 0x02c

    volatile uint32_t queue_sel;     // 0x030 (W)
    volatile uint32_t queue_num_max; // 0x034 (R)
    volatile uint32_t queue_num;     // 0x038 (W)
    volatile uint32_t queue_align;   // 0x03c (W) Legacy only
    volatile uint32_t queue_pfn;     // 0x040 (W) Legacy only

    volatile uint32_t reserved_3[3]; // 0x044 - 0x04c

    volatile uint32_t queue_notify; // 0x050 (W)

    volatile uint32_t reserved_4[3]; // 0x054 - 0x05c

    volatile uint32_t interrupt_status; // 0x060 (R)
    volatile uint32_t interrupt_ack;    // 0x064 (W)

    volatile uint32_t reserved_5[2]; // 0x068 - 0x06c

    volatile uint32_t status; // 0x070 (R/W)

    volatile uint32_t reserved_6[35]; // 0x074 - 0x0fc
};

} // namespace QUARK::virtio
