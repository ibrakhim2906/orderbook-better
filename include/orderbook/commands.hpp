#pragma once
#include <orderbook/types.hpp>

namespace orderbook {

inline Command makeAdd(OrderId id, Side side, Price price, Quantity qty,
                       TimeInForce tif = TimeInForce::DAY) {
    Command c{};
    c.type = CommandType::Add;
    c.orderId = id;
    c.side = side;
    c.price = price;
    c.quantity = qty;
    c.timeInForce = tif;
    return c;
}

inline Command makeCancel(OrderId id) {
    Command c{};
    c.type = CommandType::Cancel;
    c.orderId = id;
    return c;
}

inline Command makeReduce(OrderId id, Quantity qty) {
    Command c{};
    c.type = CommandType::Reduce;
    c.orderId = id;
    c.quantity = qty;
    return c;
}

inline Command makeReplace(OrderId oldId, OrderId newId, Price price, Quantity qty) {
    Command c{};
    c.type = CommandType::Replace;
    c.orderId = oldId;
    c.newOrderId = newId;
    c.price = price;
    c.quantity = qty;
    return c;
}

}  // namespace orderbook