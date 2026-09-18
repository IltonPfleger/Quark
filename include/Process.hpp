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
    assert(va.length() == pa.length());
    pt_->map(va.start(), pa.start(), va.length(), MMU::PageTable::UserRWX);
  }

  Chunk attach(const Chunk &pa) {
    const uintptr_t va = pt_->find(pa.length());
    const auto flags = MMU::PageTable::UserRWX;
    bool result = pt_->map(va, pa.start(), pa.length(), flags);
    assert(result);
    return Chunk(va, pa.length());
  }

  void activate() { pt_->activate(); }

  static Process *current() { return Thread::running()->process_; }

private:
  MMU::PageTable *pt_;
};

} // namespace QUARK

#endif
