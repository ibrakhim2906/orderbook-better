#include <gtest/gtest.h>
#include <orderbook/book.hpp>
#include <utils/helpers.hpp>

using namespace orderbook;
using namespace orderbook::test;

TEST(Replace, LosesTimePriority) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 20000, 100), f);
    b.apply(add(2, Side::Buy, 20000, 110), f);
    b.apply(replace(1, 3, 20000, 120), f);
    b.apply(add(4, Side::Sell, 20000, 110), f);
    EXPECT_EQ(b.liveOrderCount(), 1u);
    EXPECT_EQ(f[0].restingId, 2u);

    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Replace, CrossingPriceMatches) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 19500, 100), f);
    b.apply(add(2, Side::Sell, 20000, 100), f);
    b.apply(replace(1, 3, 20100, 80), f);

    EXPECT_EQ(b.liveOrderCount(), 1u);
    EXPECT_EQ(f[0].aggressorId, 3);
    EXPECT_EQ(f[0].restingId, 2);
    EXPECT_EQ(f[0].price, 20000);
    EXPECT_EQ(b.bestAsk(), 20000);
    EXPECT_EQ(f[0].quantity, 80u);
    EXPECT_EQ(b.quantityAt(Side::Sell, 20000), 20u);
    EXPECT_FALSE(b.bestBid().has_value());

    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Replace, UnknownIdIsIgnored) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 20000, 100), f);
    b.apply(replace(99, 100, 20010, 50), f);

    EXPECT_EQ(b.liveOrderCount(), 1u);
    EXPECT_EQ(b.bestBid(), 20000);
    EXPECT_EQ(b.quantityAt(Side::Buy, 20000), 100u);
    EXPECT_TRUE(f.empty());
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Replace, DuplicateNewIdIsIgnored) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 20000, 100), f);
    b.apply(add(2, Side::Buy, 19990, 50), f);
    b.apply(replace(1, 2, 20010, 70), f); // newId 2 already live

    EXPECT_EQ(b.liveOrderCount(), 2u);
    EXPECT_EQ(b.bestBid(), 20000);
    EXPECT_EQ(b.quantityAt(Side::Buy, 19990), 50u);
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Replace, ChangesPrice) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 20000, 100), f);
    b.apply(replace(1, 2, 20010, 100), f);

    EXPECT_EQ(b.bestBid(), 20010);
    EXPECT_EQ(b.quantityAt(Side::Buy, 20000), 0u);
    EXPECT_EQ(b.quantityAt(Side::Buy, 20010), 100u);
    EXPECT_EQ(b.liveOrderCount(), 1u);
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Replace, ChangesQuantity) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 20000, 100), f);
    b.apply(replace(1, 2, 20000, 40), f);

    EXPECT_EQ(b.quantityAt(Side::Buy, 20000), 40u);
    EXPECT_EQ(b.liveOrderCount(), 1u);
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Replace, KeepsSideEvenIfCommandSideDiffers) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Sell, 20000, 100), f);

    Command c{};
    c.type = CommandType::Replace;
    c.orderId = 1;
    c.newOrderId = 2;
    c.price = 20010;
    c.quantity = 100;
    c.side = Side::Buy; // deliberately wrong
    b.apply(c, f);

    EXPECT_FALSE(b.bestBid().has_value()); // must NOT become a bid
    EXPECT_EQ(b.bestAsk(), 20010);
    EXPECT_EQ(b.checkInvariants(), "");
}