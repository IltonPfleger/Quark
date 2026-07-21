#ifndef __QUARK_NETWORK_PROTOCOLS_ARP__
#define __QUARK_NETWORK_PROTOCOLS_ARP__

#include <Mutex.hpp>
#include <Thread.hpp>
#include <utility/collections/Hash.hpp>

namespace QUARK {

template <typename HardwareLayerType, typename ProtocolLayerType> class ARP : public Observer<const NetworkBuffer *> {
  public:
    enum : uint16_t { ProtocolValue = 0x0806 };
    enum : uint16_t { REQUEST = 1, REPLY = 2 };

    typedef typename HardwareLayerType::Address HA;
    typedef ProtocolLayerType::Address PA;

    struct Header {
        Header()
            : htype(CPU::htobe16(1)),
              ptype(CPU::htobe16(ProtocolLayerType::ProtocolValue)),
              hlen(sizeof(HA)),
              plen(sizeof(PA)),
              operation(CPU::htobe16(0)) {}

        uint16_t htype;
        uint16_t ptype;
        uint8_t hlen;
        uint8_t plen;
        uint16_t operation;
        HA sha;
        PA spa;
        HA dha;
        PA dpa;
    } __attribute__((packed));

    struct Entry {
        HA ha{};
        PA pa{};
        size_t waiting{0};
        Semaphore handler{};
        Mutex mutex{};
        bool valid{false};
    };

    struct Hasher {
        size_t operator()(const PA &pa) const { return pa[3]; }
    };

    typedef Hash<PA, Entry, 256, Hasher> Table;

    ARP(HardwareLayerType &device)
        : device_(device) {
        device_.attach(this);
    }

    ~ARP() { device_.detach(this); }

    void bind(const NetworkAddress &a) { pa_ = a; }

    bool resolve(const PA &pa, const HA &solver, HA &destination) {
        while (true) {
            auto &entry = table_[pa];

            entry.mutex.acquire();

            if (entry.valid && entry.pa == pa) {
                memcpy(destination, &entry.ha, sizeof(HA));
                entry.mutex.release();
                return true;
            }

            entry.waiting++;
            request(pa, solver);
            entry.mutex.release();

            Alarm _(TimeoutDelay, entry.handler);
        }
    }

  private:
    void update(const NetworkBuffer *buffer) {
        {
            typename HardwareLayerType::Header *header = buffer->start<typename HardwareLayerType::Header *>();
            if (header->protocol() != ProtocolValue) return;
        }
        Header *header = buffer->data<Header *>();
        if (CPU::be16toh(header->operation) == REPLY) {
            onReply(*header);
        } else {
            onRequest(*header);
        }
    }

    void request(const PA &pa, const HA &solver) {
        NetworkBuffer *buffer = device_.alloc(sizeof(Header));

        Header *header = new (buffer->data()) Header();

        header->operation = CPU::htobe16(REQUEST);

        header->sha = device_.address();
        header->spa = pa_;

        header->dha = HA();
        header->dpa = pa;

        device_.send(solver, ProtocolValue, buffer);
    }

    void onReply(const Header &received) {
        auto &entry = table_[received.spa];
        entry.mutex.acquire();
        entry.pa    = received.spa;
        entry.ha    = received.sha;
        entry.valid = true;
        for (size_t i = 0; i < entry.waiting; i++) {
            entry.handler.v();
            entry.waiting = 0;
        }
        entry.mutex.release();
    }

    void onRequest(const Header &received) {
        if (received.dpa == pa_) {
            NetworkBuffer *buffer = device_.alloc(sizeof(Header));

            Header *header = new (buffer->data()) Header();

            header->operation = CPU::htobe16(REPLY);

            header->sha = device_.address();
            header->spa = pa_;

            header->dha = received.sha;
            header->dpa = received.spa;

            device_.send(received.sha, ProtocolValue, buffer);
        }
    }

  private:
    static constexpr Microsecond TimeoutDelay = 1'000'000;

  private:
    HardwareLayerType &device_;
    PA pa_;
    Table table_;
};

} // namespace QUARK

#endif
