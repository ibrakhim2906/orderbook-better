#include <gtest/gtest.h>
#include <orderbook/book.hpp>
#include <utils/helpers.hpp>

using namespace orderbook;
using namespace orderbook::test;

TEST(Add, RestsOnEmptyBook) {
    Book b; std::vector<Fill> f;
    b.apply(add(1,Side::Buy, 20025, 100), f);
    EXPECT_EQ(b.bestBid(), 20025);
    EXPECT_EQ(b.quantityAt(Side::Buy, 20025), 100u);
    EXPECT_EQ(b.liveOrderCount(), 1u);
    EXPECT_FALSE(b.bestAsk().has_value());

    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Add, TwoOrdersSamePrice) {
    Book b; std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 20050, 100),f);
    b.apply(add(2, Side::Buy, 20050, 50),f);
    EXPECT_EQ(b.bestBid(), 20050);
    EXPECT_EQ(b.quantityAt(Side::Buy, 20050), 150u);
    EXPECT_EQ(b.liveOrderCount(), 2u);

    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Add, WorseBidNoPriority) {
    Book b; std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 20320, 10), f);
    b.apply(add(2, Side::Buy, 20230, 10), f);
    EXPECT_EQ(b.liveOrderCount(), 2u);
    EXPECT_EQ(b.bestBid(), 20320);

    EXPECT_EQ(b.checkInvariants(), "");
}

// further implementation of matching logic can make test fail
TEST(Add, DistinctBidAndAsks) {
    Book b; std::vector<Fill> f;
    b.apply(add(1, Side::Buy,  20230, 10), f);
    b.apply(add(2, Side::Sell, 20320, 10), f);
    EXPECT_EQ(b.bestBid(), 20230);
    EXPECT_EQ(b.bestAsk(), 20320);

    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Add, ZeroQuantityIgnored) {
    Book b; std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 20320, 0), f);
    EXPECT_EQ(b.liveOrderCount(), 0u);

    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Add, DuplicateIdsIgnored) {
    Book b; std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 20320, 10), f);
    b.apply(add(1, Side::Buy, 20320, 10), f);
    EXPECT_EQ(b.liveOrderCount(), 1u);

    EXPECT_EQ(b.checkInvariants(), "");
}