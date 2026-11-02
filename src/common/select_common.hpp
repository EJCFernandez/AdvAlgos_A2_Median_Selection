#pragma once
// Shared types for both selection algorithms.
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace sel {

using Key = std::int64_t;

struct Stats {
    std::uint64_t comparisons = 0;
};

// Which partition scheme an algorithm uses.
//   Lomuto:   one pass, keys equal to the pivot all go to the left side.
//   ThreeWay: one pass into <, ==, > blocks, so equal keys never stay in play.
// Both algorithms support both schemes so they can be compared like for like.
enum class Partition { Lomuto, ThreeWay };

// Route EVERY key comparison through less() so both algorithms are counted
// identically. When SEL_COUNT_COMPARISONS is not defined (the timing build),
// the counter compiles away and costs nothing.
inline bool less(Key a, Key b, Stats& s) {
#ifdef SEL_COUNT_COMPARISONS
    ++s.comparisons;
#else
    (void)s;
#endif
    return a < b;
}

}  // namespace sel
