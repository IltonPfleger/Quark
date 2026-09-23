#pragma once

#include <Traits.hpp>
#include <architecture/IC.hpp>
#include <drivers/can/CANDevice.hpp>

namespace QUARK {

template <typename Tag> class IPMSCANFD : public CANDevice {
private:
  using Traits = QUARK::Traits<Tag>;

  static constexpr uintptr_t Base = Traits::Address;
  static constexpr size_t MaxPayload = 8;

  static constexpr bool DebugMode = true;
  static constexpr bool InternalLoopback = false;
  static constexpr bool ExternalLoopback = true;

  enum class Register : uintptr_t {
    RBUF_ID = 0x00,
    RBUF_CTL = 0x04,
    RBUF_DATA1 = 0x08,
    RBUF_DATA2 = 0x0C,

    TBUF_ID = 0x50,
    TBUF_CTL = 0x54,
    TBUF_DATA1 = 0x58,
    TBUF_DATA2 = 0x5C,

    STATUS = 0xA0,
    TCMD = 0xA1,
    RCTRL = 0xA3,
    RTIE = 0xA4,
    RTIF = 0xA5,
    ERRINT = 0xA6,
    LIMIT = 0xA7,
    S_SEG_1 = 0xA8,
    F_SEG_1 = 0xAC,
    EALCAP = 0xB0
  };

  enum Mask : uint8_t {
    RESET = 0x80,

    // Error
    EIF = 0x02,
    EPIE = 0x10,
    EPIF = 0x10,
    BEIF = 0x01,

    // RX
    RIE = 0x02,
    RIF = 0x80,
    RFIF = 0x20,
    RAFIF = 0x10,
    RREL = 0x10,
    RNE = 0x03,

    // TX
    TPIF = 0x08,
    TSIF = 0x04,
    TPE = 0x10,
    TBSEL = 0x80,

    // Frame
    IDE = 0x80,
    RTR = 0x40,
    DLC = 0x0F,

    // Modes
    SACK = 0x80,
    LBE = 0x40,
    LBI = 0x20,
    STBY = 0x20,

    // Status
    BUSOFF = 0x01
  };

  struct BitTiming {
    size_t baudrate;
    size_t t[6];
  };

  static constexpr BitTiming TimingTable[] = {
      {1000, {126, 2, 3, 2, 1, 5}},  {800, {126, 3, 4, 2, 1, 5}},
      {500, {126, 6, 7, 2, 1, 5}},   {250, {251, 6, 7, 2, 1, 10}},
      {125, {1338, 1, 2, 2, 1, 53}}, {100, {833, 4, 5, 2, 1, 33}},
      {50, {1666, 4, 5, 2, 1, 66}},  {20, {3333, 6, 6, 2, 1, 132}},
      {10, {6666, 6, 6, 2, 1, 264}}};

private:
  IPMSCANFD() {
    IC::install(Traits::IRQs[0], onTrap);

    set8(Register::STATUS, RESET);

    configure(500, InternalLoopback, ExternalLoopback);

    clear8(Register::STATUS, RESET);
    set8(Register::ERRINT, EPIE);
  }

public:
  static auto &instance() {
    static IPMSCANFD _;
    return _;
  }

  int doSend(NetworkBuffer *raw) override {
    auto *frame = static_cast<CAN::Buffer *>(raw);

    uint8_t cmd = read8(Register::TCMD);
    cmd &= ~TBSEL;
    cmd &= ~STBY;
    write8(Register::TCMD, cmd);

    reg32(Register::TBUF_ID) = frame->id();

    uint32_t ctl = frame->length() & DLC;
    if (frame->isRemote())
      ctl |= RTR;
    if (frame->isExtended())
      ctl |= IDE;

    reg32(Register::TBUF_CTL) = ctl;

    auto *data = frame->data<uint32_t *>();
    reg32(Register::TBUF_DATA1) = data[0];
    reg32(Register::TBUF_DATA2) = data[1];

    set8(Register::TCMD, TPE);
    return 0;
  }

  NetworkBuffer *doReceive() override {
    if (!(read8(Register::RCTRL) & RNE))
      return nullptr;

    uint32_t id = reg32(Register::RBUF_ID);
    uint8_t ctl = read8(Register::RBUF_CTL);

    uint8_t dlc = ctl & DLC;
    if (dlc > MaxPayload)
      dlc = MaxPayload;

    bool ide = ctl & IDE;
    bool rtr = ctl & RTR;

    auto *frame = new CAN::Buffer(id, dlc, ide, rtr);

    if (!rtr) {
      auto *data = frame->data<uint32_t *>();
      data[0] = reg32(Register::RBUF_DATA1);
      data[1] = reg32(Register::RBUF_DATA2);
    }

    set8(Register::RCTRL, RREL);
    return frame;
  }

  NetworkBuffer *doAlloc(size_t) override { return new CAN::Buffer(); }
  void doRelease(NetworkBuffer *raw) override {
    delete static_cast<CAN::Buffer *>(raw);
  }
  void doFree(NetworkBuffer *raw) override {
    delete static_cast<CAN::Buffer *>(raw);
  }

private:
  static void onTrap(size_t) {
    uint8_t flags = read8(Register::RTIF);
    uint8_t err = read8(Register::ERRINT);

    if (!flags && !err)
      return;

    if ((flags & EIF) || (err & EPIF) || (err & BEIF))
      onError(err);

    if (flags)
      set8(Register::RTIF, flags);
  }

  static void onError(uint8_t err) {
    if constexpr (!DebugMode)
      return;

    uint8_t cause = read8(Register::EALCAP) & 0xE0;
    uint8_t status = read8(Register::STATUS);

    const char *msg = "Unknown Error";

    switch (cause) {
    case 0x20:
      msg = "Bit Error";
      break;
    case 0x40:
      msg = "Form Error";
      break;
    case 0x60:
      msg = "Stuff Error";
      break;
    case 0x80:
      msg = "ACK Error";
      break;
    case 0xA0:
      msg = "CRC Error";
      break;
    }

    if (status & BUSOFF)
      msg = "Bus Off";

    Trace(msg);

    set8(Register::ERRINT, err);
  }

  void configure(size_t kbps, bool lbi, bool lbe) {
    for (const auto &cfg : TimingTable) {
      if (cfg.baudrate == kbps) {
        setBaudrate(cfg);
        break;
      }
    }

    if (lbi)
      set8(Register::STATUS, LBI);

    if (lbe) {
      set8(Register::STATUS, LBE);
      set8(Register::RCTRL, SACK);
    }
  }

  static void setBaudrate(const BitTiming &cfg) {
    const auto &b = cfg.t;

    uint32_t value = ((b[1] + b[2] - 1) << 0) | ((b[3] - 1) << 8) |
                     ((b[4] - 1) << 16) | ((b[5] - 1) << 24);

    reg32(Register::S_SEG_1) = value;
  }

  static uint8_t read8(Register reg) {
    uintptr_t addr = Base + static_cast<uintptr_t>(reg);
    uintptr_t aligned = addr & ~(4 - 1);
    uint8_t shift = (addr - aligned) * 8;
    return (reinterpret_cast<volatile uint32_t *>(aligned)[0] >> shift) & 0xFF;
  }

  static void write8(Register reg, uint8_t value) {
    uintptr_t addr = Base + static_cast<uintptr_t>(reg);
    uintptr_t aligned = addr & ~(4 - 1);
    uint8_t shift = (addr - aligned) * 8;
    auto *ptr = reinterpret_cast<volatile uint32_t *>(aligned);

    uint32_t tmp = *ptr;
    tmp &= ~(0xFFu << shift);
    tmp |= (static_cast<uint32_t>(value) << shift);

    *ptr = tmp;
  }

  static void set8(Register reg, uint8_t mask) {
    write8(reg, read8(reg) | mask);
  }

  static void clear8(Register reg, uint8_t mask) {
    write8(reg, read8(reg) & ~mask);
  }

  static volatile uint32_t &reg32(Register reg) {
    return *reinterpret_cast<volatile uint32_t *>(Base +
                                                  static_cast<uintptr_t>(reg));
  }
};

} // namespace QUARK
