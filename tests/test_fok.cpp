#include <gtest/gtest.h>
#include <orderbook/book.hpp>
#include <utils/helpers.hpp>

using namespace orderbook;
using namespace orderbook::test;


TEST(FOK, ExactlyEnoughFillsCompletely) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Sell, 15000, 100), f);
    b.apply(addFok(2, Side::Buy, 15000, 100), f);

    ASSERT_EQ(f.size(), 1u);
    EXPECT_EQ(f[0].quantity, 100u);
    EXPECT_EQ(b.liveOrderCount(), 0u);
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(FOK, MoreThanEnoughLeavesRemainderResting) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Sell, 15000, 100), f);
    b.apply(addFok(2, Side::Buy, 15000, 60), f);

    ASSERT_EQ(f.size(), 1u);
    EXPECT_EQ(f[0].quantity, 60u);
    EXPECT_EQ(b.quantityAt(Side::Sell, 15000), 40u);
    EXPECT_EQ(b.liveOrderCount(), 1u);
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(FOK, EmptyBookDoesNothing) {
    Book b;
    std::vector<Fill> f;
    b.apply(addFok(1, Side::Buy, 15000, 100), f);

    EXPECT_TRUE(f.empty());
    EXPECT_EQ(b.liveOrderCount(), 0u);
    EXPECT_FALSE(b.bestBid().has_value());
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(FOK, SellSideWorksToo) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 15000, 100), f);
    b.apply(addFok(2, Side::Sell, 15000, 100), f);

    ASSERT_EQ(f.size(), 1u);
    EXPECT_EQ(b.liveOrderCount(), 0u);
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(FOK, InsufficientLiquidityNothingChanges) {
    Book b;
    std::vector<Fill> f;

    b.apply(add(1, Side::Sell, 12500, 100), f);
    b.apply(addFok(2, Side::Buy, 12500, 80), f);

    EXPECT_EQ(b.liveOrderCount(), 1u);
    EXPECT_EQ(b.bestAsk(), 12500);
    EXPECT_EQ(b.bestBid(), std::nullopt);

    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(FOK, ShortAcrossTwoLevelsChangesNothing) {
    Book b;
    std::vector<Fill> f;

    // 30+50=80 80<100 Fill or Kill should fail, no changes to the book itself
    b.apply(add(1, Side::Sell, 11000, 50), f);
    b.apply(add(2, Side::Sell, 10500, 30), f);
    b.apply(addFok(3, Side::Buy, 12000, 100), f);

    EXPECT_EQ(b.liveOrderCount(), 2u);
    EXPECT_EQ(f.size(), 0u);
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(FOK, LimitPriceBoundsAvailableLiquidity) {
    Book b;
    std::vector<Fill> f;

    // even though quantity fits into FOK order, limit price bound cancels out
    // operation book remains unchanged
    b.apply(add(1, Side::Sell, 10000, 50), f);
    b.apply(add(2, Side::Sell, 11000, 50), f);
    b.apply(add(3, Side::Sell, 12000, 50), f);
    b.apply(addFok(4, Side::Buy, 11999, 150), f);

    EXPECT_EQ(f.size(), 0u);
    EXPECT_EQ(b.liveOrderCount(), 3u);
    EXPECT_EQ(b.bestAsk(), 10000);

    EXPECT_EQ(b.checkInvariants(), "");
}
