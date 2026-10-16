#ifndef QUARK_UTILITY_DEBUG_HPP
#define QUARK_UTILITY_DEBUG_HPP

#include <Meta.hpp>
#include <architecture/Timer.hpp>
#include <utility/Console.hpp>

extern "C" void failure(bool, const char *, const char *, int);

namespace QUARK {

namespace Logging {

struct Location {

  static constexpr Location
  current(const char *file = __builtin_FILE(),
          const char *function = __builtin_FUNCTION()) {
    Location l;
    l.function_ = function;
    l.file_ = file;
    return l;
  }

  constexpr const char *function() const noexcept { return function_; }
  constexpr const char *file() const noexcept { return file_; }

private:
  const char *function_;
  const char *file_;
};

template <typename T> struct Tracker {
  T tracked;
  Location location;
  constexpr Tracker(T tracked, Location location = Location::current())
      : tracked(tracked), location(location) {}
};

template <> struct Tracker<void> {
  Location location;
  constexpr Tracker(Location location = Location::current())
      : location(location) {}
};

template <typename T> class Trace {
public:
  template <typename... Us>
  Trace(const Tracker<T> &&tracker, Us &&...us)
    requires(Meta::IsVoid<T>::Result)
      : location_(tracker.location) {
    now();
    location();
    arguments(static_cast<Us &&>(us)...);
    Console::print('\n');
  }

  template <typename... Us>
  Trace(const Tracker<T *> &&tracker, Us &&...us)
    requires(!Meta::IsVoid<T>::Result)
      : pointer_(tracker.tracked), location_(tracker.location) {
    now();
    location();
    arguments(static_cast<Us &&>(us)...);
    Console::print('\n');
  }

  ~Trace() {
    location();
    Console::print("-> return ");
    if constexpr (!Meta::IsVoid<T>::Result) {
      if (pointer_) {
        Console::print(*pointer_, " ");
      }
    }
    Console::print('\n');
  }

private:
  template <typename... Us> void arguments(Us &&...us) {
    Console::print('(');
    if constexpr (sizeof...(Us) > 0) {
      size_t i = 0;
      (Console::print(us, ++i < sizeof...(Us) ? ", " : ""), ...);
    }
    Console::print(')');
  }

  void now() { QUARK::Console::print("[", Timer::now(), "] "); }
  void location() {
    QUARK::Console::print("[", location_.file(), "::", location_.function(),
                          "] ");
  }

private:
  T *pointer_ = nullptr;
  Location location_;
};

Trace(Tracker<void>) -> Trace<void>;
template <typename T, typename... Us> Trace(T *, Us &&...) -> Trace<T>;

} // namespace Logging

template <typename T = void> using Trace = Logging::Trace<T>;

} // namespace QUARK

#define assert(condition)                                                      \
  if constexpr (QUARK::Traits<QUARK::Debug>::Error)                            \
    failure(!(condition), #condition, __FILE__, __LINE__);

#endif
