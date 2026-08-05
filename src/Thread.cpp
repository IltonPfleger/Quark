#include <Thread.hpp>
#include <Traits.hpp>
#include <abi/Thread.hpp>
#include <machine/Machine.hpp>
#include <memory/Heap.hpp>
#include <memory/Memory.hpp>
#include <utility/Debug.hpp>

namespace QUARK {

Thread *Thread::running() { return s_scheduler.current(); }

void Thread::entry(Function f, Argument a) {
    Thread *current = running();

    if (s_previous[CPU::id()]) epilogue();

    if constexpr (Traits<Payload>::Unprivileged) {
        if (current->domain_ == Domain::USER) {
            Context::demote(current->kstack_, current->stack_, f, ABI::Thread::exit, a);
            return;
        }
    }

    CPU::IRQ::enable();
    f(a);
    exit();
}

Thread::Return Thread::idle(Argument) {
    while (s_count > Traits<CPU>::Active + Traits<Deferred>::Threads) {
        reschedule();
    }

    CPU::IRQ::disable();

    CPU::barrier();

    if (CPU::id() == Traits<CPU>::BSP) Trace("\n*** Shutdown! ***\n");

    CPU::barrier();

    Machine::shutdown();

    return 0;
}

void Thread::dispatch(Thread *previous, Thread *next, Spin *lock) {
    assert(previous);
    assert(next);
    assert(next != previous);

    s_previous[CPU::id()] = previous;
    s_spin[CPU::id()]     = lock;

    CPU::mb();

    next->state_ = State::RUNNING;

    Context::swtch(previous->context_, next->context_);

    epilogue();
}

void Thread::epilogue() {
    Thread *previous = s_previous[CPU::id()];
    Spin *lock       = s_spin[CPU::id()];

    switch (previous->state_) {
        case State::READY: s_scheduler.insert(&previous->node_); break;
        case State::WAITING:
            assert(lock);
            lock->release();
            break;
        case State::FINISHING:
            CPU::Atomic::fdec(s_count);
            previous->state_ = State::FINISHED;
            break;
        default: break;
    }
}

Thread::Thread(Function f, Argument a, Criterion c, Domain d)
    : stack_(Memory::alloc(d == Domain::USER ? Traits<Thread>::UserStackSize : 0), d == Domain::USER ? Traits<Thread>::UserStackSize : 0),
      kstack_(Memory::alloc(Traits<Thread>::KernelStackSize), Traits<Thread>::KernelStackSize),
      node_(Node(this, c)),
      state_(State::READY),
      context_(kstack_, stack_, entry, f, a),
      domain_(d) {
    TraceIn(this);

    {
        CPU::IRQ::Guard irqg;
        CPU::Atomic::finc(s_count);
        s_scheduler.insert(&node_);
    }

    TraceOut();
}

Thread::~Thread() {
    join();
    Memory::free(stack_.data(), stack_.length());
    Memory::free(kstack_.data(), kstack_.length());
}

void Thread::join() {
    while (state_ != State::FINISHED) {
        reschedule();
    }
}

void Thread::exit() {
    CPU::IRQ::disable();

    Thread *previous = running();

    Node *next       = s_scheduler.remove();
    previous->state_ = State::FINISHING;

    dispatch(previous, next->value);
}

void Thread::init() {
    TraceIn();

    new (&s_scheduler) Scheduler();

    for (int i = 0; i < Traits<CPU>::Active; ++i)
        new Thread(idle, 0, Criterion::IDLE, Domain::KERNEL);

    TraceOut();
}

void Thread::run() {
    Thread *next = s_scheduler.remove()->value;
    Context::load(next->context_);
}

void Thread::yield() { Thread::reschedule(); }

void Thread::reschedule() {
    CPU::IRQ::Guard irqg;

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
        CPU::IRQ::Guard irqg;
        previous->state_ = State::WAITING;
        Node *next       = s_scheduler.remove();
        dispatch(previous, next->value, lock);
    }
}

void Thread::wakeup(List *list) {

    Node *node = list->remove();
    assert(node);
    node->value->state_ = State::READY;

    {
        CPU::IRQ::Guard irqg;
        s_scheduler.insert(node);
    }
}

} // namespace QUARK
