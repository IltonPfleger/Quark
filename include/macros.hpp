#ifndef __QUARK_MACROS__
#define __QUARK_MACROS__

#define __CONCATENATE(X, Y) X##Y
#define ___STRINGIFY(X) #X
#define __STRINGIFY(X) ___STRINGIFY(X)

/* Headers */
#define __HEADER_NAME(X) X.hpp
#define __HEADER_PAYLOAD(X) <payload/__PAYLOAD/__HEADER_NAME(X)>

/* Payload */
#ifdef __PAYLOAD
#define __PAYLOAD_TRAITS_HEADER __HEADER_PAYLOAD(Traits)
#endif

#endif
