#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include <optional>
#include <orderbook/types.hpp>

namespace orderbook {

class Book {
  public:
    void apply(const Command &command, std::vector<Fill> &out);

    std::optional<Price> bestBid() const;
    std::optional<Price> bestAsk() const;
    Quantity quantityAt(Side side, Price price) const;
    std::size_t liveOrderCount() const;
    std::string checkInvariants() const;

  private:
    struct Level {
        Price price;
        Quantity totalQuantity;
        OrderPoolIndex head;
        OrderPoolIndex tail;
        Side side;
    };


    std::vector<Order> orders_;
    std::vector<Level> levels_;

    std::unordered_map<OrderId, OrderPoolIndex> byId_;

    std::map<Price, LevelIndex, std::greater<>> bids_;
    std::map<Price, LevelIndex> asks_;

    LevelIndex findOrCreateLevel(Side side, Price price);
    void unlinkOrder(OrderPoolIndex slot);
    void restOrder(OrderId id, Side side, Price price, Quantity quantity);
    void submit(OrderId id, Side side, Price price, Quantity quantity,
                TimeInForce timeInForce, std::vector<Fill> &out);
    bool crosses(Side incomingSide, Price incomingPrice) const;
    Quantity availableAgainst(Side side, Price price, Quantity needed) const;
};
} // namespace orderbook