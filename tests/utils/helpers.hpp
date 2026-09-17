#pragma once

#include <orderbook/book.hpp>

namespace orderbook::test {
inline Command add(OrderId id, Side side, Price price, Quantity quantity) {
    Command c{};
    c.orderId = id;
    c.side = side;
    c.price = price;
    c.quantity = quantity;
    c.type = CommandType::Add;
    return c;
}

inline Command cancel(OrderId id) {
    Command c{};
    c.orderId = id;
    c.type = CommandType::Cancel;
    return c;
}

inline Command reduce(OrderId id, Quantity quantity) {
    Command c{};
    c.orderId = id;
    c.quantity = quantity;
    c.type = CommandType::Reduce;
    return c;
}

inline Command replace(OrderId orderId, OrderId newOrderId, Price price,
                       Quantity quantity) {
    Command c{};
    c.orderId = orderId;
    c.newOrderId = newOrderId;
    c.price = price;
    c.quantity = quantity;
    c.type = CommandType::Replace;
    return c;
}

inline Command addFok(OrderId orderId, Side side, Price price,
                      Quantity quantity) {
    Command c{};
    c.timeInForce = TimeInForce::FOK;
    c.type = CommandType::Add;
    c.orderId = orderId;
    c.side = side;
    c.price = price;
    c.quantity = quantity;
    return c;
}

inline Command addIoc(OrderId id, Side side, Price price, Quantity quantity) {
    Command c{};
    c.orderId = id;
    c.side = side;
    c.price = price;
    c.quantity = quantity;
    c.timeInForce = TimeInForce::IOC;
    return c;
}
} // namespace orderbook::test