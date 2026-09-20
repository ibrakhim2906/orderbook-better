#pragma once

#include <orderbook/commands.hpp>
#include <orderbook/types.hpp>
#include <random>
#include <vector>

inline std::vector<orderbook::Command> generateCommands(std::size_t n,
                                                        uint32_t seed) {
    using namespace orderbook;
    std::mt19937 rng(seed);

    std::uniform_int_distribution<int> bidDist(32000, 32130);
    std::uniform_int_distribution<int> askDist(32140, 32270);
    std::uniform_int_distribution<int> qtyDist(1, 500);
    std::uniform_int_distribution<int> reduceDist(1, 100);
    std::uniform_int_distribution<int> opDist(0, 9999);
    std::uniform_int_distribution<int> sideDist(0, 1);
    std::uniform_int_distribution<int> crossDist(0, 99);

    std::vector<Command> cmds;
    cmds.reserve(n);
    std::vector<OrderId> live;
    OrderId nextId = 1;

    auto pick = [&]() {
        std::uniform_int_distribution<std::size_t> d(0, live.size() - 1);
        return d(rng);
    };

    for (std::size_t i = 0; i < n; ++i) {
        const int roll = opDist(rng);   // 0..9999

        // keep the live set near the real book depth
        const bool mustAdd = live.empty();
        const bool mustCancel = live.size() > 220;

        if (mustCancel || (!mustAdd && roll < 4790)) {          // 47.9% cancel
            const std::size_t k = pick();
            cmds.push_back(makeCancel(live[k]));
            live[k] = live.back();
            live.pop_back();

        } else if (!mustAdd && roll < 4970) {                   // 1.8% replace
            const std::size_t k = pick();
            const Side side = sideDist(rng) ? Side::Sell : Side::Buy;
            const Price price = (side == Side::Buy) ? bidDist(rng) : askDist(rng);
            cmds.push_back(makeReplace(live[k], nextId, price, qtyDist(rng)));
            live[k] = nextId;
            ++nextId;

        } else if (!mustAdd && roll < 5100) {                   // 1.3% execute
            cmds.push_back(makeExecute(live[pick()], qtyDist(rng)));

        } else if (!mustAdd && roll < 5102) {                   // 0.02% reduce
            cmds.push_back(makeReduce(live[pick()], reduceDist(rng)));

        } else {                                                // ~49% add
            const Side side = sideDist(rng) ? Side::Sell : Side::Buy;
            Price price;
            if (crossDist(rng) < 3) {                           // ~3% aggressive
                price = (side == Side::Buy) ? askDist(rng) : bidDist(rng);
            } else {
                price = (side == Side::Buy) ? bidDist(rng) : askDist(rng);
            }
            cmds.push_back(makeAdd(nextId, side, price, qtyDist(rng)));
            live.push_back(nextId);
            ++nextId;
        }
    }
    return cmds;
}