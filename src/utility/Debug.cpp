#include <utility/Debug.hpp>

extern "C" void failure(bool condition, const char *cause, const char *file,
                        int line) {
  using namespace QUARK;
  if (condition) [[unlikely]] {
    Console::panic();
    Console::println("\n[Assertion Failed]");
    Console::println("    ", file, ":", line);
    Console::println("    ", cause);
    for (;;)
      ;
  }
}
