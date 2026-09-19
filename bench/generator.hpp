#pragma once

#include <orderbook/commands.hpp>
#include <orderbook/types.hpp>
#include <random>
#include <vector>

inline std::vector<orderbook::Command> generateCommands(std::size_t n,
                                                        uint32_t seed) {
    using namespace orderbook;
    std::mt19937 generator(seed);
    std::uniform_int_distribution<int> priceDist(19990, 20010);
    std::uniform_int_distribution<int> quantityDist(1, 500);
    std::uniform_int_distribution<int> opDist(0, 99);
    std::uniform_int_distribution<int> sideDist(0, 1);

    std::vector<Command> commands;
    commands.reserve(n);
    std::vector<OrderId> orderLive;
    OrderId id = 1;

    for (std::size_t i = 0; i < n; i++) {
        int roll = opDist(generator);

        if (roll < 50 || orderLive.empty()) {
            Price price = priceDist(generator);
            Quantity quantity = quantityDist(generator);
            Side side = (sideDist(generator)) ? Side::Buy : Side::Sell;
            commands.push_back(makeAdd(id, side, price, quantity));
            orderLive.push_back(id);
            id++;
        } else if (roll < 90) {
            std::uniform_int_distribution<std::size_t> pick(0, orderLive.size() - 1);
            std::size_t idx = pick(generator);
            OrderId orderId = orderLive[idx];
            commands.push_back(makeCancel(orderId));
            orderLive[idx] = orderLive.back();
            orderLive.pop_back();
        } else {
            std::uniform_int_distribution<std::size_t> pick(0, orderLive.size() - 1);
            std::size_t idx = pick(generator);
            std::uniform_int_distribution<int> toReduce(1, 100);
            commands.push_back(makeReduce(orderLive[idx], toReduce(generator)));
        }
    }

    return commands;
}