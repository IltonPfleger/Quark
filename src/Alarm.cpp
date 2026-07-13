#include <Alarm.hpp>
#include <Spin.hpp>
#include <architecture/Timer.hpp>
#include <utility/Console.hpp>

namespace QUARK {

Alarm::operator bool() { return Timer::now() >= node_.criterion; }

Alarm::Alarm(Microsecond at)
    : Alarm(at, local_) {}

Alarm::Alarm(Microsecond at, Semaphore &handler)
    : node_(this, at),
      local_(0),
      handler_(handler) {

    CPU::IRQ::Guard _;

    core_ = CPU::id();

    Alarms &alarms = alarms_[core_];

    alarms.insert(&node_);

    handler_.p();
}

Alarm::~Alarm() {
    CPU::IRQ::Guard _;

    Alarms &alarms = alarms_[core_];

    alarms.remove(&this->node_);
}

void Alarm::handler() {
    size_t core = CPU::id();

    Alarms &alarms = alarms_[core];

    while (true) {
        Node *head = alarms.remove();

        if (!head) break;

        if (!*head->value) {
            alarms.insert(head);
            break;
        }

        head->value->handler_.v();
    }
}

} // namespace QUARK
