#include <benchmark/benchmark.h>

static void BM_Nothing(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(0);
    }
}
BENCHMARK(BM_Nothing);

BENCHMARK_MAIN();
