#ifndef __QUARK_PROCESS__
#define __QUARK_PROCESS__

#include <Thread.hpp>
#include <architecture/MMU.hpp>
#include <memory/Chunk.hpp>

namespace QUARK {

class Process {
public:
  Process() : pt_(MMU::PageTable::clone()) {};
  ~Process() {}

  void attach(const Chunk &va, const Chunk &pa) {
    assert(va.length() == pa.length(), va.length(), " != ", pa.length());
    size_t length = va.length();
    pt_->map(va.start(), pa.start(), length, MMU::PageTable::UserRWX);
  }

  Chunk attach(const Chunk &pa) {
    uintptr_t va = pt_->find(pa.length());
    pt_->map(va, pa.start(), pa.length(), MMU::PageTable::UserRWX);
    return Chunk(va, pa.length());
  }

  void activate() { pt_->activate(); }

private:
  MMU::PageTable *pt_;
};

} // namespace QUARK

#endif
