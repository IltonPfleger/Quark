#pragma once

#include <Meta.hpp>
#include <Mutex.hpp>
#include <architecture/Timer.hpp>
#include <hypervisor/VirtualSwitch.hpp>
#include <machine/Machine.hpp>
#include <machine/aes.h>
#include <machine/nic.h>
#include <memory/Memory.hpp>
#include <network/Ethernet.hpp>
#include <utility/Delay.hpp>
#include <utility/collections/CircularBuffer.hpp>
#include <utility/observer.h>
#include <utility/ostream.h>

typedef QUARK::Meta::GetFromTypeList<QUARK::Traits<QUARK::Ethernet>::Devices, 0>::Result Device;
typedef QUARK::VirtualSwitch<Device> LocalNetwork;
typedef QUARK::NetworkBuffer NetworkBuffer;

class UDPNIC : public NIC<Only_Data_UDP_Wrpayloader>, public LocalNetwork::Observer {
    static const UInt32 KEY_SIZE = Traits<TSTP>::KEY_SIZE;
    static const UInt32 MTU      = NIC<Only_Data_UDP_Wrpayloader>::MTU;

    typedef AES<KEY_SIZE> _AES;
    static _AES _aes;

  public:
    UDPNIC()
        : m_network(LocalNetwork::instance()) {
        db<NIC>(TRC) << "UDPNIC::UDPNIC()" << endl;

        for (int i = 0; i < NumberOfBuffers; i++) {
            sx_free_.insert(&sx_buffers_[i]);
        }

        // new QUARK::Thread(worker, this);

        m_network->attach(this);
    }

    ~UDPNIC() override {
        db<NIC>(ERR) << "UDPNIC::~UDPNIC()" << endl;
        QUARK::CPU::halt();
    }

    int send(const Address &dst, const Protocol &prot, const void *data, UInt32 size) override {
        db<NIC>(ERR) << "UDPNIC::send(dst=" << dst << ", prot=" << prot << ", data=" << data << ", size=" << size << ")" << endl;
        QUARK::CPU::halt();
        return 0;
    }

    int receive(Address *src, Protocol *prot, void *data, UInt32 size) override {
        db<NIC>(ERR) << "UDPNIC::receive(size=" << size << ")" << endl;
        QUARK::CPU::halt();
        return 0;
    }

    Buffer *alloc(const Address &dst, const Protocol &prot, UInt32 once, UInt32 always, UInt32 payload) override {
        db<UDPNIC>(TRC) << "UDPNIC::alloc(s=" << address() << ",d=" << dst << ",p=" << hex << prot << dec << ",on=" << once
                        << ",al=" << always << ",ld=" << payload << ")" << endl;

        auto *node = sx_free_.remove();
        assert(node);
        Buffer *buf = &node->value;
        new (buf) Buffer(this, 0);
        buf->fill(once + always + payload + 14, m_configuration.address, dst, prot);
        buf->is_microframe           = false;
        buf->trusted                 = false;
        buf->is_new                  = true;
        buf->random_backoff_exponent = 0;
        buf->microframe_count        = 0;
        buf->times_txed              = 0;
        buf->offset                  = 0;
        return buf;
    }

    int send(Buffer *buffer) override {
        db<NIC>(TRC) << "UDPNIC::send(buf=" << buffer << ") size: " << buffer->size() << endl;
        unsigned int size      = buffer->size();
        NetworkBuffer *dbuffer = m_network->alloc(size);
        dbuffer->shrink(dbuffer->offset());
        dbuffer->rewind(dbuffer->offset());
        memcpy(dbuffer->data(), buffer->frame(), size);
        m_network->send(dbuffer);
        for (int i = 0; i < NumberOfBuffers; i++) {
            if (buffer == &sx_buffers_[i].value) {
                sx_free_.insert(&sx_buffers_[i]);
            }
        }
        return size;
    }

    void free(Buffer *buffer) override {
        (void)buffer;
        // db<NIC>(TRC) << "UDPNIC::free(buf=" << buffer << ")" << endl;
        // for (int i = 0; i < NumberOfBuffers; i++) {
        //     if (buffer == &rx_buffers_[i].value) {
        //         rx_free_.insert(&rx_buffers_[i]);
        //         break;
        //     }
        // }
    }

    const Address &address() override {
        db<NIC>(TRC) << "UDPNIC::address() [get]" << endl;
        return m_configuration.address;
    }

    void address(const Address &addr) override {
        db<NIC>(TRC) << "UDPNIC::address(addr=" << addr << ") [set]" << endl;
        m_configuration.address = addr;
    }

    bool reconfigure(const Configuration *c = nullptr) override {
        db<NIC>(ERR) << "UDPNIC::reconfigure(conf=" << c << ")" << endl;
        QUARK::CPU::halt();
        return true;
    }

    const Configuration &configuration() override {
        db<NIC>(TRC) << "UDPNIC::configuration()" << endl;
        return m_configuration;
    }

    const Statistics &statistics() override {
        db<NIC>(TRC) << "UDPNIC::statistics()" << endl;
        m_statistics.time_stamp = TSC::time_stamp();
        return m_statistics;
    }

    void update(const NetworkBuffer *buffer) {
        db<NIC>(TRC) << "UDPNIC::update " << buffer->length() << endl;

        auto *header = buffer->start<QUARK::Ethernet::Header *>();
        if (header->protocol() == PROTO_TSTP) {
            Buffer b(this, 0);
            b.size(buffer->length());
            b.sfdts = TSC::time_stamp();
            b.fill(buffer->length(), address(), address(), PROTO_TSTP, buffer->start(), buffer->length());
            notify(PROTO_TSTP, &b);
        }

        // if (!rx_.insert(b)) QUARK::Console::println("LOST");
    }

    // static void *worker(void *pointer) {
    //     auto *self = reinterpret_cast<UDPNIC *>(pointer);

    //    while (1) {
    //        while (true) {
    //            Buffer b(self, 0);
    //            if (!self->rx_.remove(b)) break;
    //            self->notify(PROTO_TSTP, &b);
    //        }
    //        QUARK::Delay(QUARK::Microsecond(100'000));
    //    }
    //    return nullptr;
    //}

  private:
    static unsigned char GRP_KEY[16];

  private:
    static constexpr int NumberOfBuffers = 32;

    QUARK::collections::Node<Buffer> sx_buffers_[NumberOfBuffers];
    QUARK::collections::FIFO<QUARK::collections::Node<Buffer>, QUARK::Mutex> sx_free_;
    QUARK::collections::CircularBuffer<Buffer, NumberOfBuffers, QUARK::Mutex> rx_{};

    LocalNetwork *m_network;
    Configuration m_configuration;
    Statistics m_statistics;
};
