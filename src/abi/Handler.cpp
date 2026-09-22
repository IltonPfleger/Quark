#include <Process.hpp>
#include <synchronization/Semaphore.hpp>
#include <Thread.hpp>
#include <abi/Handler.hpp>
#include <architecture/MMU.hpp>
#include <memory/Heap.hpp>
#include <utility/Console.hpp>

namespace QUARK::ABI {

void *Handler::handler(Operation o, const Arguments a) {
  switch (o) {
  case WRITE: {
    if (a[0] == 0) {
      uintptr_t pa = MMU::PageTable::virt2phys(a[1]);
      uintptr_t va = Memory::phys2virt(pa);
      char character = *reinterpret_cast<char *>(va);
      QUARK::Console::print(character);
    }
    break;
  }

  case EXIT: {
    QUARK::Thread::exit();
    break;
  }

  case THREAD_CONSTRUCTOR: {
    auto function = reinterpret_cast<Thread::Function>(a[0]);
    auto argument = reinterpret_cast<Thread::Argument>(a[1]);
    auto critetion = Thread::Criterion::NORMAL;
    auto flags = Thread::USER;
    auto process = Process::current();
    return new Thread(function, argument, critetion, flags, process);
  }

  case THREAD_JOIN: {
    reinterpret_cast<Thread *>(a[0])->join();
    break;
  }

  case THREAD_DESTRUCTOR: {
    delete reinterpret_cast<Thread *>(a[0]);
    break;
  }
  case SEMAPHORE_CONSTRUCTOR: {
    return new Semaphore(a[0]);
  }
  case SEMAPHORE_P: {
    reinterpret_cast<Semaphore *>(a[0])->p();
    break;
  }
  case SEMAPHORE_V: {
    reinterpret_cast<Semaphore *>(a[0])->v();
    break;
  }
  case SEMAPHORE_DESTRUCTOR: {
    delete reinterpret_cast<Semaphore *>(a[0]);
    break;
  }
  }
  //   case Function::ABI_HEAP_NEW: {
  //     return new uint8_t[a[0]];
  //     break;
  //   }
  //   case Function::ABI_HEAP_DELETE: {
  //     ::operator delete[](reinterpret_cast<void *>(a[0]), a[1]);
  //     break;
  //   }
  //   default: {
  //     assert(false, "Not Available!");
  //     break;
  //   };
  //   }
  //
  return nullptr;
}

} // namespace QUARK::ABI
