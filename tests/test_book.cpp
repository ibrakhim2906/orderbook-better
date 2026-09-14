#include <gtest/gtest.h>
#include <orderbook/book.hpp>
#include <vector>

TEST(Book, ConstructAndAcceptCommands) {
    orderbook::Book book;
    std::vector<orderbook::Fill> fills;
    orderbook::Command cmd{};
    cmd.type = orderbook::CommandType::Add;
    book.apply(cmd, fills);
    EXPECT_TRUE(fills.empty());
}


