#include <libraries/libc/string.h>

using namespace QUARK;

extern "C" {

void *memcpy(void *dd, const void *ss, size_t n) {
    uint8_t *d       = static_cast<uint8_t *>(dd);
    const uint8_t *s = static_cast<const uint8_t *>(ss);

    while (n--) {
        *d++ = *s++;
    }

    return dd;
}

void *memset(void *p, int c, size_t n) {
    uint8_t *d = static_cast<uint8_t *>(p);

    while (n--)
        *d++ = static_cast<uint8_t>(c);

    return p;
}

int memcmp(const void *ptr1, const void *ptr2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)ptr1;
    const uint8_t *p2 = (const uint8_t *)ptr2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return (int)(p1[i] - p2[i]);
        }
    }

    return 0;
}

unsigned strlen(const char *str) {
    size_t n = 0;
    while (*str++)
        n++;
    return n;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (unsigned char)(*s1) - (unsigned char)(*s2);
}

}
