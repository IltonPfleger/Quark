#include <Thread.hpp>
#include <architecture/CPU.hpp>
#include <machine/UART.hpp>
#include <utility/Console.hpp>

namespace QUARK {

void Console::panic() { CPU::Atomic::cas(panic_, 0, CPU::id()); }

bool Console::panicked() { return (panic_ && panic_ != CPU::id()); }

void Console::print(char c) {
  if (panicked())
    return;

  Device *device = Device::instance();

  if (c == '\n') {
    device->write('\r');
  }

  device->write(c);
}

void Console::print(const char *s) {
  while (s && *s)
    print(*s++);
}

void Console::print(uintmax_t number) {
  char buffer[sizeof(number) * 8 / 3 + 1];
  int i = 0;

  if (number == 0) {
    print('0');
  } else {
    while (number != 0) {
      buffer[i++] = (number % 10) + '0';
      number /= 10;
    }
    while (i > 0)
      print(buffer[--i]);
  }
}

void Console::print(intmax_t number) {
  if (number < 0) {
    print('-');
    number = -number;
  }
  print(static_cast<uintmax_t>(number));
}

void Console::print(Hex<uintmax_t> hex) {
  const char *digits = "0123456789abcdef";
  uintmax_t x = hex;
  char buffer[64];
  int i = 0;

  print("0x");

  if (x == 0) {
    print('0');
  } else {
    while (x > 0) {
      buffer[i++] = digits[x % 16];
      x = x / 16;
    }
    while (i > 0)
      print(buffer[--i]);
  }
}

void Console::print(double x) {
  if (x != x) {
    print("NaN");
    return;
  }

  if (x < 0) {
    print('-');
    x = -x;
  }

  if (x > 1e18) {
    print("Infinite");
    return;
  }

  print(static_cast<uintmax_t>(x));
  print('.');

  double decimals = (x - static_cast<double>(static_cast<uintmax_t>(x)));

  for (int i = 0; i < 10; i++) {
    decimals *= 10;
    int digit = static_cast<int>(decimals);
    print(static_cast<char>('0' + digit));
    decimals -= digit;
  }
}

void Console::print(bool boolean) { print(boolean ? "true" : "false"); }

} // namespace QUARK
