#pragma once

#include <Semaphore.hpp>
#include <network/protocols/UDP.hpp>

namespace QUARK {

class TFTP : public Observer<NetworkBuffer, uint16_t, uint16_t> {

    enum Operation : uint16_t {
        RRQ  = 1,
        WRQ  = 2,
        DATA = 3,
        ACK  = 4,
        ERR  = 5,
        OACK = 6,
    };

    enum class State {
        ERROR,
        DONE,
        WAITING,
        RECEIVING,
    };

  public:
    TFTP(UDP &udp)
        : udp_(udp),
          handler_(0),
          state_(State::DONE) {
        udp_.attach(this);
    }

    ~TFTP() { udp_.detach(this); }

    int request(const NetworkAddress &&address, const char *filename, void *buffer, size_t size) {
        TraceIn(filename);

        buffer_   = static_cast<uint8_t *>(buffer);
        size_     = size;
        received_ = 0;
        block_    = 1;
        server_   = address;
        state_    = State::WAITING;

        request(filename);

        handler_.p();

        TraceOut(received_);

        if (state_ != State::DONE || received_ == 0) return -1;

        return received_;
    }

    void update(NetworkBuffer packet, uint16_t, uint16_t source) override {
        uint16_t *header   = packet.data<uint16_t *>();
        uint16_t operation = CPU::be16toh(*header);

        if (state_ == State::WAITING) {
            state_ = State::RECEIVING;
            port_  = source;
        } else if (state_ == State::RECEIVING && port_ != source) {
            return;
        }

        switch (operation) {
            case OACK: {
                ack(0);
                break;
            }
            case DATA: {
                if (state_ == State::WAITING) state_ = State::RECEIVING;
                if (state_ != State::RECEIVING) return;
                uint16_t block = CPU::be16toh(*(header + 1));
                size_t length  = packet.length() - packet.offset() - 4;
                uint8_t *data  = reinterpret_cast<uint8_t *>(header + 2);
                onData(data, block, length);
                break;
            }
            case ERR: {
                error();
                break;
            }
        }
    }

    void onData(uint8_t *data, uint16_t block, size_t length) {
        if (received_ + length > size_) {
            error();
            return;
        }

        if (block != block_) {
            ack(block_ - 1);
            return;
        };

        block_++;

        ack(block);

        memcpy(buffer_ + received_, data, length);

        received_ += length;

        if (block % 32 == 0) Trace('#');

        if (length < BlockSize) {
            state_ = State::DONE;
            handler_.v();
        }
    }

    void error() {
        state_ = State::ERROR;
        handler_.v();
    }

    void ack(uint16_t block) {
        NetworkBuffer *buffer = udp_.alloc(4);
        uint16_t *payload     = buffer->data<uint16_t *>();
        payload[0]            = CPU::htobe16(Operation::ACK);
        payload[1]            = CPU::htobe16(block);
        udp_.send(server_, port_, buffer);
    }

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
        memcpy(pointer, BlockSizeString, sizeof(BlockSizeString));
        pointer += sizeof(BlockSizeString);
        size_t total = 2 + length + 6 + 8 + sizeof(BlockSizeString) + 1;
        packet->shrink(packet->length() - packet->offset() - total);
        udp_.send(server_, 69, packet);
    }

  private:
    static constexpr const char BlockSizeString[] = "1468";
    static constexpr uint32_t BlockSize           = 1468;
    static constexpr uint32_t MaxRetry            = 5;
    static constexpr Microsecond TimeoutDelay     = 1'000'000;

  private:
    UDP &udp_;
    NetworkAddress server_;
    Semaphore handler_;
    State state_;
    uint8_t *buffer_;
    size_t size_;
    size_t received_;
    uint16_t block_;
    uint16_t port_;
};

} // namespace QUARK
