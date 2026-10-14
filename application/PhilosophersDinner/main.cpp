#include <Traits.hpp>
#include <abi/Console.hpp>
#include <abi/Semaphore.hpp>
#include <abi/Thread.hpp>

using namespace QUARK::ABI;

template <size_t Iterations> class Philosopher {
public:
  Philosopher(int id, Semaphore &lock, Semaphore &left, Semaphore &right)
      : id_(id), lock_(lock), left_(left), right_(right),
        thread_(dispatch, this) {}

private:
  static void *dispatch(void *pointer) {
    reinterpret_cast<Philosopher *>(pointer)->worker();
    return nullptr;
  }

  void worker() {
    int iterations = Iterations;
    while (iterations--) {
      lock_.p();
      Console::println("Filósofo ", id_, " está pensando! <", iterations, ">");
      lock_.v();

      if (id_ == 0) {
        right_.p();
        left_.p();
      } else {
        left_.p();
        right_.p();
      }

      lock_.p();
      Console::println("Filósofo ", id_, " está comendo! <", iterations, ">");
      lock_.v();

      left_.v();
      right_.v();
    }
  }

private:
  int id_;
  Semaphore &lock_;
  Semaphore &left_;
  Semaphore &right_;
  Thread thread_;
};

template <size_t Quantity, size_t Iterations> class Table {
  using Philosopher = ::Philosopher<Iterations>;

public:
  Table() : philosophers_(create()) {
    for (int i = 0; i < Quantity; i++) {
      forks_[i].v();
    }
    lock_.v();
  }

  template <QUARK::size_t... Is>
  QUARK::Meta::Array<Quantity, Philosopher>
  create(QUARK::Meta::IndexSequence<Is...>) {
    return {Philosopher(Is, lock_, forks_[Is], forks_[(Is + 1) % Quantity])...};
  }

  QUARK::Meta::Array<Quantity, Philosopher> create() {
    using namespace QUARK::Meta;
    return create(typename MakeIndexSequence<Quantity>::Result{});
  }

private:
  Semaphore lock_;
  Semaphore forks_[Quantity];
  QUARK::Meta::Array<Quantity, Philosopher> philosophers_;
};

int main(int, char *[]) {
  Console::println("Quantity Dinner:");
  Table<100, 100> PhilosopherDinner;
  Console::println("\nFinished!");
}
