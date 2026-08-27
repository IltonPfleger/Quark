#pragma once

#include <libraries/libc/string.h>
#include <network/NetworkAddress.hpp>
#include <types.hpp>

namespace QUARK {

template <size_t Length> struct GenericAddress {

  constexpr GenericAddress() = default;

  template <typename... Args>
  constexpr GenericAddress(Args... args)
      : _data{static_cast<uint8_t>(args)...} {}

  constexpr GenericAddress(const uint8_t *data) {
    for (size_t i = 0; i < Length; ++i)
      _data[i] = data[i];
  }

  constexpr GenericAddress(const NetworkAddress &address) {
    for (size_t i = 0; i < Length && i < address.length(); ++i)
      _data[i] = address[i];
  }

  [[nodiscard]] operator NetworkAddress() {
    return NetworkAddress(_data, Length);
  }

  [[nodiscard]] operator const NetworkAddress() const {
    return NetworkAddress(_data, Length);
  }

  [[nodiscard]] uint8_t &operator[](size_t i) { return _data[i]; }

  [[nodiscard]] const uint8_t &operator[](size_t i) const { return _data[i]; }

  [[nodiscard]] operator uint8_t *() { return _data; }

  [[nodiscard]] operator const uint8_t *() const { return _data; }

  [[nodiscard]] size_t length() const { return Length; }

  [[nodiscard]] bool operator==(const GenericAddress &) const = default;

  static constexpr GenericAddress broadcast() {
    GenericAddress result{};

    for (size_t i = 0; i < Length; ++i)
      result._data[i] = 0xFF;

    return result;
  }

private:
  uint8_t _data[Length]{};

} __attribute__((packed));

} // namespace QUARK
