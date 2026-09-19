#pragma once

#include <orderbook/commands.hpp>
#include <orderbook/types.hpp>
#include <random>
#include <vector>

inline std::vector<orderbook::Command> generateCommands(std::size_t n,
                                                        uint32_t seed) {
    using namespace orderbook;
    std::mt19937 generator(seed);

    std::uniform_int_distribution<int> bidDist(19950, 19999);
    std::uniform_int_distribution<int> askDist(20001, 20050);
    std::uniform_int_distribution<int> quantityDist(1, 500);
    std::uniform_int_distribution<int> reduceDist(1, 100);
    std::uniform_int_distribution<int> opDist(0, 99);
    std::uniform_int_distribution<int> sideDist(0, 1);
    std::uniform_int_distribution<int> crossDist(0, 99);

    std::vector<Command> commands;
    commands.reserve(n);
    std::vector<OrderId> orderLive;
    OrderId id = 1;

    for (std::size_t i = 0; i < n; ++i) {
        const int roll = opDist(generator);

        if (roll < 50 || orderLive.empty()) {
            const Quantity quantity = quantityDist(generator);
            const Side side = sideDist(generator) ? Side::Buy : Side::Sell;

            // ~3% of adds price across the spread and will match
            Price price;
            if (crossDist(generator) < 3) {
                price = (side == Side::Buy) ? askDist(generator)
                                            : bidDist(generator);
            } else {
                price = (side == Side::Buy) ? bidDist(generator)
                                            : askDist(generator);
            }

            commands.push_back(makeAdd(id, side, price, quantity));
            orderLive.push_back(id);
            ++id;

        } else if (roll < 90) {
            std::uniform_int_distribution<std::size_t> pick(0, orderLive.size() - 1);
            const std::size_t idx = pick(generator);
            commands.push_back(makeCancel(orderLive[idx]));
            orderLive[idx] = orderLive.back();
            orderLive.pop_back();

        } else {
            std::uniform_int_distribution<std::size_t> pick(0, orderLive.size() - 1);
            const std::size_t idx = pick(generator);
            commands.push_back(makeReduce(orderLive[idx], reduceDist(generator)));
        }
    }

    return commands;
}