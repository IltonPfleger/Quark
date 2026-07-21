#pragma once

#include <architecture/IC.hpp>
#include <drivers/Driver.hpp>
#include <utility/Atomic.hpp>
#include <utility/Debug.hpp>
#include <utility/Observer.hpp>
#include <utility/WorkerManager.hpp>

namespace QUARK {

template <typename Tag> class UART16550 : public Driver, public Observed<const char *, size_t> {
    using MyTraits = Traits<Tag>;

    static constexpr unsigned long Address    = MyTraits::Address;
    static constexpr unsigned int Clock       = MyTraits::Clock;
    static constexpr unsigned int BaudRate    = MyTraits::BaudRate;
    static constexpr unsigned int BaudDivisor = Clock / (16 * BaudRate);

  private:
    UART16550() {
        Reg8(Address, IER) = 0x00;
        Reg8(Address, LCR) = LCR_DLAB;
        Reg8(Address, DLL) = static_cast<uint8_t>(BaudDivisor & 0xFF);
        Reg8(Address, DLM) = static_cast<uint8_t>((BaudDivisor >> 8) & 0xFF);
        Reg8(Address, LCR) = LCR_8N1;
        Reg8(Address, FCR) = FCR_ENABLE | FCR_CLEAR;
        Reg8(Address, IER) = IER_RX;
        Reg8(Address, MCR) = 0x0B;
    }

    enum Registers {
        RBR = 0 << MyTraits::Shift, // Receiver Buffer
        THR = 0 << MyTraits::Shift, // Transmitter Holding
        DLL = 0 << MyTraits::Shift, // Divisor Latch Low
        IER = 1 << MyTraits::Shift, // Interrupt Enable
        DLM = 1 << MyTraits::Shift, // Divisor Latch High
        IIR = 2 << MyTraits::Shift, // Interrupt Identity
        FCR = 2 << MyTraits::Shift, // FIFO Control
        LCR = 3 << MyTraits::Shift, // Line Control
        MCR = 4 << MyTraits::Shift, // Modem Control
        LSR = 5 << MyTraits::Shift, // Line Status
        MSR = 6 << MyTraits::Shift, // Modem Status
        SCR = 7 << MyTraits::Shift  // Scratch
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
        while (Reg8(Address, LSR) & LSR_RX_READY && i < sizeof(buffer)) {
            char c    = Reg8(Address, RBR);
            buffer[i] = c;
            i++;
        }
        self->notify(buffer, i);
        self->pending_.store(false);
    }

  public:
    static void init() {
        for (auto &i : MyTraits::IRQs)
            IC::install(i, isr);
    }

    static UART16550 *instance() {
        static UART16550 instance;
        return &instance;
    }

    void putc(char c) {
        while ((Reg8(Address, LSR) & LSR_TX_EMPTY) == 0)
            ;
        Reg8(Address, THR) = c;
    }

    char getc() {
        while ((Reg8(Address, LSR) & LSR_RX_READY) == 0)
            ;
        return Reg8(Address, RBR);
    }

  private:
    static constexpr uint8_t *Address = static_cast<uint8_t *>(MyTraits::Address);

  private:
    Atomic<bool> pending_;
};

} // namespace QUARK
