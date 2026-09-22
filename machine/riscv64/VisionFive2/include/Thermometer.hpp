#pragma once

#include <drivers/thermometer/JH7110_Thermometer.hpp>

namespace QUARK {

class Thermometer : public JH7110_Thermometer<void> {};

} // namespace QUARK
