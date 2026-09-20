#include "orderbook/book.hpp"
#include <algorithm>
#include <optional>
#include <string>

namespace orderbook {

Book::Book(std::size_t capacity) {
    orders_.resize(capacity);
    for (std::size_t i = 0; i + 1 < capacity; ++i) {
        orders_[i].next = static_cast<OrderPoolIndex>(i + 1);
    }
    orders_[capacity - 1].next = kNullOrder;
    freeHead_ = 0;

    levels_.assign(2 * kPriceSlots, Level{0, kNullOrder, kNullOrder});
}

OrderPoolIndex Book::allocSlot() {
    if (freeHead_ == kNullOrder) return kNullOrder;
    OrderPoolIndex slot = freeHead_;
    freeHead_ = orders_[slot].next;
    return slot;
}

void Book::freeSlot(OrderPoolIndex slot) {
    orders_[slot].next = freeHead_;
    freeHead_ = slot;
}

LevelIndex Book::indexFor(Side side, Price price) const {
    if (price < kMinPrice || price > kMaxPrice) return kNullLevel;
    auto i = static_cast<LevelIndex>(price - kMinPrice);
    return (side == Side::Buy) ? i : i + static_cast<LevelIndex>(kPriceSlots);
}

Price Book::priceOf(LevelIndex idx) const {
    auto i = (idx >= kPriceSlots) ? idx - static_cast<LevelIndex>(kPriceSlots) : idx;
    return kMinPrice + static_cast<Price>(i);
}

void Book::apply(const Command &command, std::vector<Fill> &out) {
    switch (command.type) {

    case CommandType::Add: {
        if (command.quantity == 0) break;
        if (byId_.find(command.orderId) != kNullOrder) break;
        submit(command.orderId, command.side, command.price, command.quantity,
               command.timeInForce, out);
        break;
    }

    case CommandType::Cancel: {
        OrderPoolIndex slot = byId_.find(command.orderId);
        if (slot == kNullOrder) break;
        unlinkOrder(slot);
        break;
    }

    case CommandType::Execute:
        break;

    case CommandType::Reduce: {
        OrderPoolIndex slot = byId_.find(command.orderId);
        if (slot == kNullOrder) break;
        Order &order = orders_[slot];
        if (command.quantity >= order.remainingQuantity) {
            unlinkOrder(slot);
            break;
        }
        order.remainingQuantity -= command.quantity;
        levels_[order.levelIdx].totalQuantity -= command.quantity;
        break;
    }
    case CommandType::Replace: {
        OrderPoolIndex slot = byId_.find(command.orderId);
        if (slot == kNullOrder) break;
        if (command.quantity == 0) break;
        if (byId_.find(command.newOrderId) != kNullOrder) break;

        LevelIndex li = orders_[slot].levelIdx;
        Side side = (li >= kPriceSlots) ? Side::Sell : Side::Buy;

        unlinkOrder(slot);
        submit(command.newOrderId, side, command.price, command.quantity,
               TimeInForce::DAY, out);
        break;
    }
    }
}

std::optional<Price> Book::bestBid() const {
    if (bestBidIdx_ == kNullLevel) return std::nullopt;
    return priceOf(bestBidIdx_);
}

std::optional<Price> Book::bestAsk() const {
    if (bestAskIdx_ == kNullLevel) return std::nullopt;
    return priceOf(bestAskIdx_);
}

Quantity Book::quantityAt(Side side, Price price) const {
    LevelIndex li = indexFor(side, price);
    if (li == kNullLevel) return 0;
    return levels_[li].totalQuantity;
}

std::size_t Book::liveOrderCount() const { return byId_.size(); }

void Book::unlinkOrder(OrderPoolIndex slot) {
    Order &order = orders_[slot];
    LevelIndex li = order.levelIdx;
    Level &level = levels_[li];

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

    // level emptied: if it was the best on its side, find the next one
    if (level.head == kNullOrder) {
        const bool isBid = (li < kPriceSlots);
        if (isBid && li == bestBidIdx_) {
            bestBidIdx_ = kNullLevel;
            for (LevelIndex i = li; i-- > 0;) {          // scan down
                if (levels_[i].head != kNullOrder) { bestBidIdx_ = i; break; }
            }
        } else if (!isBid && li == bestAskIdx_) {
            bestAskIdx_ = kNullLevel;
            const auto end = static_cast<LevelIndex>(2 * kPriceSlots);
            for (LevelIndex i = li + 1; i < end; ++i) {  // scan up
                if (levels_[i].head != kNullOrder) { bestAskIdx_ = i; break; }
            }
        }
    }

    freeSlot(slot);
}

void Book::restOrder(OrderId id, Side side, Price price, Quantity quantity) {
    LevelIndex li = indexFor(side, price);
    if (li == kNullLevel) return;          // outside the supported price band

    OrderPoolIndex slot = allocSlot();
    if (slot == kNullOrder) return;        // pool exhausted

    orders_[slot] = Order{.orderId = id,
                          .remainingQuantity = quantity,
                          .next = kNullOrder,
                          .prev = kNullOrder,
                          .levelIdx = li};

    Level &level = levels_[li];
    if (level.head == kNullOrder) {
        level.head = slot;
        level.tail = slot;
    } else {
        orders_[level.tail].next = slot;
        orders_[slot].prev = level.tail;
        level.tail = slot;
    }

    level.totalQuantity += quantity;
    byId_.insert(id, slot);

    // maintain the cached best: higher index is better for bids, lower for asks
    if (side == Side::Buy) {
        if (bestBidIdx_ == kNullLevel || li > bestBidIdx_) bestBidIdx_ = li;
    } else {
        if (bestAskIdx_ == kNullLevel || li < bestAskIdx_) bestAskIdx_ = li;
    }
}

void Book::submit(OrderId orderId, Side side, Price price, Quantity quantity,
                  TimeInForce timeInForce, std::vector<Fill> &out) {
    if (timeInForce == TimeInForce::FOK &&
        availableAgainst(side, price, quantity) < quantity) {
        return;
    }

    Quantity remaining = quantity;

    while (remaining > 0 && crosses(side, price)) {
        LevelIndex levelIdx = (side == Side::Buy) ? bestAskIdx_ : bestBidIdx_;
        OrderPoolIndex restingIdx = levels_[levelIdx].head;
        Order &resting = orders_[restingIdx];

        Quantity traded = std::min(remaining, resting.remainingQuantity);

        out.push_back(Fill{.aggressorId = orderId,
                           .restingId = resting.orderId,
                           .price = priceOf(levelIdx),
                           .quantity = traded});

        remaining -= traded;
        if (traded == resting.remainingQuantity) {
            unlinkOrder(restingIdx);
        } else {
            resting.remainingQuantity -= traded;
            levels_[levelIdx].totalQuantity -= traded;
        }
    }

    if (remaining > 0 && timeInForce == TimeInForce::DAY) {
        restOrder(orderId, side, price, remaining);
    }
}

bool Book::crosses(Side incomingSide, Price incomingPrice) const {
    if (incomingSide == Side::Buy) {
        if (auto ask = bestAsk()) return incomingPrice >= ask.value();
    } else {
        if (auto bid = bestBid()) return incomingPrice <= bid.value();
    }
    return false;
}

Quantity Book::availableAgainst(Side side, Price price, Quantity needed) const {
    Quantity total = 0;
    const auto end = static_cast<LevelIndex>(2 * kPriceSlots);

    if (side == Side::Buy) {
        if (bestAskIdx_ == kNullLevel) return 0;
        for (LevelIndex i = bestAskIdx_; i < end; ++i) {
            if (priceOf(i) > price) break;          // no longer crossing
            total += levels_[i].totalQuantity;
            if (total >= needed) return total;
        }
    } else {
        if (bestBidIdx_ == kNullLevel) return 0;
        for (LevelIndex i = bestBidIdx_ + 1; i-- > 0;) {
            if (priceOf(i) < price) break;
            total += levels_[i].totalQuantity;
            if (total >= needed) return total;
        }
    }
    return total;
}

std::string Book::checkInvariants() const {
    std::size_t seen = 0;
    const auto end = static_cast<LevelIndex>(2 * kPriceSlots);

    for (LevelIndex li = 0; li < end; ++li) {
        const Level &level = levels_[li];
        const bool isBid = (li < kPriceSlots);
        const char *label = isBid ? "bid" : "ask";

        if (level.head == kNullOrder) {
            if (level.totalQuantity != 0) {
                return std::string(label) + " empty level has quantity at " +
                       std::to_string(priceOf(li));
            }
            if (level.tail != kNullOrder) {
                return std::string(label) + " empty level has tail at " +
                       std::to_string(priceOf(li));
            }
            continue;
        }

        Quantity total = 0;
        OrderPoolIndex prevSeen = kNullOrder;
        OrderPoolIndex cur = level.head;
        std::size_t steps = 0;

        while (cur != kNullOrder) {
            const Order &order = orders_[cur];

            OrderPoolIndex mapped = byId_.find(order.orderId);
            if (mapped == kNullOrder) {
                return std::string(label) + " order " +
                       std::to_string(order.orderId) + " in queue but not in byId_";
            }
            if (mapped != cur) {
                return std::string(label) + " byId_ points at slot " +
                       std::to_string(mapped) + " but order is at slot " +
                       std::to_string(cur);
            }

            if (order.prev != prevSeen) {
                return std::string(label) + " broken back-link at slot " +
                       std::to_string(cur);
            }
            if (order.levelIdx != li) {
                return std::string(label) + " order " +
                       std::to_string(order.orderId) + " has wrong levelIdx";
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
                       std::to_string(priceOf(li));
            }
        }

        if (prevSeen != level.tail) {
            return std::string(label) + " tail wrong at price " +
                   std::to_string(priceOf(li));
        }
        if (total != level.totalQuantity) {
            return std::string(label) + " total mismatch at price " +
                   std::to_string(priceOf(li)) + ": walked " +
                   std::to_string(total) + " cached " +
                   std::to_string(level.totalQuantity);
        }

        // the cached best must not be beaten by any occupied level
        if (isBid && (bestBidIdx_ == kNullLevel || li > bestBidIdx_)) {
            return "bestBidIdx_ stale: occupied level above it at " +
                   std::to_string(priceOf(li));
        }
        if (!isBid && (bestAskIdx_ == kNullLevel || li < bestAskIdx_)) {
            return "bestAskIdx_ stale: occupied level below it at " +
                   std::to_string(priceOf(li));
        }

        seen += steps;
    }

    if (seen != byId_.size()) {
        return "leaked orders: byId_ has " + std::to_string(byId_.size()) +
               " but queues hold " + std::to_string(seen);
    }

    if (bestBidIdx_ != kNullLevel && levels_[bestBidIdx_].head == kNullOrder) {
        return "bestBidIdx_ points at an empty level";
    }
    if (bestAskIdx_ != kNullLevel && levels_[bestAskIdx_].head == kNullOrder) {
        return "bestAskIdx_ points at an empty level";
    }

    if (bestBidIdx_ != kNullLevel && bestAskIdx_ != kNullLevel &&
        priceOf(bestBidIdx_) >= priceOf(bestAskIdx_)) {
        return "crossed book: bid " + std::to_string(priceOf(bestBidIdx_)) +
               " >= ask " + std::to_string(priceOf(bestAskIdx_));
    }

    return {};
}

} // namespace orderbook