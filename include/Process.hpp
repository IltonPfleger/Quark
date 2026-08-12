#ifndef __QUARK_PROCESS__
#define __QUARK_PROCESS__

namespace QUARK {

class Process {
  public:
    Process(const Chunk &&rw, const Chunk &&rx) {
        (void)rw;
        (void)rx;
        // pt_.map(rw.data(), 0, rw.length(), MMU::PageTable::UserRW);
        // pt_.map(rx.data(), 0, rx.length(), MMU::PageTable::UserRX);
    }

  private:
    MMU::PageTable pt_;
};

} // namespace QUARK

#endif
