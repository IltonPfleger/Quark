namespace QUARK {

class PMU {
  public:
    static inline uint64_t cycle() {
        uint64_t value;
        asm volatile("rdcycle %0" : "=r"(value));
        return value;
    }

    static inline uint64_t instret() {
        uint64_t value;
        asm volatile("rdinstret %0" : "=r"(value));
        return value;
    }
};

} // namespace QUARK
