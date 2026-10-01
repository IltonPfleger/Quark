#ifndef __QUARK_APPLICATION_LINUX__
#define __QUARK_APPLICATION_LINUX__

__attribute__((section(".__linux__"),
               used)) static unsigned char __guest[32 * 1024 * 1024];

__attribute__((section(".__initrd__"),
               used)) static unsigned char __initrd[16 * 1024 * 1024];

#endif
