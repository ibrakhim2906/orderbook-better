#include "orderbook/generator.hpp"
#include "orderbook/book.hpp"
#include <benchmark/benchmark.h>
#include <x86intrin.h>

static void BM_RdtscOverhead(benchmark::State& state) {
    for (auto _ : state) {
        uint64_t t = __rdtsc();
        benchmark::DoNotOptimize(t);
    }
}
BENCHMARK(BM_RdtscOverhead);

static void BM_RdtscpOverhead(benchmark::State& state) {
    unsigned aux;
    for (auto _ : state) {
        uint64_t t = __rdtscp(&aux);
        benchmark::DoNotOptimize(t);
    }
}
BENCHMARK(BM_RdtscpOverhead);

static void BM_ApplyBatch(benchmark::State& state) {
    auto cmds = generateCommands(100000, 42);      // OUTSIDE the timed region

    for (auto _ : state) {
        state.PauseTiming();
        orderbook::Book book;                       // fresh book each pass
        std::vector<orderbook::Fill> fills;
        fills.reserve(1000);
        state.ResumeTiming();

        for (const auto& c : cmds) {
            book.apply(c, fills);
            fills.clear();
        }
        benchmark::DoNotOptimize(book);
    }
    state.SetItemsProcessed(state.iterations() * cmds.size());
}
BENCHMARK(BM_ApplyBatch);

BENCHMARK_MAIN();