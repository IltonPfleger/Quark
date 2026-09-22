#include <Alarm.hpp>
#include <architecture/Timer.hpp>
#include <utility/Console.hpp>

namespace QUARK {

Alarm::operator bool() { return Timer::now() >= criterion; }

Alarm::Alarm(Microsecond at) : Alarm(at, internal_) {}

Alarm::Alarm(Microsecond at, Semaphore &handler)
    : Node(at), internal_(0), handler_(handler) {
  {
    CPU::IRQ::Guard _;
    core_ = CPU::id();
    alarms_[core_].insert(this);
  }

  handler_.p();
}

Alarm::~Alarm() {
  CPU::IRQ::Guard _;
  alarms_[core_].remove(this);
}

void Alarm::handler() {
  size_t core = CPU::id();

  Alarms &alarms = alarms_[core];

  while (true) {
    Node *removed = alarms.remove();

    if (!removed)
      break;

    Alarm *head = static_cast<Alarm *>(removed);

    if (!*head) {
      alarms.insert(head);
      break;
    }

    head->handler_.v();
  }
}

} // namespace QUARK
