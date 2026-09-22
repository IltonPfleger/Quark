#include <abi/Thread.hpp>

extern int main(int, char *[]);

extern "C" void _start() {
  main(0, nullptr);
  QUARK::ABI::Thread::exit();
  __builtin_unreachable();
}
