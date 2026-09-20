#include <gtest/gtest.h>
#include <orderbook/itch.hpp>

using namespace orderbook::itch;

TEST(Itch, ByteReaders) {
    const uint8_t two[]   = {0x01, 0x2C};
    const uint8_t four[]  = {0x00, 0x00, 0x01, 0x2C};
    const uint8_t six[]   = {0x00, 0x00, 0x00, 0x00, 0x01, 0x2C};
    const uint8_t eight[] = {0,0,0,0,0,0,0x01,0x2C};
    EXPECT_EQ(be16(two), 300u);
    EXPECT_EQ(be32(four), 300u);
    EXPECT_EQ(be48(six), 300u);
    EXPECT_EQ(be64(eight), 300u);
}

TEST(Itch, HighBitsDoNotSignExtend) {
    const uint8_t v[] = {0xFF, 0xFF, 0xFF, 0xFF};
    EXPECT_EQ(be32(v), 4294967295u);      // not -1
}