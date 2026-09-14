
#include "orderbook/book.hpp"
#include <optional>

namespace orderbook {

void Book::apply(const Command &command, std::vector<Fill> &out) {
    (void)out;
    switch (command.type) {
    case CommandType::Add: {
        // reject malformed
        if (command.quantity == 0)
            return;
        if (byId_.contains(command.orderId))
            return;

        // add order
        auto slot = static_cast<OrderPoolIndex>(orders_.size());
        orders_.push_back(Order{.orderId = command.orderId,
                                .remainingQuantity = command.quantity,
                                .next = kNullOrder,
                                .prev = kNullOrder,
                                .levelIdx = kNullLevel});

        // get or create level
        LevelIndex idx = findOrCreateLevel(command.side, command.price);
        orders_[slot].levelIdx = idx;

        // link to the tail
        Level &level = levels_[idx];
        if (level.head == kNullOrder) {
            level.head = slot;
            level.tail = slot;
        } else {
            orders_[level.tail].next = slot;
            orders_[slot].prev = level.tail;
            level.tail = slot;
        }

        level.totalQuantity += command.quantity;
        byId_.emplace(command.orderId, slot);
        break;
    }
    case CommandType::Cancel:
        break;
    case CommandType::Execute:
        break;
    case CommandType::Reduce:
        break;
    case CommandType::Replace:
        break;
    }
}

std::optional<Price> Book::bestBid() const {
    if (bids_.empty())
        return std::nullopt;
    return bids_.begin()->first;
}

std::optional<Price> Book::bestAsk() const {
    if (asks_.empty())
        return std::nullopt;
    return asks_.begin()->first;
}

Quantity Book::quantityAt(Side side, Price price) const {

    if (side == Side::Buy) {
        auto it = bids_.find(price);
        if (it == bids_.end())
            return 0;
        return levels_[it->second].totalQuantity;
    } else {
        auto it = asks_.find(price);
        if (it == asks_.end())
            return 0;
        return levels_[it->second].totalQuantity;
    }
}

std::size_t Book::liveOrderCount() const { return byId_.size(); }

LevelIndex Book::findOrCreateLevel(Side side, Price price) {
    if (side == Side::Buy) {
        auto it = bids_.find(price);
        if (it != bids_.end())
            return it->second;
        levels_.push_back(Level{.price = price,
                                .totalQuantity = 0,
                                .head = kNullOrder,
                                .tail = kNullOrder});
        auto idx = static_cast<LevelIndex>(levels_.size() - 1);
        bids_.emplace(price, idx);
        return idx;
    } else {
        auto it = asks_.find(price);
        if (it != asks_.end())
            return it->second;
        levels_.push_back(Level{.price = price,
                                .totalQuantity = 0,
                                .head = kNullOrder,
                                .tail = kNullOrder});
        auto idx = static_cast<LevelIndex>(levels_.size() - 1);
        asks_.emplace(price, idx);
        return idx;
    }
}
} // namespace orderbook
