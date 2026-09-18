#include <Process.hpp>
#include <Thread.hpp>
#include <Traits.hpp>
#include <machine/Machine.hpp>
#include <memory/Heap.hpp>
#include <memory/Memory.hpp>
#include <utility/Debug.hpp>

namespace QUARK {

Thread *Thread::running() { return s_scheduler.current(); }

void Thread::entry(Function f, Argument a) {
  Thread *current = running();

  if (s_previous[CPU::id()])
    epilogue();

  if constexpr (Traits<Kernel>::Mode == Traits<Kernel>::KERNEL) {
    if (current->flags_ != KERNEL) {
      assert(current->process_);

      current->stack_ = Memory::alloc(Traits<Thread>::UserStackSize);

      const Chunk kstack = {current->kstack_, Traits<Thread>::KernelStackSize};
      const Chunk stack = current->process_->attach(
          {Memory::virt2phys(reinterpret_cast<uintptr_t>(current->stack_)),
           Traits<Thread>::UserStackSize});
      Context::demote(stack, kstack, f, a);
      return;
    }
  }

  CPU::IRQ::enable();
  f(a);
  exit();
}

Thread::Return Thread::idle(Argument) {
  while (s_count > Traits<CPU>::Active) {
    reschedule();
  }

  CPU::IRQ::disable();

  CPU::barrier();

  if (CPU::id() == Traits<CPU>::BSP)
    Trace("\n*** Shutdown! ***\n");

  CPU::barrier();

  Machine::shutdown();

  return 0;
}

void Thread::dispatch(Thread *previous, Thread *next, Spin *lock) {
  assert(previous);
  assert(next);
  assert(next != previous);

  s_previous[CPU::id()] = previous;
  s_spin[CPU::id()] = lock;

  CPU::mb();

  next->state_ = State::RUNNING;

  if constexpr (Traits<Kernel>::Mode == Traits<Kernel>::KERNEL) {
    if (next->process_) {
      next->process_->activate();
    }
  }

  Context::swtch(previous->context_, next->context_);

  epilogue();
}

void Thread::epilogue() {
  Thread *previous = s_previous[CPU::id()];
  Spin *lock = s_spin[CPU::id()];

  switch (previous->state_) {
  case State::READY:
    s_scheduler.insert(&previous->node_);
    break;
  case State::WAITING:
    assert(lock);
    lock->release();
    break;
  case State::FINISHING:
    CPU::Atomic::fdec(s_count);
    previous->state_ = State::FINISHED;
    break;
  default:
    break;
  }
}

Thread::Thread(Function e, Argument a, Criterion c, Flags f, Process *p)
    : stack_(nullptr), kstack_(Memory::alloc(Traits<Thread>::KernelStackSize)),
      node_(Node(this, c)), state_(State::READY), flags_(f),
      context_({kstack_, Traits<Thread>::KernelStackSize}, entry, e, a),
      process_(p) {
  TraceIn(this);

  {
    CPU::IRQ::Guard _;
    CPU::Atomic::finc(s_count);
    s_scheduler.insert(&node_);
  }

  TraceOut();
}

Thread::~Thread() {
  TraceIn();

  join();

  if (stack_) {
    Memory::free(stack_, Traits<Thread>::UserStackSize);
  }

  if (kstack_) {
    Memory::free(kstack_, Traits<Thread>::KernelStackSize);
  }

  TraceOut();
}

void Thread::join() {
  assert(running() != this);

  while (state_ != State::FINISHED) {
    yield();
  }
}

void Thread::exit() {
  CPU::IRQ::disable();

  Thread *previous = running();

  Node *next = s_scheduler.remove();
  previous->state_ = State::FINISHING;

  dispatch(previous, next->value);
}

void Thread::init() {
  TraceIn();

  new (&s_scheduler) Scheduler();

  for (int i = 0; i < Traits<CPU>::Active; ++i)
    new Thread(idle, 0, Criterion::IDLE, KERNEL);

  TraceOut();
}

void Thread::run() {
  Thread *next = s_scheduler.remove()->value;
  Context::load(next->context_);
}

void Thread::yield() { Thread::reschedule(); }

void Thread::reschedule() {
  CPU::IRQ::Guard _;

  Thread *previous = running();

  Node *next = s_scheduler.remove(Criterion::NORMAL);

  if (next) {
    previous->state_ = State::READY;
    dispatch(previous, next->value);
  }
}

void Thread::sleep(List *list, Spin *lock) {
  Thread *previous = running();
  list->insert(&previous->node_);

  {
    CPU::IRQ::Guard _;
    previous->state_ = State::WAITING;
    Node *next = s_scheduler.remove();
    dispatch(previous, next->value, lock);
  }
}

void Thread::wakeup(List *list) {
  Node *node = list->remove();
  assert(node);
  node->value->state_ = State::READY;

  {
    CPU::IRQ::Guard _;
    s_scheduler.insert(node);
  }
}

} // namespace QUARK
