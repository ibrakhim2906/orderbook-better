#pragma once

#include <cstdint>
#include <map>
#include <unordered_map>
#include <vector>

#include <orderbook/types.hpp>

namespace orderbook {

    class Book {
    public:
        void apply(const Command& command, std::vector<Fill>& out);

    private:
        struct Level {
            Price price;
            Quantity quantity;
            OrderPoolIndex head;
            OrderPoolIndex tail;
        };

        std::vector<Order> orders_;
        std::vector<Level> levels_;

        std::unordered_map<OrderId, OrderPoolIndex> byId_;

        std::map<Price, LevelIndex, std::greater<Price>> bids_;
        std::map<Price, LevelIndex> asks_;
    };
}