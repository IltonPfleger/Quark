#include <abi/Console.hpp>
#include <abi/Thread.hpp>

using namespace QUARK::ABI;

void *teste(void *) {
  Console::println("Hello World!");
  return nullptr;
}

int main(int, char *[]) {
  Console::println("Hello World!");
  Thread oi(teste, nullptr);
  return 0;
}
