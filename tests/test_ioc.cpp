#include <gtest/gtest.h>
#include <orderbook/book.hpp>
#include <utils/helpers.hpp>

using namespace orderbook;
using namespace orderbook::test;

TEST(IOC, PartialFillRemainderDiscard) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 10000, 100), f);
    b.apply(addIoc(2, Side::Sell, 10000, 120), f);
    EXPECT_EQ(b.liveOrderCount(), 0u);
    EXPECT_EQ(b.bestAsk(), std::nullopt);
}

TEST(IOC, EmptyBookNothingRests) {
    Book b;
    std::vector<Fill> f;
    b.apply(addIoc(1, Side::Buy, 10000, 100), f);

    EXPECT_EQ(b.liveOrderCount(), 0u);
    EXPECT_EQ(f.size(), 0);
}

TEST(IOC, FullFillBehavesDay) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 10000, 100), f);
    b.apply(addIoc(2, Side::Sell, 10000, 100), f);

    EXPECT_EQ(b.liveOrderCount(), 0u);
    EXPECT_EQ(f[0].aggressorId, 2);
    EXPECT_EQ(f[0].restingId, 1);
}

TEST(IOC, MultipleLevelsSweeps) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 15000, 100), f);
    b.apply(add(2, Side::Buy, 15001, 100), f);
    b.apply(addIoc(3, Side::Sell, 15000, 200), f);

    EXPECT_EQ(b.liveOrderCount(), 0u);
    EXPECT_EQ(b.bestAsk(), std::nullopt);
    EXPECT_EQ(b.bestBid(), std::nullopt);

    EXPECT_EQ(b.checkInvariants(), "");
}
