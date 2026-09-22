#ifndef __QUARK_MACROS__
#define __QUARK_MACROS__

#define __CONCATENATE(X, Y) X##Y
#define ___STRINGIFY(X) #X
#define __STRINGIFY(X) ___STRINGIFY(X)

/* Headers */
#define __HEADER_NAME(X) X.hpp
#define __HEADER_APPLICATION(X) <application/__APPLICATION/__HEADER_NAME(X)>

/* Application */
#ifdef __APPLICATION
#define __APPLICATION_TRAITS_HEADER __HEADER_APPLICATION(Traits)
#endif

#endif
