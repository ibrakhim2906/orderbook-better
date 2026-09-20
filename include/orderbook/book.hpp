#pragma once

#include <optional>
#include <string>
#include <vector>

#include <orderbook/idmap.hpp>
#include <orderbook/types.hpp>

namespace orderbook {

class Book {
public:
    explicit Book(std::size_t capacity = 16384);

    void apply(const Command &command, std::vector<Fill> &out);

    std::optional<Price> bestBid() const;
    std::optional<Price> bestAsk() const;
    Quantity quantityAt(Side side, Price price) const;
    std::size_t liveOrderCount() const;
    std::string checkInvariants() const;

private:
    struct Level {
        Quantity       totalQuantity;
        OrderPoolIndex head;
        OrderPoolIndex tail;
    };

    // 2 * kPriceSlots entries: bids occupy [0, N), asks [N, 2N).
    // The index encodes both price and side, so Order needs only one field.
    std::vector<Level> levels_;

    LevelIndex bestBidIdx_ = kNullLevel;
    LevelIndex bestAskIdx_ = kNullLevel;

    std::vector<Order> orders_;        // pre-sized pool
    OrderPoolIndex     freeHead_ = kNullOrder;

    IdMap byId_;                       // OrderId -> pool slot

    OrderPoolIndex allocSlot();
    void freeSlot(OrderPoolIndex slot);

    LevelIndex indexFor(Side side, Price price) const;
    Price priceOf(LevelIndex idx) const;
    void unlinkOrder(OrderPoolIndex slot);
    void restOrder(OrderId id, Side side, Price price, Quantity quantity);
    void submit(OrderId id, Side side, Price price, Quantity quantity,
                TimeInForce timeInForce, std::vector<Fill> &out);
    bool crosses(Side incomingSide, Price incomingPrice) const;
    Quantity availableAgainst(Side side, Price price, Quantity needed) const;
};

} // namespace orderbook