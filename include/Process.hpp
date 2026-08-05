#ifndef __QUARK_PROCESS__
#define __QUARK_PROCESS__

namespace QUARK {

class Process {
  public:
    Process() pt_() {}

  private:
    MMU::PageTable pt_;
};

} // namespace QUARK

#endif
