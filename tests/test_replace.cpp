#include <gtest/gtest.h>
#include <orderbook/book.hpp>

using namespace orderbook;

static Command add(OrderId id, Side side, Price price, Quantity quantity) {
    Command c{};
    c.orderId = id;
    c.side = side;
    c.price = price;
    c.quantity = quantity;
    c.type = CommandType::Add;
    return c;
}

/*
static Command cancel(OrderId id) {
    Command c{};
    c.orderId = id;
    c.type = CommandType::Cancel;
    return c;
}

static Command reduce(OrderId id, Quantity quantity) {
    Command c{};
    c.orderId = id;
    c.quantity = quantity;
    c.type = CommandType::Reduce;
    return c;
}
*/

static Command replace(OrderId orderId, OrderId newOrderId, Price price, Quantity quantity) {
    Command c{};
    c.orderId = orderId;
    c.newOrderId = newOrderId;
    c.price = price;
    c.quantity = quantity;
    c.type = CommandType::Replace;
    return c;
}

TEST(Replace, LosesTimePriority) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 15000, 100), f);
    b.apply(add(2, Side::Buy, 15000, 110), f);
    b.apply(replace(1, 3, 15000, 120), f);
    b.apply(add(4, Side::Sell, 15000, 110), f);
    EXPECT_EQ(b.liveOrderCount(), 1u);
    EXPECT_EQ(f[0].restingId, 2u);

    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Replace, CrossingPriceMatches) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 14500, 100), f);
    b.apply(add(2, Side::Sell, 15000, 100) ,f);
    b.apply(replace(1, 3, 15100, 80), f);

    EXPECT_EQ(b.liveOrderCount(), 1u);
    EXPECT_EQ(f[0].aggressorId, 3);
    EXPECT_EQ(f[0].restingId, 2);
    EXPECT_EQ(f[0].price, 15000);
    EXPECT_EQ(b.bestAsk(), 15000);
    EXPECT_EQ(f[0].quantity, 80u);
    EXPECT_EQ(b.quantityAt(Side::Sell, 15000), 20u);
    EXPECT_FALSE(b.bestBid().has_value());

    EXPECT_EQ(b.checkInvariants(), "");
}


TEST(Replace, UnknownIdIsIgnored) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 15000, 100), f);
    b.apply(replace(99, 100, 15010, 50), f);

    EXPECT_EQ(b.liveOrderCount(), 1u);
    EXPECT_EQ(b.bestBid(), 15000);
    EXPECT_EQ(b.quantityAt(Side::Buy, 15000), 100u);
    EXPECT_TRUE(f.empty());
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Replace, DuplicateNewIdIsIgnored) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 15000, 100), f);
    b.apply(add(2, Side::Buy, 14990, 50), f);
    b.apply(replace(1, 2, 15010, 70), f); // newId 2 already live

    EXPECT_EQ(b.liveOrderCount(), 2u);
    EXPECT_EQ(b.bestBid(), 15000);
    EXPECT_EQ(b.quantityAt(Side::Buy, 14990), 50u);
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Replace, ChangesPrice) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 15000, 100), f);
    b.apply(replace(1, 2, 15010, 100), f);

    EXPECT_EQ(b.bestBid(), 15010);
    EXPECT_EQ(b.quantityAt(Side::Buy, 15000), 0u);
    EXPECT_EQ(b.quantityAt(Side::Buy, 15010), 100u);
    EXPECT_EQ(b.liveOrderCount(), 1u);
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Replace, ChangesQuantity) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 15000, 100), f);
    b.apply(replace(1, 2, 15000, 40), f);

    EXPECT_EQ(b.quantityAt(Side::Buy, 15000), 40u);
    EXPECT_EQ(b.liveOrderCount(), 1u);
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Replace, KeepsSideEvenIfCommandSideDiffers) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Sell, 15000, 100), f);

    Command c{};
    c.type = CommandType::Replace;
    c.orderId = 1;
    c.newOrderId = 2;
    c.price = 15010;
    c.quantity = 100;
    c.side = Side::Buy; // deliberately wrong
    b.apply(c, f);

    EXPECT_FALSE(b.bestBid().has_value()); // must NOT become a bid
    EXPECT_EQ(b.bestAsk(), 15010);
    EXPECT_EQ(b.checkInvariants(), "");
}