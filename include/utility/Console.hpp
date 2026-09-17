#pragma once

#include <Meta.hpp>
#include <Traits.hpp>
#include <utility/Printer.hpp>

namespace QUARK {

class Console : public Printer<Console> {
  using Device = Meta::GetFromTypeList<Traits<UART>::Devices, 0>::Result;

public:
  static void panic();
  static void write(char);

private:
  static bool panicked();

private:
  static volatile inline uintmax_t panic_ = 0;
};

} // namespace QUARK
