// Randomized quickselect.
// Primary author: Ethan Fernandez
#include "quickselect/quickselect.hpp"

#include <stdexcept>
#include <utility>

namespace sel {
namespace {

    // Algorithm version was selected via research specifically helpful was the site https://www.geeksforgeeks.org/dsa/quickselect-algorithm/
    // The Algorithm was then explained to me by Claude and the specifics behind this particular implemenation version and lomuto.
    std::size_t lomuto_partition(std::vector<Key>& a, std::size_t lo, std::size_t hi, Stats& stats) {
        const Key pivot = a[hi];

        std::size_t currentLow = lo;

        for (std::size_t i = lo; i < hi; ++i) {
            if (a[i] <= pivot) {
                std::swap(a[currentLow], a[i]);
                ++currentLow;
            }
        }

        std::swap(a[currentLow], a[hi]);
        return currentLow;
    }

}

Key quickselect(std::vector<Key>& a, std::size_t k, std::mt19937_64& rng, Stats& stats) {

    std::size_t lo = 0;
    std::size_t hi = a.size() - 1;

    while (lo < hi) {
        std::uniform_int_distribution<std::size_t> pick(lo, hi);
        std::swap(a[pick(rng)], a[hi]);

        const std::size_t p = lomuto_partition(a, lo, hi, stats);
        if (k == p) {
            return a[p];
        } else if (k < p) {
            // Note: k >= lo therefore p > lo >= 0
            hi = p - 1;
        } else {
            lo = p + 1;
        }

        return a[lo];

    }
}

}  // namespace sel
