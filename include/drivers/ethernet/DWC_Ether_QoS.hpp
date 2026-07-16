#pragma once

#include <Mutex.hpp>
#include <architecture/CPU.hpp>
#include <architecture/IC.hpp>
#include <drivers/Driver.hpp>
#include <drivers/ethernet/EthernetDevice.hpp>
#include <libraries/libc/string.h>
#include <machine/Machine.hpp>
#include <memory/Heap.hpp>
#include <utility/Atomic.hpp>
#include <utility/Debug.hpp>
#include <utility/collections/FIFO.hpp>

namespace QUARK {

template <uintptr_t Base> class DWC_Ether_QoS_MDIO : Driver {
    enum Register { BASE = 0x200, DATA = 0x204 };
    enum Bit {
        WRITE = 0x1 << 2,
        READ  = 0x3 << 2,
        BUSY  = 1,
    };

  public:
    static void wait() {
        while (Reg32(Base, BASE) & BUSY)
            ;
    }

    static void set(unsigned char phy, unsigned char dev, unsigned short data) { write(phy, dev, read(phy, dev) | data); }

    static void clear(unsigned char phy, unsigned char dev, unsigned short data) { write(phy, dev, read(phy, dev) & ~data); }

    static void write(unsigned char phy, unsigned char dev, unsigned short data) {
        Reg32(Base, DATA) = data;
        Reg32(Base, BASE) = BUSY | WRITE | ((phy & 0x1F) << 21) | ((dev & 0x1F) << 16);
        wait();
    }
    static unsigned short read(unsigned char phy, unsigned char dev) {
        Reg32(Base, BASE) = BUSY | READ | ((phy & 0x1F) << 21) | ((dev & 0x1F) << 16);
        wait();
        return Reg32(Base, DATA) & 0xFFFF;
    }

    static void write45(unsigned char phy, unsigned short reg, unsigned short data) {
        write(phy, 0x1E, reg);
        write(phy, 0x1F, data);
    }

    static void set45(unsigned char phy, unsigned short reg, unsigned short data) { write45(phy, reg, read45(phy, reg) | data); }

    static void clear45(unsigned char phy, unsigned short reg, unsigned short data) { write45(phy, reg, read45(phy, reg) & ~data); }

    static unsigned short read45(unsigned char phy, unsigned short reg) { return write(phy, 0x1E, reg), read(phy, 0x1F); }
};

template <uintptr_t Base> class DWC_Ether_QoS_PHY {
    using MDIO = DWC_Ether_QoS_MDIO<Base>;

    enum Register {
        BASIC_CONTROL                = 0x0,
        BASIC_STATUS                 = 0x1,
        STATUS                       = 0x11,
        MASTER_SLAVE_STATUS_REGISTER = 0x0A,
        CHIP_CONFIG                  = 0xA001,
        RGMII_CONFIG1                = 0xA003,
        PAD_DRIVE_STRENGTH_CFG       = 0xA010,
        SYNCE_CFG                    = 0xA012,
    };
    enum Bit {
        LOCAL_RECEIVER_STATUS                  = 1 << 13,
        REMOTE_RECEIVER                        = 1 << 12,
        STATUS_LINK_STATUS                     = 1 << 10,
        BASIC_CONTROL_RESET                    = 1 << 15,
        BASIC_CONTROL_AUTO_NEGOTIATION_ENABLE  = 1 << 12,
        BASIC_CONTROL_RGMII_ISOLATION          = 1 << 10,
        BASIC_CONTROL_RE_AUTO_NEGOTIATION      = 1 << 9,
        BASIC_STATUS_AUTO_NEGOTIATION_COMPLETE = 1 << 5,
        SOFTWARE_RESET                         = 1 << 15,
        SYNCE_CFG_ENABLE                       = 1 << 6,
        SYNCE_CFG_CLK_125M                     = 1 << 4,
    };

  public:
    static void init() {
        TraceIn();

        MDIO::clear45(Phy, CHIP_CONFIG, SOFTWARE_RESET);
        while (!(MDIO::read45(Phy, CHIP_CONFIG) & SOFTWARE_RESET))
            ;

        MDIO::set(Phy, BASIC_CONTROL, SOFTWARE_RESET);
        while (MDIO::read(Phy, BASIC_CONTROL) & SOFTWARE_RESET)
            ;

        Timer::Delay(Microsecond(1));

        negotiate();

        Timer::Delay(Microsecond(1));

        // rgmii_sw_dr_2 = <0x0>;
        MDIO::clear45(Phy, PAD_DRIVE_STRENGTH_CFG, 1 << 12);

        // rgmii_sw_dr = <0x3>;
        MDIO::set45(Phy, PAD_DRIVE_STRENGTH_CFG, 3 << 4);

        // rgmii_sw_dr_rxc = <0x6>;
        MDIO::clear45(Phy, PAD_DRIVE_STRENGTH_CFG, 7 << 13);
        MDIO::set45(Phy, PAD_DRIVE_STRENGTH_CFG, 6 << 13);

        // rxc_dly_en = <0>;
        MDIO::clear45(Phy, CHIP_CONFIG, 1 << 8);

        // rx_delay_sel = <0xa>;
        MDIO::clear45(Phy, RGMII_CONFIG1, 0xF << 10);
        MDIO::set45(Phy, RGMII_CONFIG1, 0xa << 10);

        // tx_delay_sel_fe = <5>;
        MDIO::clear45(Phy, RGMII_CONFIG1, 0xF << 4);
        MDIO::set45(Phy, RGMII_CONFIG1, 5 << 4);

        // tx_delay_sel = <0xa>;
        MDIO::clear45(Phy, RGMII_CONFIG1, 0xF);
        MDIO::set45(Phy, RGMII_CONFIG1, 0xa);

        // tx_inverted_1000 = <0x1>;
        MDIO::set45(Phy, RGMII_CONFIG1, 1 << 14);

        if (speed() == 1000) {
            MDIO::set45(Phy, SYNCE_CFG, SYNCE_CFG_ENABLE | SYNCE_CFG_CLK_125M);
            MDIO::clear45(Phy, SYNCE_CFG, 0x7);
        }

        while (!(MDIO::read(Phy, STATUS) & STATUS_LINK_STATUS))
            ;

        Timer::Delay(Microsecond(1));
    }

    static bool duplex() { return MDIO::read(Phy, STATUS) & (1 << 13); }

    static bool negotiate() {
        MDIO::set(Phy, BASIC_CONTROL, BASIC_CONTROL_AUTO_NEGOTIATION_ENABLE | BASIC_CONTROL_RE_AUTO_NEGOTIATION);
        while (!(MDIO::read(Phy, BASIC_STATUS) & BASIC_STATUS_AUTO_NEGOTIATION_COMPLETE))
            ;
        while (!((MDIO::read(Phy, MASTER_SLAVE_STATUS_REGISTER) & LOCAL_RECEIVER_STATUS) &&
                 (MDIO::read(Phy, MASTER_SLAVE_STATUS_REGISTER) & REMOTE_RECEIVER)))
            ;
        return true;
    }

    static unsigned speed() {
        switch ((MDIO::read(Phy, STATUS) >> 14) & 0x3) {
            case 2: return 1000;
            case 1: return 100;
            default: return 10;
        }
    }

  private:
    static constexpr int Phy = 0;
};

template <unsigned long Base> class DWC_Ether_QoS_MAC : Driver {
    enum Bits {
        PACKET_FILTER_RECEIVE_ALL        = 1 << 31,
        PACKET_FILTER_PROMISCUOUS_MODE   = 1,
        CONFIGURATION_TRANSMITTER_ENABLE = 1 << 1,
        CONFIGURATION_RECEIVER_ENABLE    = 1,
        RX_QUEUE_CONTROL0_QUEUE0_ENABLE  = 2,
        CONFIGURATION_CST                = 1 << 21,
        CONFIGURATION_PS                 = 1 << 15,
        CONFIGURATION_FES                = 1 << 14,
        CONFIGURATION_DM                 = 1 << 13,

    };

    enum Registers {
        CONFIGURATION     = 0x0,
        PACKET_FILTER     = 0x8,
        RX_QUEUE_CONTROL0 = 0xa0,
        ADDRESS0_LOW      = 0x304,
        ADDRESS0_HIGH     = 0x300,

    };

  public:
    static void init() {
        TraceIn();
        Reg32(Base, PACKET_FILTER) |= PACKET_FILTER_RECEIVE_ALL | PACKET_FILTER_PROMISCUOUS_MODE;
        Reg32(Base, RX_QUEUE_CONTROL0) = RX_QUEUE_CONTROL0_QUEUE0_ENABLE;
        Reg32(Base, CONFIGURATION) |= CONFIGURATION_CST;
        Reg32(Base, CONFIGURATION) |= CONFIGURATION_RECEIVER_ENABLE | CONFIGURATION_TRANSMITTER_ENABLE;
        TraceOut();
    }

    static void duplex(bool full) {
        TraceIn(full);
        Reg32(Base, CONFIGURATION) &= ~CONFIGURATION_DM;
        Reg32(Base, CONFIGURATION) |= (full ? CONFIGURATION_DM : 0);
    }

    static void speed(unsigned int speed) {
        TraceIn(speed);
        switch (speed) {
            case 1000:
                Reg32(Base, CONFIGURATION) &= ~CONFIGURATION_PS;
                Reg32(Base, CONFIGURATION) &= ~CONFIGURATION_FES;
                break;
            case 100:
                Reg32(Base, CONFIGURATION) |= CONFIGURATION_PS;
                Reg32(Base, CONFIGURATION) |= CONFIGURATION_FES;
                break;
            case 10:
                Reg32(Base, CONFIGURATION) |= CONFIGURATION_PS;
                Reg32(Base, CONFIGURATION) &= ~CONFIGURATION_FES;
                break;
        }
    }
};

class DWC_Ether_QoS_Payload {
  protected:
    unsigned payload_[1522];
};

class DWC_Ether_QoS_Buffer : DWC_Ether_QoS_Payload, public NetworkBuffer {
  public:
    DWC_Ether_QoS_Buffer(size_t head = 0, size_t tail = 0)
        : NetworkBuffer(this, head, tail, &references_),
          references_(0) {}

  public:
    Atomic<uint32_t> references_;
};

template <typename MyTraits> class DWC_Ether_QoS_DMA : public Driver {
    typedef Meta::GetFromTypeList<Traits<QUARK::CacheController>::Devices, 0>::Result Cache;

    struct Descriptor {
        uint32_t des0;
        uint32_t des1;
        unsigned int des2;
        unsigned int des3;

        enum {
            OWN   = 1 << 31,
            IOC   = 1 << 30,
            FD    = 1 << 29,
            LD    = 1 << 28,
            ES    = 1 << 15,
            BUF1V = 1 << 24,
        };

        auto length() const { return des3 & 0x3FFF; }

        template <typename T = uintptr_t> T buffer() const {
            uint64_t pointer = (static_cast<uint64_t>(des1) << 32) | static_cast<uint64_t>(des0);
            return reinterpret_cast<T>(pointer);
        }

        template <typename T = uintptr_t> void buffer(T pointer) {
            uintptr_t address = reinterpret_cast<uintptr_t>(pointer);
            des0              = static_cast<uint32_t>(address);
            des1              = static_cast<uint32_t>(address >> 32);
        }
    };

    enum Registers {
        DMA_MODE               = 0x1000,
        DMA_SYSBUS_MODE        = 0x1004,
        CH0_CONTROL            = 0x1100,
        CH0_TX_CONTROL         = 0x1104,
        CH0_RX_CONTROL         = 0x1108,
        CH0_TXDESC_LIST_HADDR  = 0x1110,
        CH0_TXDESC_LIST_ADDR   = 0x1114,
        CH0_RXDESC_LIST_HADDR  = 0x1118,
        CH0_RXDESC_LIST_ADDR   = 0x111c,
        CH0_TX_TAIL_POINTER    = 0x1120,
        CH0_RX_TAIL_POINTER    = 0x1128,
        CH0_TXDESC_RING_LENGTH = 0x112c,
        CH0_RXDESC_RING_LENGTH = 0x1130,
        CH0_INTERRUPT_ENABLE   = 0x1134,
        CH0_INTERRUPT_STATUS   = 0x1160,
    };

    enum Bits {
        MODE_SOFTWARE_RESET   = 1 << 0,
        SYSBUS_MODE_EAME      = 1 << 11,
        SYSBUS_MODE_FB        = 1 << 0,
        SYSBUS_MODE_BLEN4     = 1 << 1,
        INTERRUPT_ENABLE_NIE  = 1 << 15,
        INTERRUPT_ENABLE_AIE  = 1 << 14,
        INTERRUPT_ENABLE_RIE  = 1 << 6,
        INTERRUPT_ENABLE_RBUE = 1 << 7,
        INTERRUPT_STATUS_RI   = 1 << 6,
        INTERRUPT_STATUS_RBU  = 1 << 7,
    };

  public:
    DWC_Ether_QoS_DMA()
        : sx_head_(0),
          rx_head_(0),
          rx_tail_(0) {
        TraceIn();

        for (size_t i = 0; i < MyTraits::SendBufferCount; i++) {
            memset(&sx_descriptors_[i], 0, sizeof(Descriptor));
            Cache::flush(&sx_descriptors_[i], sizeof(Descriptor));
            sx_list_.insert(sx_buffers_[i].node());
        }

        for (size_t i = 0; i < MyTraits::ReceiveBufferCount - 1; i++) {
            release(&rx_buffers_[i]);
        }

        Reg32(Address, DMA_SYSBUS_MODE) |= SYSBUS_MODE_EAME;

        uintptr_t rx_addr                      = reinterpret_cast<uintptr_t>(rx_descriptors_);
        Reg32(Address, CH0_RXDESC_LIST_ADDR)   = static_cast<uint32_t>(rx_addr);
        Reg32(Address, CH0_RXDESC_LIST_HADDR)  = static_cast<uint32_t>(rx_addr >> 32);
        Reg32(Address, CH0_RXDESC_RING_LENGTH) = MyTraits::ReceiveBufferCount - 1;

        uintptr_t tx_addr                      = reinterpret_cast<uintptr_t>(sx_descriptors_);
        Reg32(Address, CH0_TXDESC_LIST_ADDR)   = static_cast<uint32_t>(tx_addr);
        Reg32(Address, CH0_TXDESC_LIST_HADDR)  = static_cast<uint32_t>(tx_addr >> 32);
        Reg32(Address, CH0_TXDESC_RING_LENGTH) = MyTraits::SendBufferCount - 1;

        Reg32(Address, CH0_TX_CONTROL) |= 1;
        Reg32(Address, CH0_RX_CONTROL) |= 1;

        release(&rx_buffers_[MyTraits::ReceiveBufferCount - 1]);

        TraceOut();
    }

    static void reset() {
        Reg32(Address, DMA_MODE) |= MODE_SOFTWARE_RESET;
        Timer::Delay(Microsecond(1));
        while (Reg32(Address, DMA_MODE) & MODE_SOFTWARE_RESET)
            ;
    }

    DWC_Ether_QoS_Buffer *alloc(size_t length) {
        NetworkBuffer::Node *node = nullptr;

        while (!(node = sx_list_.remove())) {
            Thread::yield();
        }

        DWC_Ether_QoS_Buffer *buffer = static_cast<DWC_Ether_QoS_Buffer *>(node->value);

        new (buffer) DWC_Ether_QoS_Buffer(0, length);

        return buffer;
    }

    void free(DWC_Ether_QoS_Buffer *buffer) { sx_list_.insert(buffer->node()); }

    int send(const void *data, size_t length) {
        Cache::flush(data, length);

        sx_lock_.acquire();

        size_t &i = sx_head_;

        Descriptor &descriptor = sx_descriptors_[i % MyTraits::SendBufferCount];

        Cache::invalidate(&descriptor, sizeof(Descriptor));
        if (descriptor.des3 & Descriptor::OWN) {
            sx_lock_.release();
            return -1;
        }

        descriptor.buffer(data);
        descriptor.des2 = length & 0x3FFF;
        descriptor.des3 = Descriptor::OWN | Descriptor::FD | Descriptor::LD | (length & 0x3FFF);
        Cache::flush(&descriptor, sizeof(Descriptor));

        Reg32(Address, CH0_TX_TAIL_POINTER) = reinterpret_cast<uintptr_t>(sx_descriptors_ + ++i);

        sx_lock_.release();

        return length;
    }

    DWC_Ether_QoS_Buffer *receive() {
        size_t &i = rx_head_;

        Descriptor &descriptor = rx_descriptors_[i % MyTraits::ReceiveBufferCount];

        Cache::invalidate(&descriptor, sizeof(Descriptor));

        if (descriptor.des3 & Descriptor::OWN) return nullptr;

        i++;

        DWC_Ether_QoS_Buffer *buffer = descriptor.template buffer<DWC_Ether_QoS_Buffer *>();

        Cache::invalidate(buffer, descriptor.length());

        new (buffer) DWC_Ether_QoS_Buffer(0, descriptor.length());

        buffer->references_ = 1;

        return buffer;
    }

    void release(DWC_Ether_QoS_Buffer *buffer) {
        size_t i = rx_tail_++ % MyTraits::ReceiveBufferCount;

        Descriptor &descriptor = rx_descriptors_[i];

        descriptor.buffer(buffer);

        descriptor.des2 = 0;

        descriptor.des3 = Descriptor::OWN | Descriptor::IOC | Descriptor::BUF1V;

        Cache::flush(&descriptor, sizeof(Descriptor));

        Reg32(Address, CH0_RX_TAIL_POINTER) = reinterpret_cast<uintptr_t>(rx_descriptors_ + i);
    }

  private:
    static constexpr uintptr_t Address = MyTraits::Address;

  private:
    collections::FIFO<NetworkBuffer::Node, Mutex> sx_list_;
    Descriptor sx_descriptors_[MyTraits::SendBufferCount];
    DWC_Ether_QoS_Buffer sx_buffers_[MyTraits::SendBufferCount];

    Descriptor rx_descriptors_[MyTraits::ReceiveBufferCount];
    DWC_Ether_QoS_Buffer rx_buffers_[MyTraits::ReceiveBufferCount];

    Mutex sx_lock_;
    size_t sx_head_;
    size_t rx_head_;
    size_t rx_tail_;
};

template <unsigned long Base> class DWC_Ether_QoS_MTL : Driver {
    enum Bits {
        TX_QUEUE0_OPERATION_MODE_TSF    = 1 << 1,
        TX_QUEUE0_OPERATION_MODE_ENABLE = 1 << 3,
        RX_QUEUE0_OPERATION_MODE_RSF    = 1 << 5,
        RX_QUEUE0_OPERATION_MODE_FEP    = 1 << 4,
        RX_QUEUE0_OPERATION_MODE_FUP    = 1 << 3,

    };

    enum Registers {
        TX_QUEUE0_OPERATION_MODE = 0xd00,
        RX_QUEUE0_OPERATION_MODE = 0xd30,

    };

  public:
    static void init() {
        TraceIn();
        Reg32(Base, TX_QUEUE0_OPERATION_MODE) |= TX_QUEUE0_OPERATION_MODE_TSF | TX_QUEUE0_OPERATION_MODE_ENABLE;
        Reg32(Base, RX_QUEUE0_OPERATION_MODE) |= RX_QUEUE0_OPERATION_MODE_FUP | RX_QUEUE0_OPERATION_MODE_RSF;
        TraceOut();
    }
};

template <typename Tag> class DWC_Ether_QoS final : public EthernetDevice {
    enum Registers {
        CH0_INTERRUPT_ENABLE = 0x1134,
        CH0_INTERRUPT_STATUS = 0x1160,
    };

    enum Bits {
        INTERRUPT_ENABLE_NIE  = 1 << 15,
        INTERRUPT_ENABLE_AIE  = 1 << 14,
        INTERRUPT_ENABLE_RIE  = 1 << 6,
        INTERRUPT_ENABLE_TIE  = 1 << 0,
        INTERRUPT_STATUS_TI   = 1 << 0,
        INTERRUPT_ENABLE_RBUE = 1 << 7,
        INTERRUPT_STATUS_RI   = 1 << 6,
        INTERRUPT_STATUS_RBU  = 1 << 7,
    };

  public:
    using MyTraits = Traits<Tag>;
    using DMA      = DWC_Ether_QoS_DMA<MyTraits>;
    using MTL      = DWC_Ether_QoS_MTL<MyTraits::Address>;
    using PHY      = DWC_Ether_QoS_PHY<MyTraits::Address>;
    using MAC      = DWC_Ether_QoS_MAC<MyTraits::Address>;

    DWC_Ether_QoS()
        : address_(MyTraits::MAC) {
        TraceIn();
        PHY::init();
        DMA::reset();
        dma_ = new DMA();
        MAC::duplex(PHY::duplex());
        MAC::speed(PHY::speed());
        MTL::init();
        MAC::init();
        NetworkDevice::init();

        for (auto &i : MyTraits::IRQs) {
            IC::install(i, onTrap);
        }

        reg32(CH0_INTERRUPT_ENABLE) |= INTERRUPT_ENABLE_NIE | INTERRUPT_ENABLE_RIE;
        reg32(CH0_INTERRUPT_ENABLE) |= INTERRUPT_ENABLE_AIE | INTERRUPT_ENABLE_RBUE;

        TraceOut();
    }

    NetworkBuffer *alloc(size_t length) override {
        NetworkBuffer *buffer = dma_->alloc(length + sizeof(Header));
        buffer->advance(sizeof(Header));
        return buffer;
    }

    NetworkBuffer *receive() override {
        NetworkBuffer *buffer = dma_->receive();
        if (buffer) buffer->advance(sizeof(Header));
        return buffer;
    }

    int send(NetworkBuffer *buffer) override {
        int response = dma_->send(buffer->start(), buffer->length());
        free(buffer);
        return response;
    }

    void release(NetworkBuffer *buffer) override { dma_->release(static_cast<DWC_Ether_QoS_Buffer *>(buffer)); }

    NetworkAddress address() const override { return address_; }

    void address(const NetworkAddress &address) override { new (&address_) Address(address); }

    void free(NetworkBuffer *buffer) { dma_->free(static_cast<DWC_Ether_QoS_Buffer *>(buffer)); }

    static void onTrap(size_t) {
        volatile uint32_t &status = reg32(CH0_INTERRUPT_STATUS);
        DWC_Ether_QoS *self       = instance();

        if (status & INTERRUPT_STATUS_RI) {
            self->notify();
            status = INTERRUPT_STATUS_RI;
        }

        if (status & INTERRUPT_STATUS_RBU) {
            self->notify();
            status = INTERRUPT_STATUS_RBU;
        }
    }

    static auto *instance() {
        static DWC_Ether_QoS instance;
        return &instance;
    }

    static volatile uint32_t &reg32(size_t offset) { return *reinterpret_cast<volatile uint32_t *>(MyTraits::Address + offset); }

  private:
    Address address_;
    DMA *dma_;
};

} // namespace QUARK
