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
