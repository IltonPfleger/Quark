#include <architecture/Context.hpp>
#include <architecture/VirtualCPU.hpp>

namespace QUARK {

void HypervisorContext::swtch(HypervisorContext &previous,
                              HypervisorContext &next) {
  previous.cpu_ = VirtualCPU::swtch(next.cpu_);
  Father::swtch(previous, next);
}

} // namespace QUARK
