#include <gtest/gtest.h>
#include <orderbook/book.hpp>
#include <vector>

using namespace orderbook;

TEST(Book, ConstructAndAcceptCommands) {
    Book book;
    std::vector<Fill> fills;
    Command cmd{};
    cmd.type = CommandType::Add;
    book.apply(cmd, fills);
    EXPECT_TRUE(fills.empty());

    EXPECT_EQ(book.checkInvariants(), "");
}


