#pragma once

#include <utility/Console.hpp>

#define assert(expression, ...)                                                                                        \
    if constexpr (QUARK::Traits<QUARK::Debug>::Error) {                                                                \
        if (!(expression)) [[unlikely]] {                                                                              \
            QUARK::Console::panic();                                                                                   \
            QUARK::Console::println("\n[ASSERT] ", __PRETTY_FUNCTION__);                                               \
            QUARK::Console::println(#expression);                                                                      \
            __VA_OPT__(QUARK::Console::println(__VA_ARGS__);)                                                          \
            for (;;)                                                                                                   \
                ;                                                                                                      \
        }                                                                                                              \
    }

// ********** Traces **********

#define Trace(...)                                                                                                     \
    if constexpr (QUARK::Traits<QUARK::Debug>::Trace) {                                                                \
        __VA_OPT__(QUARK::Console::print(__VA_ARGS__);)                                                                \
    }

#define TraceIn(...)                                                                                                   \
    if constexpr (QUARK::Traits<QUARK::Debug>::Trace) {                                                                \
        Trace(__PRETTY_FUNCTION__);                                                                                    \
        QUARK::Console::print('(');                                                                                    \
        __VA_OPT__(int n = 0;                                                                                          \
                   [&](auto &&...args) { ((QUARK::Console::print(n++ ? ',' : '\0', args)), ...); }(__VA_ARGS__);)      \
        Console::println(')');                                                                                         \
    }

#define TraceOut(...)                                                                                                  \
    if constexpr (QUARK::Traits<QUARK::Debug>::Trace) {                                                                \
        __VA_OPT__(QUARK::Console::print("return="); int n = 0;                                                        \
                   [&](auto &&...args) { ((QUARK::Console::print(n++ ? ',' : '\0', args)), ...); }(__VA_ARGS__);       \
                   QUARK::Console::print('\n');)                                                                       \
        QUARK::Console::print(__func__);                                                                               \
        QUARK::Console::println('}');                                                                                  \
    }
