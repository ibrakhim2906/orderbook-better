#include <orderbook/book.hpp>
#include <orderbook/itch.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

int main(int argc, char **argv) {
    if (argc < 2) {
        std::fprintf(stderr,
                     "usage: replay <file> [locate] [minPrice] [maxPrice]\n");
        return 1;
    }
    const uint16_t target =
        (argc > 2) ? static_cast<uint16_t>(std::atoi(argv[2])) : 7451;  // SPY
    const orderbook::Price lo = (argc > 3) ? std::atoi(argv[3]) : 31900;
    const orderbook::Price hi = (argc > 4) ? std::atoi(argv[4]) : 32500;

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

    orderbook::Book book(lo, hi);
    orderbook::itch::Parser p(buf.data(), buf.size());
    orderbook::Command c;
    std::vector<orderbook::Fill> fills;
    uint16_t locate = 0;

    uint64_t applied = 0, outOfBand = 0, checks = 0;
    uint64_t byType[5] = {0, 0, 0, 0, 0};
    std::size_t maxLive = 0;

    while (p.next(c, locate)) {
        if (locate != target) continue;

        ++byType[static_cast<int>(c.type)];

        if (c.type == orderbook::CommandType::Add && (c.price < lo || c.price > hi)) {
            ++outOfBand;
        }

        book.apply(c, fills);
        fills.clear();
        ++applied;

        maxLive = std::max(maxLive, book.liveOrderCount());

        if (applied % 10000 == 0) {
            const std::string err = book.checkInvariants();
            if (!err.empty()) {
                std::printf("INVARIANT FAILED after %llu messages: %s\n",
                            static_cast<unsigned long long>(applied), err.c_str());
                return 1;
            }
            ++checks;
        }
    }

    const auto it = p.symbols().find(target);
    const char *sym = (it != p.symbols().end()) ? it->second.c_str() : "?";

    std::printf("\nlocate %u (%s), price band %d..%d\n", target, sym, lo, hi);
    std::printf("applied %llu messages, %llu out-of-band adds (%.4f%%)\n",
                static_cast<unsigned long long>(applied),
                static_cast<unsigned long long>(outOfBand),
                applied ? 100.0 * static_cast<double>(outOfBand) /
                              static_cast<double>(applied)
                        : 0.0);
    std::printf("%llu invariant checks passed\n",
                static_cast<unsigned long long>(checks));

    static const char *kName[] = {"add", "cancel", "reduce", "execute", "replace"};
    std::printf("\nmessage mix:\n");
    for (int i = 0; i < 5; ++i) {
        std::printf("  %-8s %9llu  %5.1f%%\n", kName[i],
                    static_cast<unsigned long long>(byType[i]),
                    applied ? 100.0 * static_cast<double>(byType[i]) /
                                  static_cast<double>(applied)
                            : 0.0);
    }

    std::printf("\npeak live orders: %zu\n", maxLive);
    std::printf("final: %zu live orders, best bid %d, best ask %d\n",
                book.liveOrderCount(), book.bestBid().value_or(-1),
                book.bestAsk().value_or(-1));
    return 0;
}