#pragma once
#include <orderbook/types.hpp>
#include <cstddef>
#include <vector>

namespace orderbook {

// Open-addressed hash table: OrderId -> OrderPoolIndex.
// Linear probing, power-of-two capacity, tombstone deletion.
// Unlike std::unordered_map, there is no per-entry heap allocation and no
// pointer chasing: a probe walks contiguous memory.
class IdMap {
  public:
    explicit IdMap(std::size_t capacityPow2 = 1u << 15) {
        table_.assign(capacityPow2, Entry{kEmpty, kNullOrder});
        mask_ = capacityPow2 - 1;
    }

    OrderPoolIndex find(OrderId id) const {
        std::size_t i = slotFor(id);
        for (;;) {
            const Entry &e = table_[i];
            if (e.key == id) return e.slot;
            if (e.key == kEmpty) return kNullOrder;   // empty ends the probe
            i = (i + 1) & mask_;                      // tombstone: keep going
        }
    }

    void insert(OrderId id, OrderPoolIndex slot) {
        std::size_t i = slotFor(id);
        std::size_t firstTomb = kNoSlot;
        for (;;) {
            Entry &e = table_[i];
            if (e.key == id) { e.slot = slot; return; }   // overwrite
            if (e.key == kTombstone && firstTomb == kNoSlot) {
                firstTomb = i;                            // remember, keep probing
            }
            if (e.key == kEmpty) {
                std::size_t dst = (firstTomb == kNoSlot) ? i : firstTomb;
                table_[dst] = Entry{id, slot};
                ++size_;
                if (firstTomb != kNoSlot) --tombs_;
                maybeGrow();
                return;
            }
            i = (i + 1) & mask_;
        }
    }

    void erase(OrderId id) {
        std::size_t i = slotFor(id);
        for (;;) {
            Entry &e = table_[i];
            if (e.key == id) {
                e.key = kTombstone;
                e.slot = kNullOrder;
                --size_;
                ++tombs_;
                return;
            }
            if (e.key == kEmpty) return;   // not present
            i = (i + 1) & mask_;
        }
    }

    std::size_t size() const { return size_; }

  private:
    struct Entry {
        OrderId        key;
        OrderPoolIndex slot;
    };

    // id 0 is never a live order, so the key field doubles as the occupancy
    // marker -- no separate state array, one less cache line touched.
    static constexpr OrderId kEmpty     = 0;
    static constexpr OrderId kTombstone = ~OrderId{0};
    static constexpr std::size_t kNoSlot = ~std::size_t{0};

    // Fibonacci hashing: multiply then take the HIGH bits. Sequential ids
    // would otherwise cluster, since masking keeps only the low bits.
    std::size_t slotFor(OrderId id) const {
        const OrderId h = id * 0x9E3779B97F4A7C15ull;
        return static_cast<std::size_t>(h >> 32) & mask_;
    }

    // Keep load factor under ~0.7 counting tombstones, or probes degrade.
    void maybeGrow() {
        if ((size_ + tombs_) * 10 < table_.size() * 7) return;
        std::vector<Entry> old;
        old.swap(table_);
        table_.assign(old.size() * 2, Entry{kEmpty, kNullOrder});
        mask_ = table_.size() - 1;
        size_ = 0;
        tombs_ = 0;
        for (const Entry &e : old) {
            if (e.key != kEmpty && e.key != kTombstone) insert(e.key, e.slot);
        }
    }

    std::vector<Entry> table_;
    std::size_t mask_  = 0;
    std::size_t size_  = 0;
    std::size_t tombs_ = 0;
};

}  // namespace orderbook