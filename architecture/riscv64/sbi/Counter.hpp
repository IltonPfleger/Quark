#pragma once

#include <Alarm.hpp>
#include <Semaphore.hpp>
#include <architecture/riscv64/CLINT.hpp>
#include <architecture/riscv64/Context.hpp>
#include <architecture/riscv64/VirtualCPU.hpp>

namespace QUARK::sbi {

class Counter {
    using PageTable = MMU::PageTable;

  public:
    static constexpr unsigned int EID = 0;

    static void handler(ContextFrame *c) {
        switch (c->a6) {
            case 0: {
                c->a0 = CPU::Atomic::finc(counter_);
                break;
            }
            case 1: {
                int request = CPU::Atomic::fdec(end_);
                if (request == 1) {
                    lock_.v();
                }
                lock_.p();
                Console::println("DEVICE: ", c->a2);
                uintptr_t address = PageTable::virt2phys(c->a1);
                uintptr_t *ts     = reinterpret_cast<uintptr_t *>(address);
                for (uintmax_t i = 1; i < c->a0; i += 2) {
                    Console::println(ts[i] - ts[i - 1]);
                }
                lock_.v();
                while (1) {
                    Thread::yield();
                }
                break;
            }
            case 2: {
                Alarm(c->a0);
                break;
            }
        }
    }

  public:
    static constinit volatile inline size_t counter_ = 0;
    static constinit volatile inline size_t end_     = 5;
    static constinit inline Semaphore lock_          = 0;
};

} // namespace QUARK::sbi
