#ifndef __QUARK_MONITOR_EVENTS__
#define __QUARK_MONITOR_EVENTS__

#include <monitor/Event.hpp>

namespace QUARK {

// Hardware
inline constexpr Event CPU_CYCLES{Event::HARDWARE, 0};
inline constexpr Event INSTRUCTIONS{Event::HARDWARE, 1};
inline constexpr Event CACHE_REFERENCES{Event::HARDWARE, 2};
inline constexpr Event CACHE_MISSES{Event::HARDWARE, 3};
inline constexpr Event BRANCH_INSTRUCTIONS{Event::HARDWARE, 4};
inline constexpr Event BRANCH_MISSES{Event::HARDWARE, 5};
inline constexpr Event BUS_CYCLES{Event::HARDWARE, 6};
inline constexpr Event STALLED_CYCLES_FRONTEND{Event::HARDWARE, 7};
inline constexpr Event STALLED_CYCLES_BACKEND{Event::HARDWARE, 8};
inline constexpr Event REF_CPU_CYCLES{Event::HARDWARE, 9};

// Software
inline constexpr Event CPU_CLOCK{Event::SOFTWARE, 0};
inline constexpr Event TASK_CLOCK{Event::SOFTWARE, 1};
inline constexpr Event PAGE_FAULTS{Event::SOFTWARE, 2};
inline constexpr Event CONTEXT_SWITCHES{Event::SOFTWARE, 3};
inline constexpr Event CPU_MIGRATIONS{Event::SOFTWARE, 4};
inline constexpr Event PAGE_FAULTS_MIN{Event::SOFTWARE, 5};
inline constexpr Event PAGE_FAULTS_MAJ{Event::SOFTWARE, 6};
inline constexpr Event ALIGNMENT_FAULTS{Event::SOFTWARE, 7};
inline constexpr Event EMULATION_FAULTS{Event::SOFTWARE, 8};
inline constexpr Event DUMMY{Event::SOFTWARE, 9};
inline constexpr Event BPF_OUTPUT{Event::SOFTWARE, 10};
inline constexpr Event CGROUP_SWITCHES{Event::SOFTWARE, 11};

// L1 Data Cache (L1D)
inline constexpr Event L1D_READ_ACCESS{Event::CACHE, Event::cache(Event::CACHE_L1D, Event::CACHE_OP_READ, Event::CACHE_RESULT_ACCESS)};
inline constexpr Event L1D_READ_MISS{Event::CACHE, Event::cache(Event::CACHE_L1D, Event::CACHE_OP_READ, Event::CACHE_RESULT_MISS)};
inline constexpr Event L1D_WRITE_ACCESS{Event::CACHE, Event::cache(Event::CACHE_L1D, Event::CACHE_OP_WRITE, Event::CACHE_RESULT_ACCESS)};
inline constexpr Event L1D_WRITE_MISS{Event::CACHE, Event::cache(Event::CACHE_L1D, Event::CACHE_OP_WRITE, Event::CACHE_RESULT_MISS)};
inline constexpr Event L1D_PREFETCH_ACCESS{Event::CACHE, Event::cache(Event::CACHE_L1D, Event::CACHE_OP_PREFETCH, Event::CACHE_RESULT_ACCESS)};
inline constexpr Event L1D_PREFETCH_MISS{Event::CACHE, Event::cache(Event::CACHE_L1D, Event::CACHE_OP_PREFETCH, Event::CACHE_RESULT_MISS)};

// L1 Instruction Cache (L1I)
inline constexpr Event L1I_READ_ACCESS{Event::CACHE, Event::cache(Event::CACHE_L1I, Event::CACHE_OP_READ, Event::CACHE_RESULT_ACCESS)};
inline constexpr Event L1I_READ_MISS{Event::CACHE, Event::cache(Event::CACHE_L1I, Event::CACHE_OP_READ, Event::CACHE_RESULT_MISS)};
inline constexpr Event L1I_WRITE_ACCESS{Event::CACHE, Event::cache(Event::CACHE_L1I, Event::CACHE_OP_WRITE, Event::CACHE_RESULT_ACCESS)};
inline constexpr Event L1I_WRITE_MISS{Event::CACHE, Event::cache(Event::CACHE_L1I, Event::CACHE_OP_WRITE, Event::CACHE_RESULT_MISS)};
inline constexpr Event L1I_PREFETCH_ACCESS{Event::CACHE, Event::cache(Event::CACHE_L1I, Event::CACHE_OP_PREFETCH, Event::CACHE_RESULT_ACCESS)};
inline constexpr Event L1I_PREFETCH_MISS{Event::CACHE, Event::cache(Event::CACHE_L1I, Event::CACHE_OP_PREFETCH, Event::CACHE_RESULT_MISS)};

// Last Level Cache (LL)
inline constexpr Event LL_READ_ACCESS{Event::CACHE, Event::cache(Event::CACHE_LL, Event::CACHE_OP_READ, Event::CACHE_RESULT_ACCESS)};
inline constexpr Event LL_READ_MISS{Event::CACHE, Event::cache(Event::CACHE_LL, Event::CACHE_OP_READ, Event::CACHE_RESULT_MISS)};
inline constexpr Event LL_WRITE_ACCESS{Event::CACHE, Event::cache(Event::CACHE_LL, Event::CACHE_OP_WRITE, Event::CACHE_RESULT_ACCESS)};
inline constexpr Event LL_WRITE_MISS{Event::CACHE, Event::cache(Event::CACHE_LL, Event::CACHE_OP_WRITE, Event::CACHE_RESULT_MISS)};
inline constexpr Event LL_PREFETCH_ACCESS{Event::CACHE, Event::cache(Event::CACHE_LL, Event::CACHE_OP_PREFETCH, Event::CACHE_RESULT_ACCESS)};
inline constexpr Event LL_PREFETCH_MISS{Event::CACHE, Event::cache(Event::CACHE_LL, Event::CACHE_OP_PREFETCH, Event::CACHE_RESULT_MISS)};

// Data TLB (DTLB)
inline constexpr Event DTLB_READ_ACCESS{Event::CACHE, Event::cache(Event::CACHE_DTLB, Event::CACHE_OP_READ, Event::CACHE_RESULT_ACCESS)};
inline constexpr Event DTLB_READ_MISS{Event::CACHE, Event::cache(Event::CACHE_DTLB, Event::CACHE_OP_READ, Event::CACHE_RESULT_MISS)};
inline constexpr Event DTLB_WRITE_ACCESS{Event::CACHE, Event::cache(Event::CACHE_DTLB, Event::CACHE_OP_WRITE, Event::CACHE_RESULT_ACCESS)};
inline constexpr Event DTLB_WRITE_MISS{Event::CACHE, Event::cache(Event::CACHE_DTLB, Event::CACHE_OP_WRITE, Event::CACHE_RESULT_MISS)};
inline constexpr Event DTLB_PREFETCH_ACCESS{Event::CACHE, Event::cache(Event::CACHE_DTLB, Event::CACHE_OP_PREFETCH, Event::CACHE_RESULT_ACCESS)};
inline constexpr Event DTLB_PREFETCH_MISS{Event::CACHE, Event::cache(Event::CACHE_DTLB, Event::CACHE_OP_PREFETCH, Event::CACHE_RESULT_MISS)};

// Instruction TLB (ITLB)
inline constexpr Event ITLB_READ_ACCESS{Event::CACHE, Event::cache(Event::CACHE_ITLB, Event::CACHE_OP_READ, Event::CACHE_RESULT_ACCESS)};
inline constexpr Event ITLB_READ_MISS{Event::CACHE, Event::cache(Event::CACHE_ITLB, Event::CACHE_OP_READ, Event::CACHE_RESULT_MISS)};
inline constexpr Event ITLB_WRITE_ACCESS{Event::CACHE, Event::cache(Event::CACHE_ITLB, Event::CACHE_OP_WRITE, Event::CACHE_RESULT_ACCESS)};
inline constexpr Event ITLB_WRITE_MISS{Event::CACHE, Event::cache(Event::CACHE_ITLB, Event::CACHE_OP_WRITE, Event::CACHE_RESULT_MISS)};
inline constexpr Event ITLB_PREFETCH_ACCESS{Event::CACHE, Event::cache(Event::CACHE_ITLB, Event::CACHE_OP_PREFETCH, Event::CACHE_RESULT_ACCESS)};
inline constexpr Event ITLB_PREFETCH_MISS{Event::CACHE, Event::cache(Event::CACHE_ITLB, Event::CACHE_OP_PREFETCH, Event::CACHE_RESULT_MISS)};

// Branch Prediction Unit (BPU)
inline constexpr Event BPU_READ_ACCESS{Event::CACHE, Event::cache(Event::CACHE_BPU, Event::CACHE_OP_READ, Event::CACHE_RESULT_ACCESS)};
inline constexpr Event BPU_READ_MISS{Event::CACHE, Event::cache(Event::CACHE_BPU, Event::CACHE_OP_READ, Event::CACHE_RESULT_MISS)};
inline constexpr Event BPU_WRITE_ACCESS{Event::CACHE, Event::cache(Event::CACHE_BPU, Event::CACHE_OP_WRITE, Event::CACHE_RESULT_ACCESS)};
inline constexpr Event BPU_WRITE_MISS{Event::CACHE, Event::cache(Event::CACHE_BPU, Event::CACHE_OP_WRITE, Event::CACHE_RESULT_MISS)};
inline constexpr Event BPU_PREFETCH_ACCESS{Event::CACHE, Event::cache(Event::CACHE_BPU, Event::CACHE_OP_PREFETCH, Event::CACHE_RESULT_ACCESS)};
inline constexpr Event BPU_PREFETCH_MISS{Event::CACHE, Event::cache(Event::CACHE_BPU, Event::CACHE_OP_PREFETCH, Event::CACHE_RESULT_MISS)};

// NUMA Node Memory (NODE)
inline constexpr Event NODE_READ_ACCESS{Event::CACHE, Event::cache(Event::CACHE_NODE, Event::CACHE_OP_READ, Event::CACHE_RESULT_ACCESS)};
inline constexpr Event NODE_READ_MISS{Event::CACHE, Event::cache(Event::CACHE_NODE, Event::CACHE_OP_READ, Event::CACHE_RESULT_MISS)};
inline constexpr Event NODE_WRITE_ACCESS{Event::CACHE, Event::cache(Event::CACHE_NODE, Event::CACHE_OP_WRITE, Event::CACHE_RESULT_ACCESS)};
inline constexpr Event NODE_WRITE_MISS{Event::CACHE, Event::cache(Event::CACHE_NODE, Event::CACHE_OP_WRITE, Event::CACHE_RESULT_MISS)};
inline constexpr Event NODE_PREFETCH_ACCESS{Event::CACHE, Event::cache(Event::CACHE_NODE, Event::CACHE_OP_PREFETCH, Event::CACHE_RESULT_ACCESS)};
inline constexpr Event NODE_PREFETCH_MISS{Event::CACHE, Event::cache(Event::CACHE_NODE, Event::CACHE_OP_PREFETCH, Event::CACHE_RESULT_MISS)};

} // namespace QUARK

#endif
