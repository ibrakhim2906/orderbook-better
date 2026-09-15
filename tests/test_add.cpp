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

TEST(Add, RestsOnEmptyBook) {
    Book b; std::vector<Fill> f;
    b.apply(add(1,Side::Buy, 15025, 100), f);
    EXPECT_EQ(b.bestBid(), 15025);
    EXPECT_EQ(b.quantityAt(Side::Buy, 15025), 100u);
    EXPECT_EQ(b.liveOrderCount(), 1u);
    EXPECT_FALSE(b.bestAsk().has_value());

    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Add, TwoOrdersSamePrice) {
    Book b; std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 10050, 100),f);
    b.apply(add(2, Side::Buy, 10050, 50),f);
    EXPECT_EQ(b.bestBid(), 10050);
    EXPECT_EQ(b.quantityAt(Side::Buy, 10050), 150u);
    EXPECT_EQ(b.liveOrderCount(), 2u);

    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Add, WorseBidNoPriority) {
    Book b; std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 10320, 10), f);
    b.apply(add(2, Side::Buy, 10230, 10), f);
    EXPECT_EQ(b.liveOrderCount(), 2u);
    EXPECT_EQ(b.bestBid(), 10320);

    EXPECT_EQ(b.checkInvariants(), "");
}

// further implementation of matching logic can make test fail
TEST(Add, DistinctBidAndAsks) {
    Book b; std::vector<Fill> f;
    b.apply(add(1, Side::Buy,  10230, 10), f);
    b.apply(add(2, Side::Sell, 10320, 10), f);
    EXPECT_EQ(b.bestBid(), 10230);
    EXPECT_EQ(b.bestAsk(), 10320);

    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Add, ZeroQuantityIgnored) {
    Book b; std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 10320, 0), f);
    EXPECT_EQ(b.liveOrderCount(), 0u);

    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Add, DuplicateIdsIgnored) {
    Book b; std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 10320, 10), f);
    b.apply(add(1, Side::Buy, 10320, 10), f);
    EXPECT_EQ(b.liveOrderCount(), 1u);

    EXPECT_EQ(b.checkInvariants(), "");
}