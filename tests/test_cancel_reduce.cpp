#include <gtest/gtest.h>
#include <orderbook/book.hpp>
#include <utils/helpers.hpp>

using namespace orderbook;
using namespace orderbook::test;

TEST(Cancel, MiddleOfThreeKeepListIntact) {

    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 15025, 100), f);
    b.apply(add(2, Side::Buy, 15025, 300), f);
    b.apply(add(3, Side::Buy, 15025, 400), f);
    b.apply(cancel(2), f);
    EXPECT_EQ(b.quantityAt(Side::Buy, 15025), 500);
    EXPECT_EQ(b.liveOrderCount(), 2u);

    EXPECT_EQ(b.checkInvariants(), "");
}

// TODO: create invariant check function to really assess linked list correct
// logic
TEST(Cancel, HeadOrderIsMovedWhenCancelled) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 15000, 100), f);
    b.apply(add(2, Side::Buy, 15000, 300), f);
    b.apply(cancel(1), f);
    EXPECT_EQ(b.quantityAt(Side::Buy, 15000), 300u);
    EXPECT_EQ(b.liveOrderCount(), 1u);

    EXPECT_EQ(b.checkInvariants(), "");
}

// TODO: same as HeadOrderIsBeingMoved test
TEST(Cancel, TailOrderIsMovedWhenCancelled) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 15000, 100), f);
    b.apply(add(2, Side::Buy, 15000, 300), f);
    b.apply(cancel(2), f);
    EXPECT_EQ(b.quantityAt(Side::Buy, 15000), 100u);
    EXPECT_EQ(b.liveOrderCount(), 1u);

    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Cancel, BestOfferDisappearsWithLastOrderRemoved) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 15000, 100), f);
    EXPECT_EQ(b.bestBid(), 15000);
    b.apply(cancel(1), f);
    EXPECT_EQ(b.bestBid(), std::nullopt);

    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Cancel, CancelUnknownId) {
    Book b;
    std::vector<Fill> f;
    b.apply(add(1, Side::Buy, 15000, 100), f);
    b.apply(cancel(2), f); // Order with ID 2 does not exist,
                           // for now no exceptions would be thrown and command
                           // will be simple ignored
    EXPECT_EQ(b.liveOrderCount(), 1u);

    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Reduce, PartiallyReducing) {
    Book b;
    std::vector<Fill> f;

    b.apply(add(1, Side::Buy, 15000, 100), f);
    b.apply(reduce(1, 50), f);
    EXPECT_EQ(b.quantityAt(Side::Buy, 15000), 50u);

    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Reduce, ExactQuantityReduced) {
    Book b;
    std::vector<Fill> f;

    b.apply(add(1, Side::Sell, 6700, 100), f);
    b.apply(reduce(1, 100), f);

    EXPECT_EQ(b.liveOrderCount(), 0);
    EXPECT_EQ(b.bestBid(), std::nullopt);

    EXPECT_EQ(b.checkInvariants(), "");
}

TEST(Reduce, LargerThanQuantityReduced) {
    Book b;
    std::vector<Fill> f;

    b.apply(add(1, Side::Sell, 6000, 50), f);

    b.apply(reduce(1, 100), f);

    EXPECT_EQ(b.liveOrderCount(), 0);
    EXPECT_EQ(b.bestBid(), std::nullopt);

    EXPECT_EQ(b.checkInvariants(), "");
}
