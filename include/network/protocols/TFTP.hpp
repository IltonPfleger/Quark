#pragma once

#include <Semaphore.hpp>
#include <network/protocols/UDP.hpp>

namespace QUARK {

class TFTP : public Observer<NetworkBuffer, uint16_t, uint16_t> {
    enum Operation : uint16_t {
        RRQ   = 1,
        WRQ   = 2,
        DATA  = 3,
        ACK   = 4,
        ERROR = 5,
        OACK  = 6,
    };

  public:
    TFTP(UDP &udp)
        : udp_(udp),
          semaphore_(0),
          done_(true) {
        udp_.attach(this);
    }

    ~TFTP() { udp_.detach(this); }

    void request(const char *filename) {
        NetworkBuffer *packet = udp_.alloc(512);
        size_t length         = strlen(filename);
        uint16_t *operation   = packet->data<uint16_t *>();
        *operation            = CPU::htobe16(Operation::RRQ);
        char *pointer         = reinterpret_cast<char *>(operation + 1);
        memcpy(pointer, filename, length + 1);
        pointer += length + 1;
        memcpy(pointer, "octet", 6);
        pointer += 6;
        memcpy(pointer, "blksize", 8);
        pointer += 8;
        memcpy(pointer, k_blksize_string, sizeof(k_blksize_string));
        pointer += sizeof(k_blksize_string);
        size_t total = 2 + length + 6 + 8 + sizeof(k_blksize_string) + 1;
        packet->shrink(packet->length() - packet->offset() - total);
        udp_.send(server_, 69, packet);
    }

    size_t request(const NetworkAddress &&address, const char *filename, void *buffer, size_t size) {
        buffer_      = static_cast<uint8_t *>(buffer);
        buffer_size_ = size;
        received_    = 0;
        block_       = 1;
        done_        = false;
        error_       = false;
        server_      = address;

        request(filename);
        semaphore_.p();

        if (!error_) {
            Console::println("\n[OK]");
            return received_;
        }

        return 0;
    }

    void update(NetworkBuffer packet, uint16_t, uint16_t source) override {
        uint16_t *header   = packet.data<uint16_t *>();
        uint16_t operation = CPU::be16toh(*header);

        switch (operation) {
            case OACK: {
                ack(0, source);
                break;
            }
            case DATA: {
                uint16_t block = CPU::be16toh(*(header + 1));
                size_t length  = packet.length() - packet.offset() - 4;
                uint8_t *data  = reinterpret_cast<uint8_t *>(header + 2);
                onData(data, block, length, source);
                break;
            }
            case ERROR: {
                onError();
                break;
            }
        }
    }

    void onData(uint8_t *data, uint16_t block, size_t length, uint16_t source) {
        if (done_) return;

        if (received_ + length > buffer_size_) {
            onError();
            return;
        }

        if (block != block_) {
            ack(block_ - 1, source);
            return;
        };

        block_++;

        ack(block, source);

        memcpy(buffer_ + received_, data, length);

        received_ += length;

        if constexpr (Trace)
            if (block % 32 == 0) Console::print('#');

        if (length < k_blksize_int) {
            done_ = true;
            semaphore_.v();
        }
    }

    void onError() {
        error_ = true;
        semaphore_.v();
    }

    void ack(uint16_t block, uint16_t source) {
        NetworkBuffer *buffer = udp_.alloc(4);
        uint16_t *payload     = buffer->data<uint16_t *>();
        payload[0]            = CPU::htobe16(Operation::ACK);
        payload[1]            = CPU::htobe16(block);
        udp_.send(server_, source, buffer);
    }

  private:
    static constexpr const char *k_blksize_string     = "1468";
    static constexpr const unsigned int k_blksize_int = 1468;
    static constexpr bool Trace                       = true;

  private:
    UDP &udp_;
    NetworkAddress server_;
    Semaphore semaphore_;

    uint8_t *buffer_;
    size_t buffer_size_;

    size_t received_;

    uint16_t block_;

    volatile bool done_;
    volatile bool error_;
};

} // namespace QUARK
