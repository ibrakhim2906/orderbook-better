tick-size: 0.01 in cents (1 converted to integer)

# Notes

## Environment

WSL2 Ubuntu 24.04, Intel, 16 logical / 8 physical cores (CPU 2k -> core k).
g++ 13.3.0, -O3 -march=native -g -DNDEBUG. Benchmarks: taskset -c 2.

PMU passthrough works: cycles, instructions, branches, cache-references,
cache-misses, L1-dcache-* all real.
`perf c2c` does NOT work -- "memory events not supported", no PEBS in the
guest. Phase 6.8 false-sharing analysis needs native Linux or ships
without mechanism evidence.
No cpufreq: /sys/devices/system/cpu/cpu0/cpufreq does not exist. Frequency
is controlled by the Windows power plan only. Google Benchmark reports
1996.2 MHz nominal; perf measured 3.207 GHz actual.

## Design decisions and why

- Order links are uint32 indices, not pointers. Pointers would make "the
  pool must never reallocate" a rule enforced only by memory; indices make
  a corrupt link assertable and let the pool move. Cost: one extra
  indirection, believed free on x86.
- Order is 24 bytes. Got there by questioning fields, not reordering --
  reordering cannot help when fields sum to 25 and alignment is 8.
  `side` packs into a bit; timestamp dropped because list position already
  encodes arrival order.
- Fill carries BOTH order ids (aggressor + resting). One record fully
  describes one trade; the fuzzer can compare sequences element by element.
- Replace always loses time priority (unlink + re-add at tail). ITCH `U`
  carries a NEW order reference, so the wire format says it's a
  replacement, not an edit. Venues differ -- this is a policy choice.
- Trade price is the RESTING order's price, never the aggressor's.
- Level stores `side` so unlinkOrder can erase the right map on empty.
- Empty levels are erased from bids_/asks_; the levels_ slot leaks
  (acceptable in Phase 1, Phase 3 addresses it).

## Bugs found, and how

- Preset bug: CMAKE_CXX_FLAGS is emitted BEFORE CMAKE_CXX_FLAGS_<CONFIG>,
  so `-O3 ... -O2` meant release actually built at -O2. Verified with
  `g++ -O3 -O2 -Q --help=optimizers` (-floop-interchange enabled vs
  disabled). Fix: set CMAKE_CXX_FLAGS_<CONFIG> instead.
- ASan did not fire on the first sanitizer check -- UBSan's object-size
  check pre-empted it because `new int[4]` has a compile-time-known size.
  Had to hide the size behind std::stoul to actually exercise ASan.
- Zero-page: a 256MB read-only malloc showed a 0.90% cache-miss ratio.
  Untouched pages map to the shared zero page, so the benchmark measured
  page faults, not memory bandwidth. memset first.
- Predicted 4.19M L1 misses, measured 16.5M -- forgot memset write-allocate
  and kernel page zeroing. sys time was 6x user. Use `:u` to scope.
- Order id vs pool slot index confused FOUR times (level.head, unlinkOrder
  x2, generator). Both are integers; the type system cannot tell them
  apart. A strong typedef would make these compile errors.
- The invariant checker caught a crossed-book test where bid/ask prices
  were transposed -- caught a bug in the TEST, first run.

## Measurement

Noise floor depends on what you measure:
BM_Nothing (no memory traffic):  cv 0.96%, spread 3.0%
BM_ApplyBatch (real workload):   cv 1.33%, spread 4.5%
The second is the one that counts. **Differences under 5% are not results.**

rdtsc overhead drifted +85% between runs (8.05 -> 14.9 ns) when other
benchmarks were present. Short benchmarks are frequency-sensitive; long
ones average it out.

Clock costs: steady_clock::now() 20.9 ns, rdtsc 8.05, rdtscp 16.2.
Per-op timing with two steady_clock reads would add 41.8 ns to an ~80 ns
operation and would systematically understate later improvements, since
the overhead is constant.

## Baseline (end of Phase 2)

Book with std::map levels + grow-only vector pool:
84.0 ns/cmd wall, 79.8 ns/cmd CPU, 12.5 M cmd/s
n=10, cv 1.33%, 95% CI +/- 0.76 ns
100k synthetic commands, seed 42: ~50% add, ~40% cancel, ~10% reduce,
price band 19990-20010 (21 ticks), qty 1-500
Verified not optimized away: objdump shows the apply call present.

## Known gaps / deliberate omissions

- Execute (ITCH 'E') is an empty case. Phase 5 needs it.
- Market orders not implemented -- an IOC at an extreme price, belongs in
  the adapter not the engine.
- No independent reference implementation. Correctness rests on ~45
  self-written tests plus the invariant checker. Phase 5's ITCH replay is
  the only external check, and it validates book maintenance, not matching.
- Tick size / price range (0.11) not yet decided. Phase 4 needs it.
- Generator assumptions unmeasured: what fraction of cancels hit live
  orders, what fraction of reduces become full cancels, mean book depth.