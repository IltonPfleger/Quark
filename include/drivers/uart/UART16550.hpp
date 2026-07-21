#pragma once

#include <architecture/IC.hpp>
#include <utility/Atomic.hpp>
#include <utility/Debug.hpp>
#include <utility/Observer.hpp>
#include <utility/WorkerManager.hpp>

namespace QUARK {

template <typename Tag> class UART16550 : public Observed<const char *, size_t> {
    using Traits = QUARK::Traits<Tag>;

    static constexpr unsigned int Clock       = Traits::Clock;
    static constexpr unsigned int BaudRate    = Traits::BaudRate;
    static constexpr unsigned int BaudDivisor = Clock / (16 * BaudRate);

  private:
    UART16550() {
        Address[IER] = 0x00;
        Address[IER] = 0x00;
        Address[LCR] = LCR_DLAB;
        Address[DLL] = static_cast<uint8_t>(BaudDivisor & 0xFF);
        Address[DLM] = static_cast<uint8_t>((BaudDivisor >> 8) & 0xFF);
        Address[LCR] = LCR_8N1;
        Address[FCR] = FCR_ENABLE | FCR_CLEAR;
        Address[IER] = IER_RX;
        Address[MCR] = 0x0B;
    }

    enum Registers {
        RBR = 0 << Traits::Shift, // Receiver Buffer
        THR = 0 << Traits::Shift, // Transmitter Holding
        DLL = 0 << Traits::Shift, // Divisor Latch Low
        IER = 1 << Traits::Shift, // Interrupt Enable
        DLM = 1 << Traits::Shift, // Divisor Latch High
        IIR = 2 << Traits::Shift, // Interrupt Identity
        FCR = 2 << Traits::Shift, // FIFO Control
        LCR = 3 << Traits::Shift, // Line Control
        MCR = 4 << Traits::Shift, // Modem Control
        LSR = 5 << Traits::Shift, // Line Status
        MSR = 6 << Traits::Shift, // Modem Status
        SCR = 7 << Traits::Shift  // Scratch
    };

    enum Bits {
        LCR_DLAB     = 1 << 7,
        LCR_8N1      = 0x03,
        FCR_ENABLE   = 0x01,
        FCR_CLEAR    = 0x06,
        IER_RX       = 0x01,
        LSR_RX_READY = 1 << 0,
        LSR_TX_EMPTY = 1 << 5,
    };

    static void isr(size_t) {
        auto *self = reinterpret_cast<UART16550 *>(instance());
        if (!self->pending_.tsl()) WorkerManager ::schedule(worker, instance());
    }

    static void worker(void *pointer) {
        auto *self = reinterpret_cast<UART16550 *>(pointer);
        char buffer[128];
        size_t i = 0;
        while (Address[LSR] & LSR_RX_READY && i < sizeof(buffer)) {
            char c    = Address[RBR];
            buffer[i] = c;
            i++;
        }
        self->notify(buffer, i);
        self->pending_.store(false);
    }

  public:
    static void init() {
        for (auto &i : Traits::IRQs)
            IC::install(i, isr);
    }

    static UART16550 *instance() {
        static UART16550 instance;
        return &instance;
    }

    void putc(char c) {
        while ((Address[LSR] & LSR_TX_EMPTY) == 0)
            ;
        Address[THR] = c;
    }

    char getc() {
        while ((Address[LSR] & LSR_RX_READY) == 0)
            ;
        return Address[RBR];
    }

  private:
    static inline volatile uint8_t *Address = reinterpret_cast<uint8_t *>(Traits::Address);

  private:
    Atomic<bool> pending_;
};

} // namespace QUARK
