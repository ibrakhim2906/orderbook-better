#include <gtest/gtest.h>
#include <orderbook/book.hpp>
#include <orderbook/generator.hpp>

using namespace orderbook;

TEST(Fuzz, InvariantHoldsUnderRandomCommands) {
    constexpr uint32_t nSeeds = 500;
    constexpr std::size_t nCommands = 5000;

    for (uint32_t seed = 0; seed < nSeeds; ++seed) {
        auto cmds = generateCommands(nCommands, seed);
        Book book;
        std::vector<Fill> f;

        for (std::size_t i = 0; i < cmds.size(); ++i) {
            book.apply(cmds[i], f);
            f.clear();

            ASSERT_EQ(book.checkInvariants(), "")
                << "seed=" << seed << " command=" << i
                << " type=" << static_cast<int>(cmds[i].type)
                << " id=" << cmds[i].orderId;

        }
    }
}