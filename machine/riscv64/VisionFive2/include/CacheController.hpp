#pragma once

#include <drivers/cache/SiFiveU74L2CacheController.hpp>

namespace QUARK {

class CacheController : public SiFiveU74L2CacheController<void> {};

} // namespace QUARK
