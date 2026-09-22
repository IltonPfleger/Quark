#pragma once

#ifndef assert

#ifdef NDEBUG

#define assert(expr) ((void)0)

#else

#define assert(expr)

#endif

#endif
