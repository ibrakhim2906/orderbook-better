# Orderbook Matching Engine

A limit order book and matching engine in C++20, tested against a real
NASDAQ BX market data feed.

**~20 ns per command, ~50M commands/sec.**

## What it does

Maintains a live order book: orders arrive, rest at a price, match against the
other side when they cross, and leave when cancelled or filled. Supports limit,
IOC and fill-or-kill orders.

Everything goes through singular entry point, so tests, benchmarks and the real ITCH
feed all exercise the same code path.

## Build

```sh
./run.sh test     # sanitizers + 46 tests
./run.sh bench    # release build + benchmarks
```

Needs CMake 3.21+, C++20, and vcpkg

## Design

- **Price levels are a flat array**, indexed by price arithmetic — no tree, no
  hash lookup. Best bid and ask are cached indices.
- **Orders live in a pre-sized pool** linked by `uint32_t` indices rather than
  pointers, so the storage can move and a corrupt link can be asserted on.
- **`Order` is 24 bytes.** Time priority comes from position in the queue, so no
  timestamp is stored.
- **ID lookup is an open-addressed hash table** — flat array, linear probing, no
  allocation per entry.

## Correctness

46 tests, including a fuzzer that runs 250,000 random commands under
AddressSanitizer and verifies every invariant after each one: linked lists
intact, cached quantities matching the walked sums, no leaked orders, book never
crossed.

Then a full trading day of real data — 614,578 SPY messages from NASDAQ BX:

```
applied 614578 messages
61 invariant checks passed
final: 0 live orders
```

The book ends exactly empty, indicating every operation was accounted for.

## Performance

g++ 13.3, `-O3 -march=native`, single core, 100k commands per pass, n=10.

`std::map` price levels, a grow-only order vector and `std::unordered_map` for
ID lookup were replaced by a flat level array, an indexed pool and an
open-addressed table:

| | ns/cmd | cmd/s | instructions/cmd | IPC |
|---|---|---|---|---|
| standard containers | 51.6 | 19.4M | 367 | 1.45 |
| this implementation | **20.1** | **50.3M** | **230** | **1.98** |

**2.57x faster, 37% fewer instructions per command.**

(Individual contribution to performance of each change were not recorded)

Profiling put 23.5% of cycles in the hash table — lookups plus a heap
allocation on every insert and a free on every erase, on a structure touched by
every command. After the change no allocator symbols appear in the profile.

Noise floor is 1.4%, so anything under 2% isn't a result.


Data: `20191230.BX_ITCH_50.gz` from
[emi.nasdaq.com](https://emi.nasdaq.com/ITCH/Nasdaq%20BX%20ITCH/).