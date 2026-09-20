#include <orderbook/itch.hpp>

#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <unordered_map>
#include <vector>

int main(int argc, char **argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: dump_itch <file> [locate]\n");
        return 1;
    }
    const uint16_t target =
        (argc > 2) ? static_cast<uint16_t>(std::atoi(argv[2])) : 7451;  // SPY

    std::FILE *f = std::fopen(argv[1], "rb");
    if (!f) {
        std::perror("fopen");
        return 1;
    }

    std::fseek(f, 0, SEEK_END);
    const long fileSize = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (fileSize <= 0) {
        std::fprintf(stderr, "empty or unseekable file\n");
        std::fclose(f);
        return 1;
    }

    std::printf("reading %.1f MB...\n", fileSize / 1048576.0);
    std::vector<uint8_t> buf(static_cast<std::size_t>(fileSize));
    const std::size_t n = std::fread(buf.data(), 1, buf.size(), f);
    std::fclose(f);
    buf.resize(n);

    orderbook::Command c;
    uint16_t locate = 0;

    // ---- pass 1: message counts per symbol
    orderbook::itch::Parser p1(buf.data(), buf.size());
    std::unordered_map<uint16_t, uint64_t> total, adds;
    uint64_t messages = 0;

    while (p1.next(c, locate)) {
        ++messages;
        ++total[locate];
        if (c.type == orderbook::CommandType::Add) ++adds[locate];
    }

    std::printf("%llu book messages, %zu symbols in directory\n\n",
                static_cast<unsigned long long>(messages), p1.symbols().size());

    std::vector<std::pair<uint16_t, uint64_t>> ranked(total.begin(), total.end());
    std::sort(ranked.begin(), ranked.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    const auto &syms = p1.symbols();
    std::printf("%-8s %-8s %12s %12s\n", "symbol", "locate", "messages", "adds");
    for (std::size_t i = 0; i < ranked.size() && i < 25; ++i) {
        const auto it = syms.find(ranked[i].first);
        std::printf("%-8s %-8u %12llu %12llu\n",
                    it != syms.end() ? it->second.c_str() : "?",
                    ranked[i].first,
                    static_cast<unsigned long long>(ranked[i].second),
                    static_cast<unsigned long long>(adds[ranked[i].first]));
    }

    // ---- pass 2: price distribution for one symbol
    orderbook::itch::Parser p2(buf.data(), buf.size());
    orderbook::Price lo = INT32_MAX, hi = INT32_MIN;
    uint64_t seen = 0;
    std::vector<orderbook::Price> prices;

    while (p2.next(c, locate)) {
        if (locate != target) continue;
        if (c.type != orderbook::CommandType::Add) continue;
        if (c.price < lo) lo = c.price;
        if (c.price > hi) hi = c.price;
        prices.push_back(c.price);
        ++seen;
    }

    const auto tit = p1.symbols().find(target);
    const char *tsym = (tit != p1.symbols().end()) ? tit->second.c_str() : "?";

    if (seen == 0) {
        std::printf("\nlocate %u (%s): no add orders\n", target, tsym);
        return 0;
    }

    std::sort(prices.begin(), prices.end());
    auto pct = [&](double q) {
        return prices[static_cast<std::size_t>(q * (prices.size() - 1))];
    };

    std::printf("\nlocate %u (%s): %llu adds\n", target, tsym,
                static_cast<unsigned long long>(seen));
    std::printf("  full range   %d .. %d   (%d ticks)\n", lo, hi, hi - lo + 1);
    std::printf("  p0.1 .. p99.9  %d .. %d   (%d ticks)\n",
                pct(0.001), pct(0.999), pct(0.999) - pct(0.001) + 1);
    std::printf("  p1   .. p99    %d .. %d   (%d ticks)\n",
                pct(0.01), pct(0.99), pct(0.99) - pct(0.01) + 1);
    std::printf("  median       %d\n", pct(0.5));
    return 0;
}