
#include "orderbook/book.hpp"
#include <algorithm>
#include <optional>
#include <string>
#include <string_view>

namespace orderbook {

void Book::apply(const Command &command, std::vector<Fill> &out) {
    switch (command.type) {
    case CommandType::Add: {
        // reject malformed
        if (command.quantity == 0)
            break;
        if (byId_.contains(command.orderId))
            break;

        submit(command.orderId, command.side, command.price, command.quantity, out);

        break;
    }
    case CommandType::Cancel: {
        auto it = byId_.find(command.orderId);
        if (it == byId_.end())
            return;
        unlinkOrder(it->second);
        break;
    }
    case CommandType::Execute:
        break;
    case CommandType::Reduce: {
        auto it = byId_.find(command.orderId);
        if (it == byId_.end())
            return;
        Order &order = orders_[it->second];
        if (command.quantity >= order.remainingQuantity) {
            unlinkOrder(it->second);
            break;
        }
        order.remainingQuantity -= command.quantity;
        levels_[order.levelIdx].totalQuantity -= command.quantity;
        break;
    }

    case CommandType::Replace: {
        auto it = byId_.find(command.orderId);
        if (it == byId_.end()) break;
        if (command.quantity==0) break;
        if (byId_.contains(command.newOrderId)) break;

        OrderPoolIndex slot{it->second};
        Side side = levels_[orders_[slot].levelIdx].side;
        unlinkOrder(slot);
        submit(command.newOrderId, side, command.price, command.quantity, out);
        break;
    }
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
                                .tail = kNullOrder,
                                .side = side});
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
                                .tail = kNullOrder,
                                .side = side});

        auto idx = static_cast<LevelIndex>(levels_.size() - 1);
        asks_.emplace(price, idx);
        return idx;
    }
}

void Book::unlinkOrder(OrderPoolIndex slot) {
    Order &order = orders_[slot];
    Level &level = levels_[order.levelIdx];

    if (order.prev != kNullOrder) {
        orders_[order.prev].next = order.next;
    } else {
        level.head = order.next;
    }

    if (order.next != kNullOrder) {
        orders_[order.next].prev = order.prev;
    } else {
        level.tail = order.prev;
    }

    level.totalQuantity -= order.remainingQuantity;
    byId_.erase(order.orderId);

    if (level.head == kNullOrder) {
        if (level.side == Side::Buy) {
            bids_.erase(level.price);
        } else {
            asks_.erase(level.price);
        }
    }
}

void Book::restOrder(OrderId id, Side side, Price price, Quantity quantity) {
    // add order
    auto slot = static_cast<OrderPoolIndex>(orders_.size());
    orders_.push_back(Order{.orderId = id,
                            .remainingQuantity = quantity,
                            .next = kNullOrder,
                            .prev = kNullOrder,
                            .levelIdx = kNullLevel});

    // get or create level
    LevelIndex idx = findOrCreateLevel(side, price);
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

    level.totalQuantity += quantity;
    byId_.emplace(id, slot);
}

void Book::submit(OrderId orderId, Side side, Price price, Quantity quantity, std::vector<Fill>& out) {
    Quantity remaining = quantity;

    while (remaining > 0 && crosses(side, price)) {
        LevelIndex levelIdx = (side == Side::Buy) ?
                        asks_.begin()->second :
                        bids_.begin()->second;

        OrderPoolIndex restingIdx = levels_[levelIdx].head;
        Order& resting = orders_[restingIdx];

        Quantity traded = std::min(remaining, resting.remainingQuantity);

        out.push_back(Fill{
            .aggressorId = orderId,
            .restingId = resting.orderId,
            .price = levels_[resting.levelIdx].price,
            .quantity = traded
        });

        remaining -= traded;
        if (traded == resting.remainingQuantity) {
            unlinkOrder(restingIdx);
        } else {
            resting.remainingQuantity -= traded;
            levels_[levelIdx].totalQuantity -= traded;
        }
    }

    if (remaining > 0) {
        restOrder(orderId, side, price,
                  remaining);
    }

}

std::string Book::checkInvariants() const {
    std::size_t seen = 0;

    auto checkSide = [&](const auto &mp,
                         std::string_view label) -> std::string {
        for (const auto &[price, levelIdx] : mp) {
            const Level &level = levels_[levelIdx];

            if (level.price != price) {
                return std::string(label) + " level price mismatch at " +
                       std::to_string(price);
            }
            if (level.side != (label == "bid" ? Side::Buy : Side::Sell)) {
                return std::string(label) + " level side mismatch at " +
                       std::to_string(price);
            }
            if (level.head == kNullOrder) {
                return std::string(label) + " empty level left in map at " +
                       std::to_string(price);
            }

            Quantity total = 0;
            OrderPoolIndex prevSeen = kNullOrder;
            OrderPoolIndex cur = level.head;
            std::size_t steps = 0;

            while (cur != kNullOrder) {
                const Order &order = orders_[cur];

                auto it = byId_.find(order.orderId);
                if (it == byId_.end()) {
                    return std::string(label) + " order " +
                           std::to_string(order.orderId) +
                           " in queue but not in byId_";
                }
                if (it->second != cur) {
                    return std::string(label) + " byId_ points at slot " +
                           std::to_string(it->second) +
                           " but order is at slot " + std::to_string(cur);
                }
                if (order.prev != prevSeen) {
                    return std::string(label) + " broken back-link at slot " +
                           std::to_string(cur);
                }
                if (order.levelIdx != levelIdx) {
                    return std::string(label) + " order " +
                           std::to_string(order.orderId) +
                           " has wrong levelIdx";
                }
                if (order.remainingQuantity == 0) {
                    return std::string(label) + " zero-quantity order " +
                           std::to_string(order.orderId) + " still resting";
                }

                total += order.remainingQuantity;
                prevSeen = cur;
                cur = order.next;

                if (++steps > byId_.size()) {
                    return std::string(label) + " cycle at price " +
                           std::to_string(price);
                }
            }

            if (prevSeen != level.tail) {
                return std::string(label) + " tail wrong at price " +
                       std::to_string(price);
            }
            if (total != level.totalQuantity) {
                return std::string(label) + " total mismatch at price " +
                       std::to_string(price) + ": walked " +
                       std::to_string(total) + " cached " +
                       std::to_string(level.totalQuantity);
            }

            seen += steps;
        }
        return {};
    };

    if (auto err = checkSide(bids_, "bid"); !err.empty())
        return err;
    if (auto err = checkSide(asks_, "ask"); !err.empty())
        return err;

    if (seen != byId_.size()) {
        return "leaked orders: byId_ has " + std::to_string(byId_.size()) +
               " but queues hold " + std::to_string(seen);
    }

    // book must never be crossed
    if (!bids_.empty() && !asks_.empty() &&
        bids_.begin()->first >= asks_.begin()->first) {
        return "crossed book: bid " + std::to_string(bids_.begin()->first) +
               " >= ask " + std::to_string(asks_.begin()->first);
    }

    return {};
}

bool Book::crosses(Side incomingSide, Price incomingPrice) const {

    if (incomingSide == Side::Buy) {
        if (auto ask = bestAsk()) {
            return incomingPrice >= ask.value();
        }
    } else {
        if (auto bid = bestBid()) {
            return incomingPrice <= bid.value();
        }
    }

    return false;
}

} // namespace orderbook
