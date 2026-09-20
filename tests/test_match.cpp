
#include <gtest/gtest.h>
#include <orderbook/book.hpp>
#include <utils/helpers.hpp>

using namespace orderbook;
using namespace orderbook::test;

TEST(Match, FullyMatchesSingleResting) {
    Book b;
    std::vector<Fill> f;

    b.apply(add(1, Side::Buy, 20000, 100), f);
    b.apply(add(2, Side::Sell, 20000, 100), f);

    ASSERT_EQ(f.size(), 1);
    EXPECT_EQ(f[0].aggressorId, 2u);
    EXPECT_EQ(f[0].restingId, 1u);
    EXPECT_EQ(f[0].price, 20000);
    EXPECT_EQ(f[0].quantity, 100u);

    EXPECT_EQ(b.liveOrderCount(), 0u);

    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Match, IncomingLargerRemainsRest) {
    Book b;
    std::vector<Fill> f;

    b.apply(add(1, Side::Buy, 19000, 100), f);
    b.apply(add(2, Side::Sell, 18500, 120), f);

    ASSERT_EQ(f.size(), 1u);
    EXPECT_EQ(f[0].price, 19000);
    EXPECT_EQ(f[0].quantity, 100u);
    EXPECT_EQ(b.bestAsk(), 18500);
    EXPECT_EQ(b.quantityAt(Side::Sell, 18500), 20u);
    EXPECT_FALSE(b.bestBid().has_value());

}

TEST(Match, IncomingSmallerRestingKeepsRemainder) {
    Book b; std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 19000, 100), f);
    b.apply(add(2, Side::Sell, 19000, 80), f);

    ASSERT_EQ(f.size(), 1u);
    EXPECT_EQ(f[0].quantity, 80u);
    EXPECT_EQ(b.bestBid(), 19000);
    EXPECT_EQ(b.quantityAt(Side::Buy, 19000), 20);
    EXPECT_EQ(b.liveOrderCount(), 1);
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Match, TradeHappensAtRestingPrice) {
    Book b; std::vector<Fill> f;
    b.apply(add(1, Side::Sell, 20028, 100), f);
    b.apply(add(2, Side::Buy,  20030, 100), f);

    ASSERT_EQ(f.size(), 1u);
    EXPECT_EQ(f[0].price, 20028);
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Match, SweepsMultipleLevels) {
    Book b; std::vector<Fill> f;
    b.apply(add(1, Side::Sell, 20000, 50), f);
    b.apply(add(2, Side::Sell, 20001, 50), f);
    b.apply(add(3, Side::Sell, 20002, 50), f);
    b.apply(add(4, Side::Buy,  20001, 120), f);

    ASSERT_EQ(f.size(), 2);
    EXPECT_EQ(f[0].price, 20000);
    EXPECT_EQ(f[1].price, 20001);
    EXPECT_EQ(b.bestAsk(), 20002);
    EXPECT_EQ(b.bestBid(), 20001);
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Match, OlderOrderFillsFirst) {
    Book b; std::vector<Fill> f;
    b.apply(add(1, Side::Sell, 20000, 50), f);
    b.apply(add(2, Side::Sell, 20000, 50), f);
    b.apply(add(3, Side::Buy,  20000, 50), f);

    ASSERT_EQ(f.size(), 1u);
    EXPECT_EQ(f[0].restingId, 1);
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Match, NonCrossingOrderJustRests) {
    Book b; std::vector<Fill> f;
    b.apply(add(1, Side::Sell, 20030, 100), f);
    b.apply(add(2, Side::Buy,  20025, 100), f);

    EXPECT_TRUE(f.empty());
    EXPECT_EQ(b.liveOrderCount(), 2u);
    EXPECT_EQ(b.checkInvariants(), "");
}





