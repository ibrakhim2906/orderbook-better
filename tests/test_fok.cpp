#include <gtest/gtest.h>
#include <orderbook/book.hpp>
#include <utils/helpers.hpp>

using namespace orderbook;
using namespace orderbook::test;


TEST(FOK, ExactlyEnoughFillsCompletely) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Sell, 20000, 100), f);
    b.apply(addFok(2, Side::Buy, 20000, 100), f);

    ASSERT_EQ(f.size(), 1u);
    EXPECT_EQ(f[0].quantity, 100u);
    EXPECT_EQ(b.liveOrderCount(), 0u);
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(FOK, MoreThanEnoughLeavesRemainderResting) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Sell, 20000, 100), f);
    b.apply(addFok(2, Side::Buy, 20000, 60), f);

    ASSERT_EQ(f.size(), 1u);
    EXPECT_EQ(f[0].quantity, 60u);
    EXPECT_EQ(b.quantityAt(Side::Sell, 20000), 40u);
    EXPECT_EQ(b.liveOrderCount(), 1u);
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(FOK, EmptyBookDoesNothing) {
    Book b;
    std::vector<Fill> f;
    b.apply(addFok(1, Side::Buy, 20000, 100), f);

    EXPECT_TRUE(f.empty());
    EXPECT_EQ(b.liveOrderCount(), 0u);
    EXPECT_FALSE(b.bestBid().has_value());
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(FOK, SellSideWorksToo) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 20000, 100), f);
    b.apply(addFok(2, Side::Sell, 20000, 100), f);

    ASSERT_EQ(f.size(), 1u);
    EXPECT_EQ(b.liveOrderCount(), 0u);
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(FOK, InsufficientLiquidityNothingChanges) {
    Book b;
    std::vector<Fill> f;

    b.apply(add(1, Side::Sell, 17500, 100), f);
    b.apply(addFok(2, Side::Buy, 17500, 80), f);

    EXPECT_EQ(b.liveOrderCount(), 1u);
    EXPECT_EQ(b.bestAsk(), 17500);
    EXPECT_EQ(b.bestBid(), std::nullopt);

    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(FOK, ShortAcrossTwoLevelsChangesNothing) {
    Book b;
    std::vector<Fill> f;

    // 30+50=80 80<100 Fill or Kill should fail, no changes to the book itself
    b.apply(add(1, Side::Sell, 16000, 50), f);
    b.apply(add(2, Side::Sell, 20500, 30), f);
    b.apply(addFok(3, Side::Buy, 17000, 100), f);

    EXPECT_EQ(b.liveOrderCount(), 2u);
    EXPECT_EQ(f.size(), 0u);
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(FOK, LimitPriceBoundsAvailableLiquidity) {
    Book b;
    std::vector<Fill> f;

    // 150 shares exist, but only 100 at prices the buyer will accept
    b.apply(add(1, Side::Sell, 20000, 50), f);
    b.apply(add(2, Side::Sell, 21000, 50), f);
    b.apply(add(3, Side::Sell, 22000, 50), f);
    b.apply(addFok(4, Side::Buy, 21999, 150), f);

    EXPECT_TRUE(f.empty());
    EXPECT_EQ(b.liveOrderCount(), 3u);
    EXPECT_EQ(b.bestAsk(), 20000);
    EXPECT_EQ(b.quantityAt(Side::Sell, 20000), 50u);
    EXPECT_EQ(b.quantityAt(Side::Sell, 21000), 50u);
    EXPECT_EQ(b.quantityAt(Side::Sell, 22000), 50u);
    EXPECT_EQ(b.checkInvariants(), "");
}
