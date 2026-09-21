#include <libraries/libc/stdlib.h>
#include <memory/Heap.hpp>

using namespace QUARK;

// extern "C" void *malloc(size_t size) { return new uint8_t[size]; }

extern "C" long atol(const char *str) {
  long result = 0;
  bool negative = false;

  if (*str == '-') {
    negative = true;
    ++str;
  }

  while (*str >= '0' && *str <= '9') {
    result = result * 10 + (*str - '0');
    ++str;
  }

  return negative ? -result : result;
}

extern "C" int atexit(void (*)(void)) { return 0; }

extern "C" [[noreturn]]
void __cxa_pure_virtual() {
  assert(false);
  while (1)
    ;
}
