// #include <network/NetworkDevice.hpp>
//
// namespace QUARK {
//
// void *NetworkDevice::worker(void *argument) {
//     auto *self = static_cast<NetworkDevice *>(argument);
//
//     if (!self->running_) break;
//
//     NetworkBuffer *buffer = nullptr;
//
//     while ((buffer = self->receive()) != nullptr) {
//         self->notify(buffer);
//         self->release(buffer);
//     }
//     return nullptr;
// }
//
// } // namespace QUARK
