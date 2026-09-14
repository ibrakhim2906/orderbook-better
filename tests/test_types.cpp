#include <gtest/gtest.h>
#include <orderbook/types.hpp>

using namespace orderbook;

TEST(Types, SizesAreStable) {
    EXPECT_EQ(sizeof(Command), 40u);
    EXPECT_EQ(sizeof(Order), 24u);
    EXPECT_EQ(sizeof(Fill), 24u);
}

TEST(Types, NullSentinelsDifferFromValidIndices) {
    EXPECT_NE(kNullLevel, 0u);
    EXPECT_NE(kNullOrder, 0u);
}