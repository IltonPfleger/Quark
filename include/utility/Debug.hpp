#pragma once

#include <utility/Console.hpp>

extern "C" [[noreturn]] inline void failure(const char *cause, const char *file,
                                            int line) {
  using namespace QUARK;
  Console::panic();
  Console::println("\n[Assertion Failed]");
  Console::println("    ", file, ":", line);
  Console::println("    ", cause);
  for (;;)
    ;
}

#define assert(expression, ...)                                                \
  if constexpr (QUARK::Traits<QUARK::Debug>::Error) {                          \
    if (!(expression)) [[unlikely]] {                                          \
      failure(#expression, __FILE__, __LINE__);                                \
    }                                                                          \
  }

// ********** Traces **********

#define Trace(...)                                                             \
  if constexpr (QUARK::Traits<QUARK::Debug>::Trace) {                          \
    __VA_OPT__(QUARK::Console::print(__VA_ARGS__);)                            \
  }

#define TraceIn(...)                                                           \
  if constexpr (QUARK::Traits<QUARK::Debug>::Trace) {                          \
    Trace(__PRETTY_FUNCTION__);                                                \
    QUARK::Console::print('(');                                                \
    __VA_OPT__(int n = 0; [&](auto &&...args) {                                \
      ((QUARK::Console::print(n++ ? ',' : '\0', args)), ...);                  \
    }(__VA_ARGS__);)                                                           \
    Console::println(") {");                                                   \
  }

#define TraceOut(...)                                                          \
  if constexpr (QUARK::Traits<QUARK::Debug>::Trace) {                          \
    __VA_OPT__(QUARK::Console::print("return="); int n = 0;                    \
               [&](auto &&...args) {                                           \
                 ((QUARK::Console::print(n++ ? ',' : '\0', args)), ...);       \
               }(__VA_ARGS__);                                                 \
               QUARK::Console::print('\n');)                                   \
    QUARK::Console::print(__func__);                                           \
    QUARK::Console::println('}');                                              \
  }
