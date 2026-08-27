#include <architecture/riscv64/Context.hpp>
#include <architecture/riscv64/VirtualCPU.hpp>

void QUARK::HypervisorContext::swtch(HypervisorContext &previous,
                                     HypervisorContext &next) {
  previous.cpu_ = VirtualCPU::swtch(next.cpu_);
  Father::swtch(previous, next);
}
