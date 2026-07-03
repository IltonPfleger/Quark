#ifndef __QUARK_SCHEDULER_FIXED_CORE__
#define __QUARK_SCHEDULER_FIXED_CORE__

#include <Spin.hpp>
#include <types.hpp>
#include <utility/Console.hpp>
#include <utility/collections/FIFO.hpp>

namespace QUARK {

class FixedCore {
  public:
    enum : uint8_t { IDLE = 0, NORMAL = 1 };
    enum : uint32_t { ANY = ~0U };

    static constexpr uint32_t NumberOfCores = Traits<CPU>::Active;

    FixedCore(uint8_t rank = NORMAL, uint32_t cpu = ANY, ...) {
        build(rank, cpu);
        TraceIn(this->rank(), this->cpu());
        TraceOut();
    }

    FixedCore(const FixedCore &)            = default;
    FixedCore &operator=(const FixedCore &) = default;

    template <typename Node> struct Collection {
        void insert(const FixedCore &fc, Node *node) { queues_[fc.cpu()][fc.rank()].insert(node); }

        Node *remove(uint8_t threshold) {
            Node *node = queues_[CPU::id()][NORMAL].remove();
            if (!node && threshold == IDLE) node = queues_[CPU::id()][IDLE].remove();
            return node;
        }

      private:
        collections::FIFO<Node, Spin> queues_[NumberOfCores][2];
    };

    operator uint8_t() { return rank(); }

  private:
    uint32_t cpu() const { return cpu_; }
    uint8_t rank() const { return rank_; }

    void build(uint8_t rank, uint32_t cpu) {
        if (rank == IDLE && cpu == ANY) {
            cpu = CPU::Atomic::finc(idles_) % NumberOfCores;
        } else if (cpu == ANY) {
            cpu = Traits<CPU>::BSP;
        }

        assert(rank < 2, rank);
        assert(cpu < NumberOfCores, cpu);

        cpu_  = cpu;
        rank_ = rank;
    }

  private:
    static constinit inline uint32_t idles_ = 0;

  private:
    uint32_t cpu_;
    uint8_t rank_;
};

} // namespace QUARK

#endif
