#include <Process.hpp>
#include <Thread.hpp>
#include <Traits.hpp>
#include <architecture/IPI.hpp>
#include <machine/Machine.hpp>
#include <memory/Heap.hpp>
#include <memory/Memory.hpp>
#include <utility/Debug.hpp>

namespace QUARK {

void Thread::entry(Function f, Argument a) {
  Thread *current = running();

  if (previous_[CPU::id()])
    epilogue();

  if constexpr (Traits<Kernel>::Mode == Traits<Kernel>::KERNEL) {
    Process *process = current->process_;

    if (current->flags_ != KERNEL) {
      assert(process);

      current->stack_ = Memory::alloc(Traits<Thread>::UserStackSize);

      // TODO: Concurrency Error, in Process Attach
      Chunk kstack = {current->kstack_, Traits<Thread>::KernelStackSize};
      uintptr_t istack = reinterpret_cast<uintptr_t>(current->stack_);
      uintptr_t pstack = Memory::virt2phys(istack);
      Chunk ustack = process->attach({pstack, Traits<Thread>::UserStackSize});
      Context::demote(ustack, kstack, f, a);
      return;
    }
  }

  CPU::IRQ::enable();
  f(a);
  exit();
}

Thread::Return Thread::idle(Argument) {
  while (counter_ > Traits<CPU>::Active + Traits<Deferred>::Threads) {
    // Console::println(counter_, " ",
    //                  Traits<CPU>::Active + Traits<Deferred>::Threads);
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

void Thread::dispatch(Thread *previous, Thread *next) {
  assert(previous);
  assert(next);
  assert(next != previous);

  previous_[CPU::id()] = previous;

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
  Thread *previous = previous_[CPU::id()];

  switch (previous->state_) {
  case State::READY:
    scheduler_.insert(&previous->node_);
    break;
  case State::BLOCKED:
    assert(previous->blocking_);
    previous->blocking_->insert(&previous->node_);
    break;
  case State::FINISHING:
    CPU::Atomic::fdec(counter_);
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
    CPU::Atomic::finc(counter_);
    scheduler_.insert(&node_);
  }

  TraceOut();
}

Thread::~Thread() {
  TraceIn(this);

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

void Thread::kill() {
  flags_ = DEAD;
  join();
}

void Thread::exit() {
  Thread *previous = running();

  CPU::IRQ::disable();
  Thread *next = Thread::next();
  previous->state_ = State::FINISHING;
  dispatch(previous, next);
}

void Thread::init() {
  TraceIn();

  new (&scheduler_) Scheduler();

  for (int i = 0; i < Traits<CPU>::Active; ++i)
    new (Heap::SYSTEM) Thread(idle, 0, Criterion::IDLE, KERNEL);

  TraceOut();
}

void Thread::run() {
  Thread *next = Thread::next();
  Context::load(next->context_);
}

void Thread::yield() { Thread::reschedule(); }

void Thread::reschedule() {
  Thread *previous = running();

  if (previous->flags_ == DEAD) [[unlikely]] {
    Thread::exit();
  }

  {
    CPU::IRQ::Guard _;

    Thread *next = Thread::next(Criterion::NORMAL);

    if (!next)
      return;

    previous->state_ = State::READY;
    dispatch(previous, next);
  }
}

void Thread::sleep(List *list) {
  Thread *previous = running();
  previous->blocking_ = list;

  {
    CPU::IRQ::Guard _;
    previous->state_ = State::BLOCKED;
    dispatch(previous, Thread::next());
  }
}

bool Thread::wakeup(List *list) {
  Node *node;

  {
    CPU::IRQ::Guard _;
    node = list->remove();
  }

  if (!node)
    return false;

  assert(node->value->state_ == State::BLOCKED);
  node->value->state_ = State::READY;
  node->value->blocking_ = nullptr;

  {
    CPU::IRQ::Guard _;
    scheduler_.insert(node);
  }

  return true;
}

Thread *Thread::next(decltype(Criterion::IDLE) criterion) {
  while (Node *next = scheduler_.remove(criterion)) {
    if (next->value->flags_ == DEAD) [[unlikely]] {
      next->value->state_ = State::FINISHED;
      CPU::Atomic::fdec(counter_);
      continue;
    }
    current_[CPU::id()] = next->value;
    return next->value;
  }
  assert(criterion != Criterion::IDLE);
  return nullptr;
}

Thread *Thread::running() { return current_[CPU::id()]; }

} // namespace QUARK
