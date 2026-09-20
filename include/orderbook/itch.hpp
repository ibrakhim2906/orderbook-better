#pragma once
#include <orderbook/types.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>

namespace orderbook::itch {

// ITCH 5.0 fields are big-endian and land at unaligned offsets, so read them
// byte-wise. Casting a packed struct over the buffer would be UB on the
// misaligned 8-byte fields.

inline uint16_t be16(const uint8_t *p) {
    return static_cast<uint16_t>(p[0] << 8 | p[1]);
}

inline uint32_t be32(const uint8_t *p) {
    return static_cast<uint32_t>(p[0]) << 24 |
           static_cast<uint32_t>(p[1]) << 16 |
           static_cast<uint32_t>(p[2]) << 8 | static_cast<uint32_t>(p[3]);
}

inline uint64_t be48(const uint8_t *p) {
    return static_cast<uint64_t>(p[0]) << 40 |
           static_cast<uint64_t>(p[1]) << 32 |
           static_cast<uint64_t>(p[2]) << 24 |
           static_cast<uint64_t>(p[3]) << 16 |
           static_cast<uint64_t>(p[4]) << 8 | static_cast<uint64_t>(p[5]);
}

inline uint64_t be64(const uint8_t *p) {
    return static_cast<uint64_t>(p[0]) << 56 |
           static_cast<uint64_t>(p[1]) << 48 |
           static_cast<uint64_t>(p[2]) << 40 |
           static_cast<uint64_t>(p[3]) << 32 |
           static_cast<uint64_t>(p[4]) << 24 |
           static_cast<uint64_t>(p[5]) << 16 |
           static_cast<uint64_t>(p[6]) << 8 | static_cast<uint64_t>(p[7]);
}

class Parser {
  public:
    Parser(const uint8_t *data, std::size_t size)
        : cur_(data), end_(data + size) {}

    bool next(Command &out, uint16_t &locate) {
        while (cur_ + 2 <= end_) {
            const uint16_t len = be16(cur_);
            if (cur_ + 2 + len > end_) return false;   // truncated stream

            const uint8_t *m = cur_ + 2;
            cur_ += 2 + len;

            const uint16_t loc = be16(m + 1);
            out = Command{};
            out.timeStamp = be48(m + 5);

            switch (m[0]) {
            case 'R': {                                // stock directory
                std::string t(reinterpret_cast<const char *>(m + 11), 8);
                while (!t.empty() && t.back() == ' ') t.pop_back();
                symbols_[loc] = t;
                break;                                 // not a command
            }

            case 'A':                                  // add order
            case 'F':                                  // add order with MPID
                out.type     = CommandType::Add;
                out.orderId  = be64(m + 11);
                out.side     = (m[19] == 'B') ? Side::Buy : Side::Sell;
                out.quantity = be32(m + 20);
                out.price    = static_cast<Price>(be32(m + 32) / 100);
                locate = loc;
                return true;

            case 'D':                                  // delete -- full removal
                out.type    = CommandType::Cancel;
                out.orderId = be64(m + 11);
                locate = loc;
                return true;

            case 'X':                                  // cancel -- partial
                out.type     = CommandType::Reduce;
                out.orderId  = be64(m + 11);
                out.quantity = be32(m + 19);
                locate = loc;
                return true;

            case 'E':                                  // executed
            case 'C':                                  // executed with price
                out.type     = CommandType::Execute;
                out.orderId  = be64(m + 11);
                out.quantity = be32(m + 19);
                locate = loc;
                return true;

            case 'U':                                  // replace
                out.type       = CommandType::Replace;
                out.orderId    = be64(m + 11);
                out.newOrderId = be64(m + 19);
                out.quantity   = be32(m + 27);
                out.price      = static_cast<Price>(be32(m + 31) / 100);
                locate = loc;
                return true;

            default:
                break;
            }
        }
        return false;
    }

    const std::unordered_map<uint16_t, std::string> &symbols() const {
        return symbols_;
    }

  private:
    const uint8_t *cur_;
    const uint8_t *end_;
    std::unordered_map<uint16_t, std::string> symbols_;
};

} // namespace orderbook::itch