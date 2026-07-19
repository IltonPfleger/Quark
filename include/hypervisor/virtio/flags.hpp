#pragma once

namespace QUARK::virtio {

enum {
    VRING_DESC_F_NEXT     = 0x1,
    VRING_DESC_F_WRITE    = 0x2,
    VRING_DESC_F_INDIRECT = 0x4,

    VRING_AVAIL_F_NO_INTERRUPT = 0x1,
    VRING_USED_F_NO_NOTIFY     = 0x1,
};

}
