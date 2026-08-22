#ifndef __QUARK_VIRTUAL_INTERRUPT_CONTROLLER__
#define __QUARK_VIRTUAL_INTERRUPT_CONTROLLER__

namespace QUARK {

class VirtualInterruptController {};

template <typename T>
concept VirtualInterruptControllerConcept = requires(T controller) {
  { controller.interrupt(0) } -> Meta::SameAs<void>;
  { controller.read(0, nullptr, 0) } -> Meta::SameAs<bool>;
  { controller.write(0, nullptr, 0) } -> Meta::SameAs<bool>;
};

template <typename T> struct IsInterruptController {
  static constexpr bool Result =
      Meta::IsBaseOf<VirtualInterruptController, T>::Result &&
      VirtualInterruptControllerConcept<T>;
};

} // namespace QUARK

#endif
