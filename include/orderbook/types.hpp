#pragma once
#include <cstdint>
#include <type_traits>

namespace orderbook {
enum class Side : uint8_t { Buy, Sell };

using Price = int32_t;
using Quantity = uint32_t;
using OrderId = uint64_t;
using TimeStamp = uint64_t;
using OrderPoolIndex = uint32_t;
using LevelIndex = uint32_t;

inline constexpr LevelIndex kNullLevel = 0xFFFFFFFFu;
inline constexpr OrderPoolIndex kNullOrder = 0xFFFFFFFFu;

enum class CommandType : uint8_t {
    Add,
    Cancel, // full removal (ITCH 'D')
    Reduce, // partial cancel (ITCH 'X')
    Execute,
    Replace,
};

enum class TimeInForce : uint8_t {
    DAY,
    IOC,
    FOK,
};

struct Command {
    OrderId orderId;
    OrderId newOrderId;
    TimeStamp timeStamp;
    Price price;
    Quantity quantity;
    CommandType type;
    Side side;
    TimeInForce timeInForce;
};

struct Order {
    OrderId orderId;
    Quantity remainingQuantity;
    OrderPoolIndex next;
    OrderPoolIndex prev;
    LevelIndex levelIdx;
};

struct Fill {
    OrderId aggressorId;
    OrderId restingId;
    Price price;
    Quantity quantity;
};

static_assert(sizeof(Order) == 24, "Order must stay 24 bytes");
static_assert(sizeof(Command) == 40, "Command layout changed");
static_assert(sizeof(Fill) == 24, "Fill layout changes");
static_assert(std::is_trivially_copyable_v<Order>);
static_assert(std::is_trivially_copyable_v<Command>);
static_assert(std::is_trivially_copyable_v<Fill>);

} // namespace orderbook
