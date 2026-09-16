
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

// static Command cancel(OrderId id) {
//     Command c{};
//     c.orderId = id;
//     c.type = CommandType::Cancel;
//     return c;
// }
//
// static Command reduce(OrderId id, Quantity quantity) {
//     Command c{};
//     c.orderId = id;
//     c.quantity = quantity;
//     c.type = CommandType::Reduce;
//     return c;
// }

TEST(Match, FullyMatchesSingleResting) {
    Book b;
    std::vector<Fill> f;

    b.apply(add(1, Side::Buy, 15000, 100), f);
    b.apply(add(2, Side::Sell, 15000, 100), f);

    ASSERT_EQ(f.size(), 1);
    EXPECT_EQ(f[0].aggressorId, 2u);
    EXPECT_EQ(f[0].restingId, 1u);
    EXPECT_EQ(f[0].price, 15000);
    EXPECT_EQ(f[0].quantity, 100u);

    EXPECT_EQ(b.liveOrderCount(), 0u);

    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Match, IncomingLargerRemainsRest) {
    Book b;
    std::vector<Fill> f;

    b.apply(add(1, Side::Buy, 14000, 100), f);
    b.apply(add(2, Side::Sell, 13500, 120), f);

    ASSERT_EQ(f.size(), 1u);
    EXPECT_EQ(f[0].price, 14000);
    EXPECT_EQ(f[0].quantity, 100u);
    EXPECT_EQ(b.bestAsk(), 13500);
    EXPECT_EQ(b.quantityAt(Side::Sell, 13500), 20u);
    EXPECT_FALSE(b.bestBid().has_value());

}

TEST(Match, IncomingSmallerRestingKeepsRemainder) {
    Book b; std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 14000, 100), f);
    b.apply(add(2, Side::Sell, 14000, 80), f);

    ASSERT_EQ(f.size(), 1u);
    EXPECT_EQ(f[0].quantity, 80u);
    EXPECT_EQ(b.bestBid(), 14000);
    EXPECT_EQ(b.quantityAt(Side::Buy, 14000), 20);
    EXPECT_EQ(b.liveOrderCount(), 1);
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Match, TradeHappensAtRestingPrice) {
    Book b; std::vector<Fill> f;
    b.apply(add(1, Side::Sell, 15028, 100), f);
    b.apply(add(2, Side::Buy,  15030, 100), f);

    ASSERT_EQ(f.size(), 1u);
    EXPECT_EQ(f[0].price, 15028);
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Match, SweepsMultipleLevels) {
    Book b; std::vector<Fill> f;
    b.apply(add(1, Side::Sell, 15000, 50), f);
    b.apply(add(2, Side::Sell, 15001, 50), f);
    b.apply(add(3, Side::Sell, 15002, 50), f);
    b.apply(add(4, Side::Buy,  15001, 120), f);

    ASSERT_EQ(f.size(), 2);
    EXPECT_EQ(f[0].price, 15000);
    EXPECT_EQ(f[1].price, 15001);
    EXPECT_EQ(b.bestAsk(), 15002);
    EXPECT_EQ(b.bestBid(), 15001);
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Match, OlderOrderFillsFirst) {
    Book b; std::vector<Fill> f;
    b.apply(add(1, Side::Sell, 15000, 50), f);
    b.apply(add(2, Side::Sell, 15000, 50), f);
    b.apply(add(3, Side::Buy,  15000, 50), f);

    ASSERT_EQ(f.size(), 1u);
    EXPECT_EQ(f[0].restingId, 1);
    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Match, NonCrossingOrderJustRests) {
    Book b; std::vector<Fill> f;
    b.apply(add(1, Side::Sell, 15030, 100), f);
    b.apply(add(2, Side::Buy,  15025, 100), f);

    EXPECT_TRUE(f.empty());
    EXPECT_EQ(b.liveOrderCount(), 2u);
    EXPECT_EQ(b.checkInvariants(), "");
}





